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

#ifndef NOTIFICATION_HPP_INCLUDED
#define NOTIFICATION_HPP_INCLUDED

#ifndef PHM_PRIVATE
#    define PHM_PRIVATE private
#endif

#include <cstdint>
#include <optional>
#include <memory>

#include <array>
#include <string>
#include "etas/vrte/saf/logging/PhmLogger.hpp"
#include "etas/vrte/saf/timers/TimeConversion.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"
#include <score/lcm/control_client.h>

namespace xaap
{
namespace vrte
{
namespace saf
{
namespace recovery
{
namespace supervision
{
using SupervisionErrorInfo = struct SupervisionErrorInfo
{
    uint32_t failedProcessExecutionError;
    uint32_t failedSupervisionType;
    std::array<uint8_t, 1024> failedprocessGroupMetaModelIdentifier;
};

}  // namespace supervision
}  // namespace recovery
}  // namespace saf
}  // namespace vrte
}  // namespace xaap

namespace score
{
namespace lcm
{
enum class TypeOfSupervision : std::uint32_t
{
    /// @brief Supervision is of type AliveSupervision
    AliveSupervision = 0,
    /// @brief Supervision is of type DeadlineSupervision
    DeadlineSupervision = 1,
    /// @brief Supervision is of type LogicalSupervision
    LogicalSupervision = 2
};

}  // namespace lcm
}  // namespace score

namespace etas
{
namespace vrte
{
namespace saf
{
namespace recovery
{

/* RULECHECKER_comment(0, 14, check_non_private_non_pod_field, "NotificationConfig data is\
 set to public scope", true_no_defect) */
/// @brief Structure containing configuration data for Recovery notification
class NotificationConfig final
{
public:
    /// @brief Name of the RecoveryNotification configuration container
    std::string configName{""};
    /// @brief Instance specifier metamodel path of the PPortPrototype of Phm RecoveryActionInterface
    std::string serviceInstanceSpecifierPath{""};
    /// @brief Process Group Meta Model identifier of the process that is responsible for the monitoring event
    std::string processGroupMetaModelIdentifier{""};
    /// @brief Maximum acceptable amount of time to wait for an acknowledgment after sending the notification
    timers::NanoSecondType timeout{UINT64_MAX};
};

/// @brief Notification class which initiates the recovery action.
/// The recovery action could be a notification to State Management, or directly setting the final timeout state
/// which would eventually trigger a reaction via the watchdog
class Notification
{
public:
    /// @brief Default constructor
    /// @param [in] f_recoveryClient_r Reference to the recovery interface of launch manager
    /// @warning If the default constructor is used for the construction of Notification class, the notification
    /// instance will not send the recovery notification to State Management, instead it will directly set the
    /// final timeout state so that the recovery can be handled via watchdog
    Notification(score::lcm::ControlClient& f_recoveryClient_r);

    /// @brief Parametric constructor
    /// @param [in] f_notificationConfig_r Reference to the configuration structure of Notification class
    /// @param [in] f_recoveryClient_r Reference to the recovery interface of launch manager
    /// @note If this parametric constructor is used for the construction of Notification class, the notification
    /// instance will send the recovery notification to State Management
    explicit Notification(const NotificationConfig& f_notificationConfig_r, score::lcm::ControlClient& f_recoveryClient_r);

    /// @brief Copy constructor for this class is not supported
    Notification(const Notification&) = delete;

    /// @brief Move assignment operator for this class is not supported
    Notification& operator=(Notification&&) & = delete;

    /// @brief Move constructor
    Notification(Notification&&);

    /// @brief Copy assignment operator for this class is not supported
    Notification& operator=(const Notification&) & = delete;

    /// @brief Default destructor
    virtual ~Notification();

    /// @brief Start initialization of proxy instance
    /// @details This invokes StartFindService which will continue to asynchronously look for the proxy instance.
    /// @returns True if StartFindService succeeds, False in case of error
    bool initProxy() noexcept(false);

    /// @brief Send method to trigger sending the notification
    /// This method sends a notification to State Management if this class was constructed with the configuration
    /// data for RecoveryNotification. Otherwise, this method sets the timeout state such that a reaction can then
    /// be triggered via the watchdog
    /// @param [in] f_executionErrorInfo_r Reference to the process error information
    ///                                    (supervision) which should be sent to State
    ///                                    Management
    /// @note The process execution error is used only if this class was constructed with the configuration
    /// data for RecoveryNotification, otherwise it is ignored.
    void send(const xaap::vrte::saf::recovery::supervision::SupervisionErrorInfo& f_executionErrorInfo_r);

    /// @brief Send the notifications to State Management (if required), in every cycle of PHM daemon
    /// @note This is done by invoking the Recovery handler of State Management and verifying the feedback
    /// from the Recovery handler methods
    /// @warning The current implementation doesn't support parallel calls to the RecoveryHandler
    /// remote procedure (i.e., if the previous call to the RecoveryHandler is pending, any subsequent calls to
    /// the same remote method will also be in pending state (will be queued)).
    void cyclicTrigger(void);

    /// @brief Method to check if the final timeout state of the recovery notification is reached
    /// @return Boolean flag to indicate if the final timeout has occurred
    ///         (true: Final timeout occurred, false: otherwise)
    bool isFinalTimeoutStateReached(void) const noexcept;

    /// @brief Read the configuration name of the RecoveryNotification configuration container
    /// @details Read the configuration name of the RecoveryNotification
    /// @return A String object   Configuration name of the RecoveryNotification as text string which was read
    const std::string& getConfigName(void) const noexcept;

    PHM_PRIVATE:
    /// @brief Internal states of the Notification
    enum class State : std::uint8_t
    {
        kIdle,               //< Not Sending or waiting for any response
        kTimeout,            //< Final timeout reached
        kSending,            //< Should invoke the RecoveryHandler next
        kWaitingForResponse  //< Waiting for feedback from STM
    };

    /// @brief The current internal state
    State currentState;

    /// @brief Method to invoke the call to the recovery handler method of State Management
    void invokeRecoveryHandler(void);

    /// @brief Method to verify the response from the RecoveryHandler method of State Management
    /// @note This method is responsible to verify if the call to recovery handler of State Management has executed
    /// within the configured timeout
    void verifyRecoveryHandlerResponse(void);

    /// @brief Set the final timeout state for the Recovery notification
    void setFinalTimeoutState(void);

    /// @brief Instance of the structure containing the configuration data pertaining to the Recovery notification
    const NotificationConfig k_notificationConfig{};

    /// @brief Message header used for logging
    const std::string messageHeader;

    /// @brief Start timestamp for evaluation of recovery notification timeout
    timers::NanoSecondType startTimestamp{0U};

    /// @brief Boolean flag to indicate if this class was constructed with the recovery notification configuration
    /// data (true: this class is to be used to trigger notifications to State Management, false: this class is to be
    /// used to directly set the final timeout state such that the recovery is performed via watchdog)
    bool isNotificationConfigAvailable{false};

    score::concurrency::InterruptibleFuture<void> recoveryStateFutureOutput;
    score::lcm::IdentifierHash recoveryProcessGroup;
    score::lcm::IdentifierHash recoveryProcessGroupState;
    score::lcm::ControlClient& recoveryClient;

    /// @brief Logger
    logging::PhmLogger& logger_r;
};

}  // namespace recovery
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
