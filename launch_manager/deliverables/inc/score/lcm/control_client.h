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
#ifndef CONTROL_CLIENT_H_
#define CONTROL_CLIENT_H_

#include <functional>

#include "score/concurrency/future/interruptible_future.h"
#include "score/concurrency/future/interruptible_promise.h"
#include "score/result/result.h"
#include <string_view>
#include "score/lcm/exec_error_domain.h"
#include "score/lcm/execution_error_event.h"

namespace score {

namespace lcm {

class ControlClientImpl;

/// @brief Class representing connection to Launch Manager that is used to request Process Group state transitions (or other operations).
/// @note ControlClient opens communication channel to Launch Manager (e.g. POSIX FIFO). Each Process that intends to perform state management, shall create an instance of this class and it shall have rights to use it.
///
class ControlClient final {
   public:
    /// @brief Constructor that creates Control Client instance.
    ///
    /// Registers given callback which is called in case a Process Group changes its state unexpectedly to an Undefined
    /// Process Group State.
    ///
    /// @param[in] undefinedStateCallback      callback to be invoked by ControlClient library if a ProcessGroup changes its
    ///                                        state unexpectedly to an Undefined Process Group State, i.e. without
    ///                                        previous request by SetState(). The affected ProcessGroup and ExecutionError
    ///                                        is provided as an argument to the callback in form of ExecutionErrorEvent.
    ///
    explicit ControlClient(std::function<void(const score::lcm::ExecutionErrorEvent&)> undefinedStateCallback) noexcept;

    /// @brief Destructor of the Control Client instance.
    /// @param  None.
    ///
    ~ControlClient() noexcept;

    // Applying the rule of five
    // Class will not be copyable, but it will be movable

    /// @brief Suppress default copy construction for ControlClient.
    ControlClient(const ControlClient&) = delete;

    /// @brief Suppress default copy assignment for ControlClient.
    ControlClient& operator=(const ControlClient&) = delete;

    /// @brief Intentional use of default move constructor for ControlClient.
    ///
    /// @param[in] rval reference to move
    ControlClient(ControlClient&& rval) noexcept;

    /// @brief Intentional use of default move assignment for ControlClient.
    ///
    /// @param[in] rval reference to move
    /// @returns the new reference
    ControlClient& operator=(ControlClient&& rval) noexcept;

    /// @brief Method to request state transition for a single Process Group.
    ///
    /// This method will request Launch Manager to perform state transition and return immediately.
    /// Returned InterruptibleFuture can be used to determine result of requested transition.
    ///
    /// @param[in] pg_name representing meta-model definition of a specific Process Group 
    /// @param[in] pg_state representing meta-model definition of a state. Launch Manager will perform state transition from the current state to the state identified by this parameter.
    /// @returns void if requested transition is successful, otherwise it returns ExecErrorDomain error.
    /// @error score::lcm::ExecErrc::kCancelled if transition to the requested Process Group state was cancelled by a newer request
    /// @error score::lcm::ExecErrc::kFailed if transition to the requested Process Group state failed
    /// @error score::lcm::ExecErrc::kFailedUnexpectedTerminationOnExit if Unexpected Termination in Process of previous Process Group State happened.
    /// @error score::lcm::ExecErrc::kFailedUnexpectedTerminationOnEnter if Unexpected Termination in Process of target Process Group State happened.
    /// @error score::lcm::ExecErrc::kInvalidArguments if arguments passed doesn't appear to be valid (e.g. after a software update, given processGroup doesn't exist anymore)
    /// @error score::lcm::ExecErrc::kCommunicationError if ControlClient can't communicate with Launch Manager (e.g. IPC link is down)
    /// @error score::lcm::ExecErrc::kAlreadyInState if the ProcessGroup is already in the requested state
    /// @error score::lcm::ExecErrc::kInTransitionToSameState if a transition to the requested state is already ongoing
    /// @error score::lcm::ExecErrc::kInvalidTransition if transition to the requested state is prohibited (e.g. Off state for MainPG)
    /// @error score::lcm::ExecErrc::kGeneralError if any other error occurs.
    ///
    /// @threadsafety{thread-safe}
    ///
    score::concurrency::InterruptibleFuture<void> SetState(const IdentifierHash& pg_name, const IdentifierHash& pg_state) const noexcept;

    /// @brief Method to retrieve result of Machine State initial transition to Startup state.
    ///
    /// This method allows State Management to retrieve result of a transition.
    /// Please note that this transition happens once per machine life cycle, thus result delivered by this method shall not change (unless machine is started again).
    ///
    /// @param  None.
    /// @returns void if requested transition is successful, otherwise it returns ExecErrorDomain error.
    /// @error score::lcm::ExecErrc::kCancelled if transition to the requested Process Group state was cancelled by a newer request
    /// @error score::lcm::ExecErrc::kFailed if transition to the requested Process Group state failed
    /// @error score::lcm::ExecErrc::kCommunicationError if ControlClient can't communicate with Launch Manager (e.g. IPC link is down)
    /// @error score::lcm::ExecErrc::kGeneralError if any other error occurs.
    ///
    /// @threadsafety{thread-safe}
    ///
    score::concurrency::InterruptibleFuture<void> GetInitialMachineStateTransitionResult() const noexcept;

    /// @brief Returns the execution error which changed the given Process Group to an Undefined Process Group State.
    ///
    /// This function will return with error and will not return an ExecutionErrorEvent object, if the given
    /// Process Group is in a defined Process Group state again.
    ///
    /// @param[in] processGroup   Process Group of interest.
    ///
    /// @returns The execution error which changed the given Process Group to an Undefined Process Group State.
    /// @error score::lcm::ExecErrc::kFailed    Given Process Group is not in an Undefined Process Group State.
    /// @error score::lcm::ExecErrc::kCommunicationError if ControlClient can't communicate with Launch Manager (e.g. IPC link is down)
    ///
    /// @threadsafety{thread-safe}
    ///
    score::Result<score::lcm::ExecutionErrorEvent> GetExecutionError(
        const IdentifierHash& processGroup) noexcept;

   private:
    /// @brief Pointer to implementation (Pimpl), we use this pattern to provide ABI compatibility.
    std::unique_ptr<ControlClientImpl> control_client_impl_;
};

}  // namespace lcm

}  // namespace score

#endif  // CONTROL_CLIENT_H_
