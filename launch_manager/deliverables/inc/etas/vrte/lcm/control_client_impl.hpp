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

#ifndef CONTROL_CLIENT_IMPL_H_
#define CONTROL_CLIENT_IMPL_H_

#include <array>
#include <mutex>
#include <score/lcm/control_client.h>

#include <score/lcm/identifier_hash.hpp>
#include <atomic>
#include <etas/vrte/lcm/config.hpp>
#include <etas/vrte/lcm/osal/semaphore.hpp>
#include <etas/vrte/lcm/controlclientchannel.hpp>
#include <functional>

namespace score {

namespace lcm {

/// @brief Data structure used to manage active, i.e. still not completed, requests from ControlClient.
/// Most of the ControlClient methods are asynchronous. This means that when an API call returns,
/// the request was communicated to LCM and LCM agreed to work on it. The result of that request usually
/// arrives later, so ControlClientImpl needs a storage where those active requests can wait for completion.
struct ControlClientRequestInfo {
    score::concurrency::InterruptiblePromise<void>
        promise_;  ///< promise that should be fulfilled, i.e. set_value(), when answer from LCM is available. SetState() and GetInitialMachineStateTransitionResult() use this type.
    std::atomic_bool
        in_use_;  ///< information whether this slot in the array is used or not. We are using atomic flag as only the code that reserves the slot will use synchronization primitive. There is no reason to protect release code, thanks to the std::atomic_flag
    bool
        initial_machine_state_transition_request_;  ///< is this a request that originated from GetInitialMachineStateTransitionResult? Due to the design of LCM, there is no place to cache those request on LCM side. As there is no limit on how many times ControlClient instance can call GetInitialMachineStateTransitionResult method, we will cache them on ControlClientImpl side.
    // Constructor to initialize all members
    ControlClientRequestInfo() : promise_(), in_use_(false), initial_machine_state_transition_request_(false) {
    }
};

/// @brief Class to encapsulate LCM implementation details of Control Client.
/// This class exist to provide ABI compatibility for ControlClient, which is an AUTOSAR defined interface.
///
/// Why this class exist?
/// * To hide (encapsulate) implementation details of ControlClient class, from users.
///     - Please note that we don't have to publish any details of communication channel (control_client_channel.hpp).
///
/// * To provide ABI compatibility for ControlClient.
///
/// * To provide access to the ControlClientChannel (IPC link to LCM daemon).
///     - The request_ link will be managed through function calls from SM and access will be protected by
///       a semaphore. When a request is successfully communicated to LCM, a promise will be stored inside
///       ControlClientImpl and a future, obtained from that promise, will be returned to the caller.
///
///     - The response_ link will be managed by background thread. This thread will read responses
///       and pass them to the corresponding promises, so they could be received through a future inside SM.
///
///     - This class is essentially a singleton, we can only have one thread that manages the response_ link.
///       For this reason factory pattern will be deployed and ControlClient instances will only hold
///       shared pointers to this class.
class ControlClientImpl final {
   public:

    ControlClientImpl() = delete;


    ControlClientImpl(
       std::function<void(const score::lcm::ExecutionErrorEvent&)> undefinedStateCallback) noexcept;

    // this class is not movable or copyable by definition
    ControlClientImpl(const ControlClientImpl&) = delete;

    ControlClientImpl& operator=(const ControlClientImpl&) = delete;

    ControlClientImpl(ControlClientImpl&& rval) = delete;

    ControlClientImpl& operator=(ControlClientImpl&& rval) = delete;

    /// @brief Method to request state transition for a single Process Group.
    ///
    /// This method will request Launch Manager to perform state transition and return immediately.
    /// Returned InterruptibleFuture can be used to determine result of requested transition.
    ///
    ///
    /// @param[in] pg_name representing meta-model definition of a specific Process Group 
    /// @param[in] pg_state representing meta-model definition of a state. Launch Manager will perform state transition from the current state to the state identified by this parameter.
    ///
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
    score::concurrency::InterruptibleFuture<void> SetState(const IdentifierHash& pg_name, const IdentifierHash& pg_state) noexcept;

    /// @brief Method to retrieve result of Machine State initial transition to Startup state.
    ///
    /// Please note that this transition happens once per machine life cycle, thus result delivered by this method shall not change (unless machine is started again).
    ///
    /// @returns void if requested transition is successful, otherwise it returns ExecErrorDomain error.
    /// @error score::lcm::ExecErrc::kCancelled if transition to the requested Process Group state was cancelled by a newer request
    /// @error score::lcm::ExecErrc::kFailed if transition to the requested Process Group state failed
    /// @error score::lcm::ExecErrc::kCommunicationError if ControlClient can't communicate with Launch Manager (e.g. IPC link is down)
    /// @error score::lcm::ExecErrc::kGeneralError if any other error occurs.
    score::concurrency::InterruptibleFuture<void> GetInitialMachineStateTransitionResult() noexcept;

