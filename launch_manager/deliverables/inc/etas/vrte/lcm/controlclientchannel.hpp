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

#ifndef CONTROL_CLIENT_CHANNEL_HPP_INCLUDED
#define CONTROL_CLIENT_CHANNEL_HPP_INCLUDED

#include <atomic>
#include <etas/vrte/lcm/process_group_state_id.hpp>
#include <etas/vrte/lcm/osal/osalipccomms.hpp>
#include <condition_variable>
#include <mutex>

namespace etas {

namespace vrte {

namespace lcm {

/// @brief This is initially some ID provided by the Control Client library. When received
/// by Control Client handler additional information is added - the state manager
/// process originating the request. This can be given in the form of a function
/// group index and process index.
/// When the Control Client library receives a response, it must be able to extract
/// the client ID, ignoring the state manager process identification.
struct ControlClientID final {
    uint16_t process_group_index_;  ///< Process group containing the state manager process
    uint16_t process_index_;         ///< The process within the process group
    uint32_t future_id_;             ///< ID to match request and response
    ControlClientID() : process_group_index_(0), process_index_(0), future_id_(0) {
    }  ///< For use by Control Client
};

/// @brief Code for requests from Control Client and responses back from Launch Manager
enum class ControlClientCode  // Both request and response codes are given. Mapping to AUTOSAR is done at client side
{
    // General
    kNotSet = 0,          ///< This code is used to initialise variables
    kInvalidRequest = 1,  ///< Response back to Control Client if Launch Manager gets a code it does not recognise

    // setState functionality
    kSetStateRequest = 16,           ///< setState request code
    kSetStateInvalidArguments = 17,  ///<  Response: invalid arguments response for set state request
    kSetStateCancelled = 18,         ///< Response: setState request was cancelled by a newer request
    kSetStateFailed = 19,            ///< Response: setState request failed (PG in undefined state)
    kSetStateSuccess = 20,           ///< Response: setState request succeeded (PG in new state)
    kSetStateAlreadyInState = 21,    ///< Response: setState had no effect because PG was already in requested state
    kSetStateTransitionToSameState =
        22,  ///< Response: setState had no effect because PG already in transition to new state

    // Responses resulting from unexpected termination (when do we report these?)
    kFailedUnexpectedTerminationOnEnter =
        23,  ///< Response: Unexpected Termination of a process during transition to new process group state
    kFailedUnexpectedTermination = 24,  ///< Response: termination of a process when not in transition

    // getInitialMachineState functionality
    kGetInitialMachineStateRequest      = 32, ///< Request the initial machine state result
    kInitialMachineStateNotSet          = 33, ///< Internal value used before first state transition or there is no machine PG
    kInitialMachineStateFailed          = 34, ///<  Response: The transition to the initial machine state failed (or was cancelled)
    kInitialMachineStateSuccess         = 35, ///< Response: The initial machine state transition was successful

    // getExecutionError functionality
    kGetExecutionErrorRequest = 48,        ///< Request the execution error for a process group
    kExecutionErrorInvalidArguments = 49,  ///< Response: Process group does not exist
    kExecutionErrorRequestFailed = 50,     ///< Response: The process group is in a defined state
    kExecutionErrorRequestSuccess = 51,    ///< Response: Execution error reported for the given process group

    // validateProcessGroupState
    kValidateProcessGroupState = 64,        ///< Request to validate the process group state ID
    kValidateProcessGroupStateFailed = 65,  ///< Response: Process group state ID w/as invalid
    kValidateProcessGroupStateSuccess =
        66  ///< Response: Both process group name and process group state name were valid
};

/// @brief A message that can be a request, and acknowledgement or a response
struct ControlClientMessage final {
    ControlClientID originating_control_client_;  ///< ID of the individual Control Client and state manager process
    ControlClientCode request_or_response_;     ///< Request code (SM -> LM) or acknowledgement/response code (LM -> SM)
    ProcessGroupStateID process_group_state_;  ///< Payload for most requests & responses
    uint32_t
        execution_error_code_;  ///< Additional payload for `kExecutionErrorRequestSuccess` and `kFailedUnexpectedTermination`
    // Constructor to initialize all data members
    ControlClientMessage()
        : originating_control_client_(),
          request_or_response_(ControlClientCode::kNotSet),
          process_group_state_(),
          execution_error_code_(0) {
    }
};

/// @brief Represents a mapping between a ControlClientCode and its corresponding description string.
// RULECHECKER_comment(1, 1, check_incomplete_data_member_construction, "wi 45913 - This struct is POD, which doesn't have user-declared constructor. The rule doesn’t apply.", false)
struct ControlClientCodeMapping {
    ControlClientCode code;
    const char* description;
};

/// @brief Communications channel used for requests and responses
struct ControlClientComms final {
    std::atomic_bool empty_;  ///< true when a message can be placed, false when one may be read
    ControlClientMessage msg_;  ///< The message to be sent
    // Constructor to initialize all data members
    ControlClientComms() : empty_(true), msg_() {
    }
};

struct ControlClientChannel;
using ControlClientChannelP = std::shared_ptr<ControlClientChannel>;

/// @brief The bidirectional communications channel between a state manager and the Launch Manager
/// @note A Control Client message contains both the message from a Control Client to
/// Launch Manager and the response back from Launch Manager to the
/// Control Client.
/// Details about the originating Control Client and state manager are filled
/// in as they become available. The state manager is responsible for the
/// Control Client details; these are just fields Launch Manager will record
/// and send back with responses. Launch Manager fills in the details about
/// the state manager, i.e., which process of which process group originated
/// a request.
/// The Control Client handler is responsible for directing the request to the
/// correct graph (process group). The graph stores a copy of the last
/// requesting Control Client & state manager process. This information is
/// written by the Control Client handler.
/// A response to a transition request is routed to the correct process & state
/// client by copying the information stored in the request.
/// A report of an asynchronous event (an unexpected termination resulting in an
/// undefined state of the process group) will be routed to the correct process
/// and state by copying the information stored in the graph.
///
class ControlClientChannel final {
   public:
    /// @brief Constructor, deleted. We cannot create or delete objects of this type in the normal ways.
    ControlClientChannel() = delete;

