/********************************************************************************
* Copyright (c) 2025 Contributors to the Eclipse Foundation
*
* See the NOTICE file(s) distributed with this work for additional
* information regarding copyright ownership.
*
* This program and the accompanying materials are made available under the
* terms of the Apache License Version 2.0 which is available at
* https://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
********************************************************************************/
// (c) 2025 ETAS GmbH. All rights reserved.

#ifndef PHMDAEMON_HPP_INCLUDED
#define PHMDAEMON_HPP_INCLUDED

/* RULECHECKER_comment(0, 3, check_include_errno, "Required to process clock_nanosleep return value", true_no_defect) */
#include <cerrno>
#include <memory>

#include <vector>
#include "score/lcm/lifecycle_client.h"
#include "score/lcm/control_client.h"
#include "etas/vrte/saf/common/PhmSignalHandler.hpp"
#include "etas/vrte/saf/daemon/PhmDaemonCmdLineParser.hpp"
#include "etas/vrte/saf/daemon/PhmDaemonConfig.hpp"
#include "etas/vrte/saf/daemon/SwClusterHandler.hpp"
#include "etas/vrte/saf/factory/MachineConfigFactory.hpp"
#include "etas/vrte/saf/ifexm/ProcessStateReader.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/timers/CycleTimeValidator.hpp"
#include "etas/vrte/saf/timers/CycleTimer.hpp"
#include "etas/vrte/saf/watchdog/IWatchdogIf.hpp"
namespace etas
{
namespace vrte
{
namespace saf
{
namespace daemon
{

/// @brief PHM daemon main class wraps the functionality for initialization and cyclic execution.
/// @details This is the main class responsible to execute the main functionalities of PHM daemon,
///          by using the necessary classes from this software component.
class PhmDaemon
{
public:
    /// @brief Return codes for PhmDaemon Initialization
    enum class EInitCode : std::int8_t
    {
        kNoError = 0,                          ///< Init Successful (no error occurred)
        kPrintHelpOrVersion = 1,               ///< Daemon start with -h or -v option
        kParsingError = -1,                    ///< Error while parsing cmdline arguments
        kCycleTimeInitFailed = -2,             ///< Cyclic Timer initialization failed
        kConstructFlatCfgFactoryFailed = -3,   ///< FlatCfgFactory failed loading SWCL configurations
        kWatchdogInitFailed = -4,              ///< Watchdog Initialization failed
        kWatchdogEnableFailed = -5,            ///< Enabling watchdog device failed
        kMachineConfigInitFailed = -6,         ///< MachineConfigFactory failed loading the machine configuration
        kSignalHandlerRegistrationFailed = -7  ///< Failed to register signal handler for termination signals
    };

    /* RULECHECKER_comment(0, 4, check_expensive_to_copy_in_parameter, "f_supervisionErrorInfo name is passed by value\
     as same as generated function", true_no_defect) */
    /// @brief Set the OS clock interface and the logger
    /// @param[in] f_osClock Access to the system clock (dependency injection possible in tests)
    /// @param[in] f_logger_r Reference to the logger (dependency injection possible in tests)
    /// @param[in] f_watchdog watchdog implementation (dependency injection possible in tests)
    /* RULECHECKER_comment(3,1, check_expensive_to_copy_in_parameter, "Move only types cannot be passed by const ref",
       true_no_defect) */
    PhmDaemon(etas::vrte::saf::timers::OsClockInterface& f_osClock, logging::PhmLogger& f_logger_r,
              std::unique_ptr<watchdog::IWatchdogIf> f_watchdog);

    /* RULECHECKER_comment(0, 4, check_min_instructions, "Default destructor is not provided\
       a function body", true_no_defect) */
    /// @brief Destroys the workers
    virtual ~PhmDaemon() = default;

    /// @brief No Copy Constructor
    PhmDaemon(const PhmDaemon&) = delete;
    /// @brief No Copy Assignment
    PhmDaemon& operator=(const PhmDaemon&) = delete;
    /// @brief No Move Constructor
    PhmDaemon(PhmDaemon&&) = delete;
    /// @brief No Move Assignment
    PhmDaemon& operator=(PhmDaemon&&) = delete;