    /// @brief Returns the execution error which changed the given Process Group to an Undefined Process Group State.
    ///
    /// This function will return with error and will not return an ExecutionErrorEvent object, if the given
    /// Process Group is in a defined Process Group state again.
    ///
    /// @param[in] processGroup   For which Process Group the error should be retrieved.
    ///
    /// @returns The execution error which changed the given Process Group to an Undefined Process Group State.
    /// @error score::lcm::ExecErrc::kFailed    Given Process Group is not in an Undefined Process Group State.
    /// @error score::lcm::ExecErrc::kCommunicationError if ControlClient can't communicate with Launch Manager (e.g. IPC link is down)
    score::Result<score::lcm::ExecutionErrorEvent> GetExecutionError(
        const score::lcm::IdentifierHash& processGroup) noexcept;

    ~ControlClientImpl() noexcept;
    private:
    /// @brief Flag to indicate whether an instance of ControlClient has already been created.
    /// Only one instance per process is allowed. If a second instance is created, the application
    /// will be aborted.
    static bool instance_created_;

    /// @brief Protect instance creation in multi-threaded scenarios.
    static std::mutex instance_creation_mutex_;

    /// @brief callback that ControlClient instance ask us to invoke when there is a problem with PG
    std::function<void(const score::lcm::ExecutionErrorEvent&)> undefined_state_callback_;

    /// @brief Array of active requests, that wait for completion from LCM side.
    /// When a request has been send to LCM and the answer is not immediately available,
    /// ControlClientImpl classify such a request as an active or ongoing request.
    /// A promise for such a request is stored in this array and is fulfilled when
    /// answer arrives from LCM.
    std::array<ControlClientRequestInfo,
                     static_cast<uint16_t>(etas::vrte::lcm::ControlClientLimits::kControlClientMaxRequests)>
        control_client_requests_;

    /// @brief Semaphore used to protect access to the request_ link of ControlClientChannel,
    /// as well as control_client_requests_.
    /// Access to the request_ link needs to be protected as, this link only support a single request
    /// at a time and LCM acceptance of the request needs to be retrieved.
    /// Please note that synchronization for control_client_requests_ is only needed, when we are booking a slot
    /// inside this array. When we are releasing a slot inside this array, this can be done without
    /// ipc_request_semaphore_ protection.
    etas::vrte::lcm::osal::Semaphore ipc_request_semaphore_;

    /// @brief Thread used for monitoring response_ link of ControlClientChannel.
    /// Asynchronous nature of ControlClient API means responses to ControlClient requests, will arrive at
    /// a random point in future. For this reason a background thread is needed to monitor response_ link,
    /// this way we can deliver answers when they arrive from LCM.
    std::unique_ptr<std::thread> ipc_response_thread_;

    /// @brief Synchronization variable used to manage lifetime of ipc_response_thread_
    /// As long as ipc_response_thread_running_ is set to true,
    /// the ipc_response_thread_ should stay alive and perform its job.
    /// When ipc_response_thread_running_ is set to false,
    /// the ipc_response_thread_ should finish its execution and exit ASAP.
    std::atomic_bool ipc_response_thread_running_;

    /// @brief Entry point for ipc_response_thread_
    /// This code will run in background and perform the work that is needed.
    /// Exit from this function depends on ipc_response_thread_running_ variable.
    void run();

    /// @brief Handle to the real IPC communication channel with LCM
    /// This handle is used to perform low level communication with LCM.
    etas::vrte::lcm::ControlClientChannelP ipc_channel_;

    /// @brief Helper method to send a message to LCM, through IPC link (aka request_ link).
    ///
    /// This method will check if we have empty slot in control_client_requests_ array.
    /// If yes, it will create a fresh promise for this request and send the message. The index of the slot
    /// used to store mentioned promise, is saved inside message before it is send.
    /// If no, error will be returned.
    ///
    ///@param[in, out] msg A message that should be send to LCM.
    ///                    If kControlClientMaxRequests is not reached and we have empty slot,
    ///                    this slot index is written into the msg before sending.
    ///                    Otherwise msg.originating_control_client_.future_id_ is not updated and
    ///                    error is returned.
    ///
    /// @returns score::concurrency::InterruptibleFuture<void> when message is successfully sent to LCM. This future can be
    ///                                  used to retrieve LCM response at a later time.
    /// @error score::lcm::ExecErrc::kFailed if the message could not be send to LCM
    /// @error score::lcm::ExecErrc::kCommunicationError if we can't get access to the request_ link
    ///
    /// @threadsafety{thread-safe}
    score::concurrency::InterruptibleFuture<void> SendIpcMessage(etas::vrte::lcm::ControlClientMessage& msg) noexcept;
};

}  // namespace lcm

}  // namespace score

#endif  // CONTROL_CLIENT_IMPL_H_
