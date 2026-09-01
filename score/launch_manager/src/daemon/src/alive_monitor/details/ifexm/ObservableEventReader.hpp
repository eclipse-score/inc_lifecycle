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

#ifndef OBSERVABLEEVENTREADER_HPP_INCLUDED
#define OBSERVABLEEVENTREADER_HPP_INCLUDED

#include <map>

#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"
#include "score/mw/launch_manager/supervision_control_client/supervision_event.hpp"

#include "score/result/result.h"

namespace score::mw::lifecycle::internal::saf::ifexm
{

/// @brief Observable Event reader
/// @details The Observable Event reader fetches supervision events via the lcm library and distributes
/// the information to the Observable Event classes.
class ObservableEventReader
{
  public:
    /// @brief Constructor
    /// @param [in] f_observable_event_receiver   Shared pointer to the ring buffer used to receive supervision events
    explicit ObservableEventReader(std::shared_ptr<SupervisionBufferType> f_observable_event_receiver);

    /// @brief No Copy Constructor
    ObservableEventReader(const ObservableEventReader&) = delete;
    /// @brief No Move Constructor
    ObservableEventReader(ObservableEventReader&&) = delete;
    /// @brief No Copy Assignment
    ObservableEventReader& operator=(const ObservableEventReader&) = delete;
    /// @brief No Move Assignment
    ObservableEventReader& operator=(ObservableEventReader&&) = delete;

    /// @brief Default Destructor
    virtual ~ObservableEventReader() = default;

    /// @brief Register observable events for reader
    /// @param [in]  f_processState_r   Process state to be registered
    /// @param [in]  f_processId        Process ID
    /// @return     true (registered), false (not registered)
    bool registerObservableEvent(ObservableEvent& f_processState_r, const IdentifierHash f_processId) noexcept(false);

    /// @brief Deregister observable events from reader
    /// @param [in]  f_processId        Process ID to deregister the particular process
    void deregisterObservableEvent(const IdentifierHash f_processId) noexcept;

    /// @brief Distribute changes
    /// @details Distribute supervision events to the registered Observable Event classes
    /// @param [in] f_syncTimestamp   Timestamp for cyclic synchronization
    /// @return     true (successful distribution), false (failed distribution)
    bool distributeChanges(const timers::NanoSecondType f_syncTimestamp) noexcept;

  private:
    /// @brief Push update for changed registered process
    /// @param [in] f_event              Supervision event for which push update is needed
    /// @param [in] f_syncTimestamp      Timestamp for cyclic synchronization
    /// @return     true (sync timestamp is reached), false (sync timestamp is not yet reached)
    bool pushUpdateTill(const SupervisionEvent& f_event, const timers::NanoSecondType f_syncTimestamp) noexcept;

    /// @brief Returns a queued SupervisionEvent that has not yet been parsed.
    /// @returns Result containing SupervisionEvent in case of success, or ExecError in case of failure.
    score::Result<std::optional<SupervisionEvent>> getNextSupervisionEvent() noexcept;

    /// @brief Ring buffer through which supervision events are received from the Launch Manager
    std::shared_ptr<SupervisionBufferType> buffer_;

    /// @brief Map for process id and observable event object
    std::map<IdentifierHash, ObservableEvent*> processStateMap{};

    /// @brief Flag for pending pushData from previous distribution of observable event changes
    bool isPushPending{false};

    /// @brief Pointer for last changed process for which push update is pending
    ObservableEvent* lastChangedProcess_p{nullptr};
};

}  // namespace score::mw::lifecycle::internal::saf::ifexm

#endif