    /// @brief Wraps the initialization steps of the PHM daemon
    /// (command line parsing, Constructing the workers, adjusting the cycle time, initialization of fixed step timer)
    /// @param[in] f_argc Number of arguments (shall be greater zero)
    /// @param[in] f_argv An array pointing to the command line arguments
    /// @return See EInitCode definition
    EInitCode init(int f_argc, char** f_argv) noexcept(false)
    {
        etas::vrte::saf::daemon::PhmDaemonCmdLineParser<> cmdLineParser{};
        const int8_t parseResult{cmdLineParser.parseOptions(f_argc, f_argv)};
        if (parseResult != 0)
        {
            return static_cast<EInitCode>(parseResult);
        }

        factory::MachineConfigFactory machineConfig{};
        if (!machineConfig.init())
        {
            return EInitCode::kMachineConfigInitFailed;
        }
        if (machineConfig.isHmShutdownEnabled())
        {
            if (!etas::vrte::saf::common::PhmSignalHandler::registerHandler())
            {
                return EInitCode::kSignalHandlerRegistrationFailed;
            }
        }

        if (!construct(machineConfig.getSupervisionBufferConfig()))
        {
            return EInitCode::kConstructFlatCfgFactoryFailed;
        }

        int64_t cycleTimeModified{static_cast<std::int64_t>(machineConfig.getCycleTimeInNs())};
        cycleTimeModified =
            etas::vrte::saf::timers::CycleTimeValidator::adjustCycleTimeOnClockAccuracy(cycleTimeModified, osClock);

        const int64_t timerInit{cycleTimer.init(cycleTimeModified)};
        if (timerInit > 0)
        {
            logger_r.LogInfo() << "Phm Daemon: The (configured) periodicity in [ns] is set to:"
                               << static_cast<uint64_t>(cycleTimeModified);
            logger_r.LogDebug() << "Phm Daemon: The accuracy of the monotonic system clock in [ns] is:"
                                << static_cast<uint64_t>(
                                       etas::vrte::saf::timers::CycleTimeValidator::getMonotonicClockAccuracy(
                                           osClock));
        }
        else
        {
            logger_r.LogError() << "Phm Daemon: Initialization of CycleTimer instance failed!";
            return EInitCode::kCycleTimeInitFailed;
        }

        if (!watchdog->init(cycleTimeModified, machineConfig))
        {
            logger_r.LogError() << "Phm Daemon: Initialization of watchdog failed!";
            return EInitCode::kWatchdogInitFailed;
        }

        if (!watchdog->enable())
        {
            logger_r.LogError() << "Phm Daemon: Enabling of watchdog failed!";
            return EInitCode::kWatchdogEnableFailed;
        }

        return EInitCode::kNoError;
    }