    /// @brief Copy Constructor, deleted. We cannot create or delete objects of this type in the normal ways.
    ControlClientChannel(const ControlClientChannel& other) = delete;

    /// @brief Move Constructor, deleted. We cannot create or delete objects of this type in the normal ways.
    ControlClientChannel(ControlClientChannel&& other) = delete;

    /// @brief Copy assignment operator, deleted. We cannot create or delete objects of this type in the normal ways.
    ControlClientChannel& operator=(const ControlClientChannel& other) = delete;

    /// @brief Move assignment operator, deleted. We cannot create or delete objects of this type in the normal ways.
    ControlClientChannel& operator=(const ControlClientChannel&& other) = delete;

    /// @brief Desctructor, deleted. We cannot create or delete objects of this type in the normal ways.
    ~ControlClientChannel() = delete;

    /// @brief Initialise the comms channels
    /// called when the shared memory is initially created by Launch Manager
    void initialize();

    /// @brief Deinitialize the comms channels
    /// called when the shared memory is destroyed by Launch Manager
    void deinitialize();

    /// @brief Send a request to the LM and get the response
    /// Used by the Control Client to send a request.
    /// Posts on the semaphore channel to wake Control Client handler up.
    /// Will block until the request is acknowledged.
    /// Not thread-safe; expected to be used by one thread only.
    /// Note there is no time-out, because if LM is not
    /// responding we are doomed anyway.
    /// @param msg the message to send and response to receive
    void sendRequest(ControlClientMessage& msg);

    /// @brief Poll for a request from the Control Client
    /// This is used by Launch Manager to poll for requests from Control Clients
    /// @return true if a request is available, false otherwise
    bool getRequest();

    /// @brief Acknowledge the request from Control Client
    /// Used by Launch Manager to inform Control Client that
    /// a message has been processed and the immediate response
    /// is ready
    void acknowledgeRequest();

    /// @brief Return a reference to the current request
    /// Used by Launch Manager to handle requests from state manager
    /// @return Reference of ControlClientMessage
    ControlClientMessage& request();

    /// @brief Send a response to the Control Client. Not threadsafe.
    /// @param msg the message to send. If there is no room the message is not sent
    /// @return True on success, false if message not sent
    bool sendResponse(ControlClientMessage& msg);

    /// @brief Get a response from the Launch Manager & acknowledge it
    /// This is used by State Manager to read responses
    /// @param msg Where to put (a copy of) the response message
    /// @return true if a response was available, false otherwise
    bool getResponse(ControlClientMessage& msg);

    /// @brief This static method returns a pointer to a ControlClientChannel object
    /// The object won't exist unless the comms_type_ is kControlClient
    /// Note that the ControlClientChannel object is in the shared memory after the Comms object
    /// @param fd - the file descriptor to use, defaults to Comms::sync_fd
    /// @return pointer to the corresponding ControlClientChannel, or nullptr if it does not exist
    /// @note This method is for use by the Launch Manager and the Control Client, the Control Client always uses
    /// the default parameter.
    static ControlClientChannelP initializeControlClientChannel(int fd = osal::IpcCommsSync::sync_fd, osal::IpcCommsP* mem_ptr = nullptr);

