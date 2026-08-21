/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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

#ifndef SCORE_MW_LIFECYCLE_LM_CONTROL_SERVICE_H_
#define SCORE_MW_LIFECYCLE_LM_CONTROL_SERVICE_H_

#include <cstdint>

#include "score/mw/com/types.h"
#include "score/mw/lifecycle/ilm_control.hpp"

namespace score::mw::lifecycle::internal
{

// ============================================================================
// activate_run_target
// ============================================================================

/// @brief Controls how an activation request interacts with any in-progress activation.
///
/// @details Mirrors the `force` parameter of `ILmControl::activate_run_target`.
enum class ActivationMode : uint8_t
{
    kQueued = 0,  ///< Queue this request behind any in-progress activation.
    kForced = 1,  ///< Cancel any in-progress activation and the pending queue,
                  ///< then start this activation immediately.
};

/// @brief Request sent by the State Manager to activate a Run Target.
struct ActivateRunTargetRequest
{
    /// @brief Name of the Run Target to activate.
    RunTargetName run_target_name;

    /// @brief Whether to queue or force this activation.
    ActivationMode mode{ActivationMode::kQueued};
};

/// @brief Indicates whether the request was accepted or rejected by the Launch Manager.
enum class RequestStatus : uint8_t
{
    kAccepted = 0,  ///< The request was accepted.
    kRejected = 1,  ///< The request was rejected; see the accompanying rejection_reason field.
};

/// @brief Synchronous response to an activate_run_target request.
///
/// Carries only the immediate acceptance/rejection decision. Activation
/// completion is notified separately via the `activation_result` event.
struct ActivateRunTargetResponse
{
    /// @brief Whether the Launch Manager accepted or rejected the request.
    RequestStatus status{RequestStatus::kRejected};

    /// @brief Reason for rejection. Only meaningful when status == RequestStatus::kRejected.
    ExecErrc rejection_reason{ExecErrc::kGeneralError};
};

// ============================================================================
// get_active_run_target
// ============================================================================

/// @brief Indicates whether the Launch Manager can provide the currently active Run Target.
enum class QueryStatus : uint8_t
{
    kAvailable = 0,     ///< An active Run Target exists; see the run_target field.
    kNotAvailable = 1,  ///< No active Run Target; an activation is in progress.
};

/// @brief Synchronous response to a get_active_run_target query.
struct GetActiveRunTargetResponse
{
    /// @brief Whether an active Run Target is currently available.
    QueryStatus status{QueryStatus::kNotAvailable};

    /// @brief The currently active Run Target.
    ///        Only meaningful when status == QueryStatus::kAvailable.
    RunTargetName run_target;

    /// @brief Reason the active Run Target is not available.
    ///        Only meaningful when status == QueryStatus::kNotAvailable.
    ///        Expected value: kActivationInProgress.
    ExecErrc rejection_reason{ExecErrc::kGeneralError};
};

// ============================================================================
// activation_result event
// ============================================================================

/// @brief Event sent by the Launch Manager when a Run Target activation completes.
///
/// Activation always resolves into some Run Target — it cannot fail at the graph
/// level. The activated Run Target may differ from the one originally requested if a
/// preempting request or recovery action redirected the transition.
struct ActivationResult
{
    /// @brief The Run Target the Launch Manager activated.
    RunTargetName activated_run_target;

    /// @brief What caused this activation to occur.
    RunTargetActivationSource activation_source{RunTargetActivationSource::kStateManagerRequest};
};

// ============================================================================
// Service definition
// ============================================================================

/// @brief mw::com service definition for the Launch Manager control interface.
///
/// Instantiated twice by mw::com: once as LmControlProxy (client) and once as
/// LmControlSkeleton (server). mw::com injects @p Trait, which supplies the
/// side-specific Base class and the Method/Event templates below.
template <typename Trait>
class LmControlService : public Trait::Base
{
  public:
    /// Inherit mw::com's binding constructors.
    using Trait::Base::Base;

    /// @brief Request Run Target activation.
    ///
    /// Returns an `ActivateRunTargetResponse` synchronously indicating whether
    /// the request was accepted or rejected (e.g. queue full, unknown Run Target).
    /// If accepted, the actual activation result arrives asynchronously via
    /// the `activation_result` event.
    typename Trait::template Method<ActivateRunTargetResponse(ActivateRunTargetRequest)> activate_run_target{
        *this,
        "ActivateRunTarget"};

    /// @brief Query the currently active Run Target.
    ///
    /// Returns a `GetActiveRunTargetResponse` synchronously. If an activation is
    /// in progress, status is QueryStatus::kNotAvailable and rejection_reason
    /// carries kActivationInProgress. The caller should wait for the
    /// activation_result event and retry if needed.
    typename Trait::template Method<GetActiveRunTargetResponse()> get_active_run_target{*this, "GetActiveRunTarget"};

    /// @brief Event fired by the Launch Manager when a Run Target activation completes.
    typename Trait::template Event<ActivationResult> activation_result{*this, "ActivationResult"};
};

using LmControlProxy = score::mw::com::AsProxy<LmControlService>;
using LmControlSkeleton = score::mw::com::AsSkeleton<LmControlService>;

}  // namespace score::mw::lifecycle::internal

#endif  // SCORE_MW_LIFECYCLE_LM_CONTROL_SERVICE_H_