    /// @pre PhmDaemon::init() has been invoked without errors
    /// @brief Start cyclic execution
    /// @param[in] f_client Reference to an LifecycleClient object. This is given as a parameter from
    /// the outside for better testability. In the test case, not the real execution management needs
    /// to be informed about the execution state.
    /// @param[in] f_terminateCond Boolean predicate to determine when to terminate the cyclic loop
    /// (e.g. due to a signal received)
    /// @return bool false in case start of cyclic execution failed
    ///
    /// @tparam LifecycleClientType Template parameter allows the exchange of the real productive
    /// lifecycle client with a mock or stub for testing.
    /// @tparam TerminationSignalPredType Template parameter for the boolean condition to terminate the loop. This is
    /// normally deduced by the C++ compiler. It's a template parameter to also allow std::atomics for instance.
    /// @details
    /// 1. Report the execution state kRunning to the Execution Manager (log a waring if reporting was not possible)
    /// 2. Enter the cyclic loop:
    ///   - Sleep for the configured interval
    ///   - Perform the cyclic triggers
    ///   - Calculate the next absolute deadline until when to sleep
    /// @todo Introduce a threshold how many times a sleep may fail w/ an error, until the loop is terminated
    /// or add a strategy for recovery.
    /// @todo Add more sophisticated time handling (deviation reporting, reporting on sleep errors)
    /// @todo Rework the signal handling
    /// @todo Monitor the correct increment of the sleep interval
    /* RULECHECKER_comment(0, 4, check_cheap_to_copy_in_parameter, "f_terminateCond is passed as reference\
       for signal handling", true_no_defect) */
    template <typename LifecycleClientType, typename TerminationSignalPredType>
    bool startCyclicExec(LifecycleClientType& f_client, const TerminationSignalPredType& f_terminateCond) noexcept
    {
        auto rv{f_client.ReportExecutionState(score::lcm::ExecutionState::kRunning)};
        if (false == rv.has_value())
        {
            logger_r.LogWarn() << "Phm Daemon: Could not report execution state kRunning to LM!";
        }

        timers::NanoSecondType startTimestamp{cycleTimer.start()};
        if (startTimestamp == 0U)
        {
            logger_r.LogError() << "Phm Daemon: Failed to get initial timestamp";
            return false;
        }

#ifdef LAUNCH_MANAGER_ALIVE_SUPERVISION
        processStateReader.distributeExmActivation(startTimestamp);
#else
        logger_r.LogWarn() << "Phm Daemon: LM Alive supervision is not activated, since LM does not yet support "
                              "checkpoint reporting.";
#endif

        while (!f_terminateCond.load())
        {
            performCyclicTriggers();

            // Return value is neglected for the moment being.
            // The return value is used for unit tests until now, but can be used for more sophisticated monitoring
            // in the future.
            (void)cycleTimer.calcNextShot();

            // Sleep for the remaining cycle time or break out of cyclic loop if termination is requested
            std::uint64_t nsOverDeadline{0U};
            const int sleepResult{cycleTimer.sleep(f_terminateCond, nsOverDeadline)};
            if (sleepResult == EINTR)
            {
                logger_r.LogInfo() << "Phm Daemon: Sleep was interrupted by termination signal";
                break;
            }
            else if (sleepResult == timers::CycleTimer::kDeadlineAlreadyOver)
            {
                logger_r.LogDebug() << "Phm Daemon: Phm cycle took"
                                    << (static_cast<double>(nsOverDeadline) / 1000000.0 /*ns per ms*/)
                                    << "ms longer than the configured cycle time";
            }
            else if (sleepResult != 0)
            {
                logger_r.LogError() << "Phm Daemon: Error during sleep system call, Code:"
                                    << static_cast<uint64_t>(sleepResult);
            }
            else
            {
                /* sleeping successfully */
            }
        }
        logger_r.LogInfo() << "Phm Daemon: Received termination request - shutting down";

        watchdog->disable();
        return true;
    }

private:
    /// @brief Create SwCluster objects & Invoke construction of worker objects
    /// @details Create the SwclusterHandler objects and the workers for the SwclusterHandler
    /// @param[in] f_bufferConfig_r The buffer configuration used for worker construction
    /// @return bool true if workers creation succeeded, false otherwise
    bool construct(const factory::MachineConfigFactory::SupervisionBufferConfig& f_bufferConfig_r) noexcept(false);

    /// @brief Perform cyclic execution of Phm daemon
    /// @details Perform cyclic execution of Phm daemon functionalities, for e.g., evaluation of supervisions.
    void performCyclicTriggers(void);

    /// @brief System clock interface to access the monotonic clock for sleep
    etas::vrte::saf::timers::OsClockInterface& osClock;

    /// @brief For fixed time-step execution during the cyclic execution
    etas::vrte::saf::timers::CycleTimer cycleTimer;

    /// @brief Logging entity for warnings, errors used in init() phase and the cyclic() phase
    logging::PhmLogger& logger_r;

    /// @brief Recovery interface to Launch Manager
    std::unique_ptr<score::lcm::ControlClient> recoveryClient;

    /// @brief Vector of SwCluster handler
    std::vector<SwClusterHandler> swClusterHandlers;

    /// @brief Process State Reader for PHM daemon
    etas::vrte::saf::ifexm::ProcessStateReader processStateReader;

    /// @brief Connection to watchdog devices
    std::unique_ptr<watchdog::IWatchdogIf> watchdog;
};

}  // namespace daemon
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