    /// @brief This static method returns a pointer to a ControlClientChannel object
    /// @param sync a Shared pointer to an existing Comms object
    /// Note that the ControlClientChannel object is in the shared memory after the Comms object
    /// Note also that we take a copy of the pointer to the base comms object.
    /// @return pointer to the corresponding ControlClientChannel, or nullptr if it does not exist
    /// @note this method is for use by the Launch Manager only!
    static ControlClientChannelP getControlClientChannel(osal::IpcCommsP sync);

    /// @brief post on the global shared semaphore to notify the Control Client handler of an action
    static void nudgeControlClientHandler();

    /// @brief post on the shared sempahore for this state manager to nudge the LM handler thread
    void nudgeLMHandler();

    /// @brief Release ownership of the parent comms mapping stored inside the channel
    /// @details The ControlClientChannel lives in shared memory and its destructor is never run, so any
    /// std::shared_ptr members would leak their control blocks unless explicitly cleared. This call
    /// drops the internal reference to the IpcCommsSync mapping allowing it to be unmapped when
    /// external owners release theirs.
    void releaseParentMapping();

    /// @brief semaphore pointer for nudging the Control Client handler (for LM only)
    static osal::Semaphore* nudgeControlClientHandler_;

    /// @brief Requests (SM -> LM) appear here, i.e. for communication started by SM
    ControlClientComms request_;

    /// @brief Responses (LM -> SM) are put here, i.e. for communication started by LM
    ControlClientComms response_;

    /// @brief Count of requests to obtain the initial state transition result
    uint16_t initial_result_count_;

    /// @brief A utility function that converts codes to strings for logging purposes
    /// @param code The code to convert
    /// @return A string representing the code
    const char* toString(ControlClientCode code);

   private:

    /// @brief Ensure that the ControlClientChannel was setup properly before
    /// accessing it
    static bool is_initialized_;
    static std::condition_variable init_cv_;
    static std::mutex init_mutex_;

    /// @brief Copy of parent pointer when created from an `IpcCommsSync` object
    /// @details When a `ControlClientChannel` is created by Launch Manager, we do so by using
    /// the already mapped memory, since we've close the file descriptor at this point. We need to
    /// take a copy of the pointer to make sure that the memory is not unmapped.
    osal::IpcCommsP ipc_parent_;

    /// @brief Semaphore to nudge the LM handler thread in each state manager
    osal::Semaphore nudge_LM_Handler_;
};
/// @brief Define a constexpr array of ControlClientCodeMapping structures
/// Each element in the array maps a ControlClientCode to its corresponding description
// coverity[autosar_cpp14_m3_4_1_violation:INTENTIONAL] The value is used in a global context.
constexpr ControlClientCodeMapping stateArray[] = {
    {ControlClientCode::kNotSet, "kNotSet"},
    {ControlClientCode::kInvalidRequest, "kInvalidRequest"},
    {ControlClientCode::kSetStateRequest, "kSetStateRequest"},
    {ControlClientCode::kSetStateInvalidArguments, "kSetStateInvalidArguments"},
    {ControlClientCode::kSetStateCancelled, "kSetStateCancelled"},
    {ControlClientCode::kSetStateFailed, "kSetStateFailed"},
    {ControlClientCode::kSetStateSuccess, "kSetStateSuccess"},
    {ControlClientCode::kSetStateAlreadyInState, "kSetStateAlreadyInState"},
    {ControlClientCode::kSetStateTransitionToSameState, "kSetStateTransitionToSameState"},
    {ControlClientCode::kFailedUnexpectedTerminationOnEnter, "kFailedUnexpectedTerminationOnEnter"},
    {ControlClientCode::kFailedUnexpectedTermination, "kFailedUnexpectedTermination"},
    {ControlClientCode::kGetInitialMachineStateRequest, "kGetInitialMachineStateRequest"},
    {ControlClientCode::kInitialMachineStateNotSet, "kInitialMachineStateNotSet"},
    {ControlClientCode::kInitialMachineStateFailed, "kInitialMachineStateFailed"},
    {ControlClientCode::kInitialMachineStateSuccess, "kInitialMachineStateSuccess"},
    {ControlClientCode::kGetExecutionErrorRequest, "kGetExecutionErrorRequest"},
    {ControlClientCode::kExecutionErrorInvalidArguments, "kExecutionErrorInvalidArguments"},
    {ControlClientCode::kExecutionErrorRequestFailed, "kExecutionErrorRequestFailed"},
    {ControlClientCode::kExecutionErrorRequestSuccess, "kExecutionErrorRequestSuccess"},
    {ControlClientCode::kValidateProcessGroupState, "kValidateProcessGroupState"},
    {ControlClientCode::kValidateProcessGroupStateFailed, "kValidateProcessGroupStateFailed"},
    {ControlClientCode::kValidateProcessGroupStateSuccess, "kValidateProcessGroupStateSuccess"}};

}  // namespace lcm

}  // namespace vrte

}  // namespace etas

#endif  //CONTROL_CLIENT_CHANNEL_HPP_INCLUDED
