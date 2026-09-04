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

#ifndef PHMDAEMON_HPP_INCLUDED
#define PHMDAEMON_HPP_INCLUDED

/* RULECHECKER_comment(0, 3, check_include_errno, "Required to process clock_nanosleep return value", true_no_defect) */
#include <cerrno>
#include <memory>

#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/mw/launch_manager/alive_monitor/details/common/EInitCode.hpp"
#include "score/mw/launch_manager/alive_monitor/details/daemon/PhmDaemonConfig.hpp"
#include "score/mw/launch_manager/alive_monitor/details/daemon/SupervisionManager.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEventReader.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/CycleTimeValidator.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/CycleTimer.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/TimeConversion.hpp"
#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/supervision_control_client/isupervision_factory.hpp"

namespace score::mw::lifecycle::internal::saf::daemon
{

/// @brief PHM daemon main class wraps the functionality for initialization and cyclic execution.
/// @details This is the main class responsible to execute the main functionalities of PHM daemon,
///          by using the necessary classes from this software component.
class PhmDaemon final : public ISupervisionFactory
{
  public:
    using OsClock = score::mw::lifecycle::internal::saf::timers::OsClockInterface;
    using RecoveryClient = score::mw::lifecycle::IRecoveryClient;
    using CycleTimer = score::mw::lifecycle::internal::saf::timers::CycleTimer;
    using CycleTimeValidator = score::mw::lifecycle::internal::saf::timers::CycleTimeValidator;
    using NanoSecondType = score::mw::lifecycle::internal::saf::timers::NanoSecondType;
    using ObservableEventReader = score::mw::lifecycle::internal::saf::ifexm::ObservableEventReader;
    using Config = score::mw::lifecycle::internal::configuration::Config;

    /// @brief Set the OS clock interface
    /// @param[in] f_osClock Access to the system clock (dependency injection possible in tests)
    /// @param[in] supervised_components Number of components that will register alive supervision
    /// in tests)
    /* RULECHECKER_comment(3,1, check_expensive_to_copy_in_parameter, "Move only types cannot be passed by const ref",
       true_no_defect) */
    explicit PhmDaemon(OsClock& f_osClock, std::size_t supervised_components);

    /// @brief Destroys the workers
    ~PhmDaemon() override = default;

    /// @brief No Copy Constructor
    PhmDaemon(const PhmDaemon&) = delete;
    /// @brief No Copy Assignment
    PhmDaemon& operator=(const PhmDaemon&) = delete;
    /// @brief No Move Constructor
    PhmDaemon(PhmDaemon&&) = delete;
    /// @brief No Move Assignment
    PhmDaemon& operator=(PhmDaemon&&) = delete;

    /// @brief Wraps the initialization steps of the PHM daemon
    /// (Constructing the workers, adjusting the cycle time, initialization of fixed step timer)
    /// @param[in] recovery_client Shared pointer to recovery client
    /// @param[in] config Config holding alive monitor and component configuration
    /// @return See EInitCode definition
    EInitCode init(
        std::shared_ptr<RecoveryClient> recovery_client,
        const configuration::AliveSupervisionConfig& config) noexcept(false)
    {
        recoveryClient = recovery_client;

        int64_t cycleTimeModified{
            static_cast<std::int64_t>(timers::TimeConversion::convertMilliSecToNanoSec(config.evaluation_cycle_ms))};

        cycleTimeModified = CycleTimeValidator::adjustCycleTimeOnClockAccuracy(cycleTimeModified, osClock);

        const int64_t timerInit{cycleTimer.init(cycleTimeModified)};
        if (timerInit > 0)
        {
            LM_LOG_INFO() << "Phm Daemon: The (configured) periodicity in [ns] is set to:"
                          << static_cast<uint64_t>(cycleTimeModified);
            LM_LOG_DEBUG() << "Phm Daemon: The accuracy of the monotonic system clock in [ns] is:"
                           << static_cast<uint64_t>(CycleTimeValidator::getMonotonicClockAccuracy(osClock));
        }
        else
        {
            LM_LOG_ERROR() << "Phm Daemon: Initialization of CycleTimer instance failed!";
            return EInitCode::kCycleTimeInitFailed;
        }

        return EInitCode::kNoError;
    }

