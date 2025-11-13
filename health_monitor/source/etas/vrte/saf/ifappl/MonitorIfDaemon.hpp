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
#ifndef MonitorIfDaemon_HPP_INCLUDED
#define MonitorIfDaemon_HPP_INCLUDED

#include <string>
#include <vector>
#include "etas/vrte/saf/ifappl/Checkpoint.hpp"
#include "etas/vrte/saf/ifappl/DataStructures.hpp"
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace ifexm
{
class ProcessState;
}
namespace supervision
{
class Local;
class Global;
}  // namespace supervision

namespace ifappl
{

/// @brief Monitor Interface for PHM Deamon
/// @details The MonitorIfDaemon class provides methods to write/read information to the
/// data exchange between PHM daemon and Application, which are only required on PHM Daemon side.
class MonitorIfDaemon : public common::Observer<ifexm::ProcessState>
{
public:
    /// @brief No Default Constructor
    MonitorIfDaemon() = delete;

    /// @brief No Copy Constructor
    MonitorIfDaemon(const MonitorIfDaemon&) = delete;
    /// @brief No Copy Assignment
    MonitorIfDaemon& operator=(const MonitorIfDaemon&) = delete;
    /// @brief No Move Assignment
    MonitorIfDaemon& operator=(MonitorIfDaemon&&) = delete;

    /// @brief Constructor
    /// @param [in] f_ipcServer_r       IPC interface
    /// @param [in] f_interfaceName_p   Interface name
    /// @throws std::bad_alloc in case of insufficient memory for string allocation
    /// @warning Please ensure that the pointer argument passed to the constructor is always a valid
    /// pointer, as the constructor doesn't check for null pointer access!
    explicit MonitorIfDaemon(CheckpointIpcServer& f_ipcServer_r,
                                      const char* f_interfaceName_p) noexcept(false);

    /// @brief Default Move Constructor
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "The default move constructor invokes parameterised\
       constructor internally. This invokes std::string copy construction", true_no_defect) */
    MonitorIfDaemon(MonitorIfDaemon&&) = default;

    /// @brief Default Destructor
    /* RULECHECKER_comment(0, 3, check_min_instructions, "Default destructor is not provided\
       a function body", true_no_defect) */
    ~MonitorIfDaemon() override = default;

    /// @brief Get interface Name
    /// @return     Interface name as string
    const std::string& getInterfaceName(void) const noexcept(true);

    /// @brief Attach checkpoint
    /// @details Attaches a checkpoint observer to the Monitor interface
    /// Note: Attached observers will receive updates in case there is new information
    /// @param [in] f_checkpoint_r      Checkpoint which is added to the observer array
    /// @throws std::bad_alloc in case of insufficient memory for vector allocation
    void attachCheckpoint(Checkpoint& f_checkpoint_r) noexcept(false);

    /// @brief Update data received from ProcessState
    /// @param [in]  f_observable_r ProcessState object which has send the update
    void updateData(const ifexm::ProcessState& f_observable_r) noexcept(true) override;

    /// @brief Check for new data
    /// @details Check Monitor interface for new data from application side
    /// @param [in]  f_syncTimestamp    Timestamp till data shall be read, newer data will not be considered
    void checkForNewData(const etas::vrte::saf::timers::NanoSecondType f_syncTimestamp) noexcept(true);

private:
    /// @brief Check if checkpoint ring buffer overflow has occurred
    /// @details The tailing read index (readIndex - 1) is always expected to be 0 (Clearing must be assured).
    /// This acts as a marker, if the marker is written with a timestamp the ring buffer was completely filled
    /// or in worst case completely overwritten.
    /// @return bool        true: Overflow occurred
    ///                     false: No overflow detected.
    bool isBufferOverflowed(void) const;

    /// @brief Push overflow event information to all checkpoint observer
    /// @details Every attached checkpoint observer will be informed that a data loss event in the
    /// Monitor has occurred
    void pushOverflowInfoToCheckpointObservers(void) const;

    /// @brief Move to kInactiveOverflow state and push overflow event to observers
    void handleOverflow(void);

    /// @brief Push new data to checkpoint observer
    /// @details The checkpoint ring buffer data is pushed to checkpoint specific objects.
    /// @param [in]  f_syncTimestamp        Timestamp till data shall be read, newer data will not be considered
    /// @returns True if reading data from IPC channel and pushing data to observers was successful, else false
    bool pushNewDataToCheckpointObservers(const etas::vrte::saf::timers::NanoSecondType f_syncTimestamp);

    /// @brief Push a single checkpoint to observers
    /// @param[in] f_elem_r The checkpoint to push to observers
    void pushCheckpointToObservers(CheckpointBufferElement& f_elem_r);

    /// Internal states for instances of this class
    enum class EInternalState : std::uint8_t
    {
        kActive,           ///< Interface active
        kInactive,         ///< Interface inactive
        kInactiveOverflow  ///< Interface inactive due to overflow
    };

    /// Current internal state
    EInternalState status{EInternalState::kInactive};

    /// @brief Current Activation request
    bool isActivateRequest{false};

    /// @brief Current Deactivation request
    bool isDeactivateRequest{false};

    /// @brief Process restart status
    bool isProcessRestarted{false};

    /// Interface name
    const std::string k_interfaceName;

    /// Array of checkpoint observers attached to the Monitor interface
    std::vector<Checkpoint*> checkpointObservers{};

    /// @brief IPC connection to application
    CheckpointIpcServer& ipcserver_r;

    /// Logger
    logging::PhmLogger& logger_r;
};

}  // namespace ifappl
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
