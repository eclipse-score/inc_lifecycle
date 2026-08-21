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
#ifndef SCORE_MW_LIFECYCLE_ILM_CONTROL_H_
#define SCORE_MW_LIFECYCLE_ILM_CONTROL_H_

#include <functional>
#include <memory>

#include "score/mw/lifecycle/execution_error.h"
#include "score/mw/lifecycle/fixed_string.hpp"
#include "score/mw/lifecycle/run_target_activation_source.hpp"
#include "score/result/result.h"

namespace score::mw::lifecycle
{

/// @brief Maximum number of usable characters in a RunTargetName, excluding the null terminator.
///
/// Names longer than this are silently truncated. The internal storage is
/// `kMaxRunTargetNameLength + 1` bytes to always accommodate the terminator.
inline constexpr std::size_t kMaxRunTargetNameLength = 128U;

/// @brief Alias for the Run Target name type used throughout the lifecycle API.
using RunTargetName = FixedString<kMaxRunTargetNameLength>;

/// @brief Public interface for controlling the Launch Manager from a State Manager.
///
/// Callers obtain an instance via ILmControl::Create() and interact with the
/// Launch Manager exclusively through this interface. The concrete implementation
/// is an internal detail - it is not visible in this header.
///
/// @par Usage
/// ```cpp
/// auto result = ILmControl::Create();
/// if (!result.has_value()) { /* handle error */ }
/// std::unique_ptr<ILmControl> lm = std::move(result).value();
/// ```
///
/// @par Ownership
/// Instances are owned through `std::unique_ptr<ILmControl>`. The interface is
/// non-copyable and non-movable to prevent slicing; transfer ownership via the
/// unique_ptr instead.
class ILmControl
{
  public:
    /// @brief Callback fired when a Run Target activation settles.
    ///
    /// Activation cannot fail: it always resolves into some Run Target,
    /// though not necessarily the one that was requested (e.g. a
    /// recovery action may activate a different Run Target).
    /// The subscriber sees every settling.
    ///
    /// @note The callback must not throw exceptions.
    ///
    /// @param[in] activationSource   What caused the activation to occur.
    /// @param[in] activatedRunTarget The Run Target that was activated - may
    ///                               differ from the one originally requested.
    using ActivationCallback =
        std::function<void(RunTargetActivationSource activationSource, RunTargetName activatedRunTarget)>;

    /// @brief Factory method - create a ILmControl instance.
    ///
    /// @warning **Work in progress  API shape not yet finalised.**
    ///          The signature and behaviour of this method may change. Open
    ///          questions include how much of the mw::com initialisation and
    ///          configuration is the caller's responsibility versus handled
    ///          internally, and whether the instance specifier is the right
    ///          abstraction to expose at this level. Do not treat this interface
    ///          as stable until these decisions are resolved.
    ///
    /// Initiates the mw::com service discovery to the Launch Manager.
    /// Until the connection is established, method calls return kCommunicationError.
    ///
    /// @pre mw::com must be fully initialized by the caller before invoking
    ///      this function. Create() does not initialize mw::com internally.
    ///
    /// @param[in] instance_specifier  The mw::com instance specifier identifying
    ///                                the Launch Manager service. Must be a
    ///                                non-empty, valid specifier string. An empty
    ///                                or malformed value is treated as an error.
    ///
    /// @returns A unique_ptr to the ILmControl instance on success.
    ///
    /// @error kInvalidArguments instance_specifier is empty or malformed.
    /// @error kCommunicationError The service discovery could not be started.
    static score::Result<std::unique_ptr<ILmControl>> Create(std::string_view instance_specifier);

    /// @brief Virtual destructor for safe deletion through this interface.
    virtual ~ILmControl() noexcept = default;

    // Non-copyable and non-movable.
    // Transfer ownership via std::unique_ptr<ILmControl> instead.
    ILmControl(const ILmControl&) = delete;
    ILmControl& operator=(const ILmControl&) = delete;
    ILmControl(ILmControl&&) = delete;
    ILmControl& operator=(ILmControl&&) = delete;

    /// @brief Request Run Target activation.
    ///
    /// Posts the request into the Launch Manager's fixed-capacity FIFO queue
    /// and returns as soon as the request is accepted. The Launch Manager
    /// executes activations one at a time in FIFO order. Completion is
    /// notified asynchronously via the callback registered with
    /// register_run_target_activation_callback().
    /// If the queue is full, kRequestQueueIsFull is returned immediately
    /// and the request is discarded.
    ///
    /// @param[in] runTargetName  Name of a Run Target configured in the Launch Manager.
    /// @param[in] force          If false (default), the request is queued behind any
    ///                           in-progress activation and executed afterwards.
    ///                           If true, any in-progress activation is cancelled, the
    ///                           queue is cleared, and this activation starts immediately.
    ///
    /// @returns void when the Launch Manager accepted the request.
    ///
    /// @error kRequestQueueIsFull   Activation was rejected because Launch Manager cannot accept another activation
    /// request.
    /// @error kRunTargetDoesntExist Name of the requested Run Target does not exist in current configuration.
    /// @error kCommunicationError         Connection with Launch Manager could not be established and request cannot be
    /// sent.
    virtual score::Result<void> activate_run_target(RunTargetName runTargetName, bool force = false) = 0;

    /// @brief Register a callback invoked whenever Launch Manager finishes a Run Target activation.
    ///
    /// Only a single subscriber is supported. The expected usage is
    /// register-once during setup, leaving the callback in place until
    /// the ILmControl instance is destroyed. Re-registration is
    /// supported defensively: a second call overrides the previous
    /// setting. There is no un-register path; empty callbacks are rejected.
    ///
    /// @param[in] callback The callback to invoke when Run Target activation finishes.
    ///
    /// @returns void on success.
    ///
    /// @error kInvalidArguments   The callback is empty.
    virtual score::Result<void> register_run_target_activation_callback(ActivationCallback callback) = 0;

    /// @brief Query the Launch Manager for the currently active Run Target.
    ///
    /// Sends a synchronous request to the Launch Manager and returns the name of
    /// the Run Target the Launch Manager has settled on. If an activation is in
    /// progress, the Launch Manager has not settled on any single Run Target yet
    /// and the call returns kActivationInProgress instead. In that case, the caller
    /// should wait for the activation completion callback and retry if needed.
    ///
    /// @returns the name of the currently active Run Target.
    ///
    /// @error kActivationInProgress Launch Manager is currently executing a Run Target
    ///                              activation and thus there is no single Run Target active.
    /// @error kCommunicationError   Connection with Launch Manager could not be established
    ///                              and information about active Run Target cannot be retrieved.
    virtual score::Result<RunTargetName> get_active_run_target() = 0;

  protected:
    ILmControl() = default;
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_MW_LIFECYCLE_ILM_CONTROL_H_