    /// @pre PhmDaemon::init() has been invoked without errors
    /// @brief Start cyclic execution
    /// @param[in] f_terminateCond Boolean predicate to determine when to terminate the cyclic loop
    /// (e.g. due to a signal received)
    /// @return bool false in case start of cyclic execution failed
    ///
    /// @tparam TerminationSignalPredType Template parameter for the boolean condition to terminate the loop. This is
    /// normally deduced by the C++ compiler. It's a template parameter to also allow std::atomics for instance.
    /// @details
    /// Enter the cyclic loop:
    /// - Sleep for the configured interval
    /// - Perform the cyclic triggers
    /// - Calculate the next absolute deadline until when to sleep
    /// @todo Introduce a threshold how many times a sleep may fail w/ an error, until the loop is terminated
    /// or add a strategy for recovery.
    /// @todo Add more sophisticated time handling (deviation reporting, reporting on sleep errors)
    /// @todo Rework the signal handling
    /// @todo Monitor the correct increment of the sleep interval
    /* RULECHECKER_comment(0, 4, check_cheap_to_copy_in_parameter, "f_terminateCond is passed as reference\
       for signal handling", true_no_defect) */
    template <typename TerminationSignalPredType>
    bool startCyclicExec(const TerminationSignalPredType& f_terminateCond) noexcept
    {
        NanoSecondType startTimestamp{cycleTimer.start()};
        if (startTimestamp == 0U)
        {
            LM_LOG_ERROR() << "Phm Daemon: Failed to get initial timestamp";
            return false;
        }

#ifdef LAUNCH_MANAGER_ALIVE_SUPERVISION
        processStateReader.distributeExmActivation(startTimestamp);
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
                LM_LOG_INFO() << "Phm Daemon: Sleep was interrupted by termination signal";
                break;
            }
            else if (sleepResult == CycleTimer::kDeadlineAlreadyOver)
            {
                LM_LOG_DEBUG() << "Phm Daemon: Phm cycle took"
                               << (static_cast<double>(nsOverDeadline) / 1000000.0 /*ns per ms*/)
                               << "ms longer than the configured cycle time";
            }
            else if (sleepResult != 0)
            {
                LM_LOG_ERROR() << "Phm Daemon: Error during sleep system call, Code:"
                               << static_cast<uint64_t>(sleepResult);
            }
            else
            {
                /* sleeping successfully */
            }
        }
        LM_LOG_INFO() << "Phm Daemon: Received termination request - shutting down";

        return true;
    }

    /// @brief @see ISupervisonFactory::constructSupervision
    std::unique_ptr<IActivationStateReporter> constructSupervision(
        const IdentifierHash id,
        const uid_t uid,
        const configuration::ComponentAliveSupervision& config) override
    {
        SCORE_LANGUAGE_FUTURECPP_ASSERT_DBG_MESSAGE(
            !supervisionManager.full(), "More alive supervisions than expected were constructed");
        if (supervisionManager.constructWorker(id, config, uid, recoveryClient, supervisionStateReader_))
        {
            return std::make_unique<SupervisionHandle>(id, buffer_);
        }
        return {};
    }

  private:
    /// @brief Perform cyclic execution of Phm daemon
    /// @details Perform cyclic execution of Phm daemon functionalities, for e.g., evaluation of supervisions.
    void performCyclicTriggers(void);

    /// @brief System clock interface to access the monotonic clock for sleep
    OsClock& osClock;

    /// @brief For fixed time-step execution during the cyclic execution
    CycleTimer cycleTimer;

    /// @brief Buffer that supervision events are pushed to and read from
    std::shared_ptr<SupervisionBufferType> buffer_;

    /// @brief Recovery interface to Launch Manager
    std::shared_ptr<RecoveryClient> recoveryClient;

    /// @brief Handler to construct and store objects needed for alive supervision
    SupervisionManager supervisionManager;

    /// @brief Observable Event Reader for PHM daemon
    ObservableEventReader supervisionStateReader_;
};

}  // namespace score::mw::lifecycle::internal::saf::daemon

#endif
