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

#ifndef CHECKPOINT_HPP_INCLUDED
#define CHECKPOINT_HPP_INCLUDED

#include <string>
#include <string_view>

#include <cstdint>

#include "score/mw/launch_manager/alive_monitor/details/common/Observer.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/alive_monitor/details/timers/Timers_OsClock.hpp"

namespace score
{
namespace mw::lifecycle::internal
{
namespace saf
{
namespace ifappl
{

/// @brief Checkpoint Observer
/// @details The checkpoint observer class acts as a distributor between a Monitor interface
/// and a Supervision. It forwards only those checkpoint informations which are required for the
/// attached supervisions.
class Checkpoint : public saf::common::Observable<Checkpoint>
{
  public:
    /// @brief No Default Constructor.
    Checkpoint() = delete;

    /// @brief No Copy Constructor
    Checkpoint(const Checkpoint&) = delete;
    /// @brief No Copy Assignment
    Checkpoint& operator=(const Checkpoint&) = delete;
    /// @brief No Move Assignment
    Checkpoint& operator=(Checkpoint&&) = delete;

    /// @brief Constructor
    /// @param [in] f_processState_p            The process that is reporting this checkpoint
    /// @throws std::bad_alloc in case of insufficient memory for string allocation
    explicit Checkpoint(const ifexm::ObservableEvent* f_processState_p) noexcept(false);

    /// @brief Default Move Constructor
    /// Cannot be noexcept, since the base class move constructor is not noexcept
    /* RULECHECKER_comment(0, 7, check_min_instructions, "Default constructor is not provided\
       a function body", true_no_defect) */
    /* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
       the member initializer", false) */
    /* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "The default move constructor invokes parameterised\
       constructor internally. This invokes std::string copy construction", true_no_defect) */
    Checkpoint(Checkpoint&&) = default;

    /// @brief Default Destructor
    ~Checkpoint() override = default;

    /// @brief Get timestamp
    /// @return NanoSecondType  Timestamp value of the reported checkpoint in [nano seconds]
    score::mw::lifecycle::internal::saf::timers::NanoSecondType getTimestamp(void) const noexcept(true);

    /// @brief Push data to checkpoint observer
    /// @details Push the checkpoint timestamp to the checkpoint observer to notify it was reported
    /// @param [in] f_timestamp     Timestamp value captured when the checkpoint was reported in [nano seconds]
    void pushData(const score::mw::lifecycle::internal::saf::timers::NanoSecondType f_timestamp) noexcept(true);

    /// @brief Set data loss event
    /// @details Set data loss event in the checkpoint observer
    /// @param [in] f_isDataLossEvent   set data loss event marker
    void setDataLossEvent(const bool f_isDataLossEvent) noexcept(true);

    /// @brief Is Data loss event
    /// @return     Data loss event occurred (true)
    bool getDataLossEvent(void) const noexcept(true);

    /// @brief Return the process that is reporting this checkpoint
    /// @return observable event
    const ifexm::ObservableEvent* getProcess(void) const noexcept(true);

  private:
    /// @brief The process that is reporting this checkpoint
    const ifexm::ObservableEvent* processState;

    /// @brief Data loss event marker
    bool isDataLossEvent;

    /// @brief Timestamp value in [nano seconds]
    score::mw::lifecycle::internal::saf::timers::NanoSecondType timestamp;
};

}  // namespace ifappl
}  // namespace saf
}  // namespace mw::lifecycle::internal
}  // namespace score

#endif
