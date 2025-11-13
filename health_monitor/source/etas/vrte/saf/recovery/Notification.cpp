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

#include "etas/vrte/saf/recovery/Notification.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace recovery
{

namespace {
    template<typename Future>
    bool is_ready(const Future& future) {
        return future.WaitFor(score::cpp::stop_token{}, std::chrono::seconds(0)).has_value();
    }
}

Notification::Notification(score::lcm::ControlClient& f_recoveryClient_r) :
    currentState(State::kIdle),
    messageHeader("Notification ( / )"),
    isNotificationConfigAvailable(false),
    recoveryClient(f_recoveryClient_r),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::recovery))
{
}

Notification::Notification(const NotificationConfig& f_notificationConfig_r, score::lcm::ControlClient& f_recoveryClient_r) :
    currentState(State::kIdle),
    k_notificationConfig(f_notificationConfig_r),
    messageHeader("Notification (" + k_notificationConfig.configName + ")"),
    isNotificationConfigAvailable(true),
    recoveryClient(f_recoveryClient_r),
    logger_r(logging::PhmLogger::getLogger(logging::PhmLogger::EContext::recovery))
{
}

// explicit declaration of destructor needed to make forward declaration work
Notification::~Notification() = default;

/* RULECHECKER_comment(0, 7, check_min_instructions, "Default move constructor is not provided\
a function body", true_no_defect) */
/* RULECHECKER_comment(0, 5, check_incomplete_data_member_construction, "Default constructor is not provided\
the member initializer", false) */
/* RULECHECKER_comment(0, 3, check_copy_in_move_constructor, "The default move constructor invokes parameterized\
constructor internally. This invokes std::string copy construction", true_no_defect) */
Notification::Notification(Notification&&) = default;

bool Notification::initProxy() noexcept(false)
{
    const auto processGroupStateStartPos = k_notificationConfig.processGroupMetaModelIdentifier.find_last_of('/');
    if(processGroupStateStartPos == std::string::npos || processGroupStateStartPos == 0U)
    {
        logger_r.LogError() << messageHeader << "Invalid ProcessGroupState identifier:"
                            << k_notificationConfig.processGroupMetaModelIdentifier;

        return false;
    }

    const std::string recoveryProcessGroupId{k_notificationConfig.processGroupMetaModelIdentifier.substr(0, processGroupStateStartPos)};
    const std::string recoveryProcessGroupStateId{k_notificationConfig.processGroupMetaModelIdentifier};
    


    recoveryProcessGroup = score::lcm::IdentifierHash(recoveryProcessGroupId);
    recoveryProcessGroupState = score::lcm::IdentifierHash(recoveryProcessGroupStateId);
    return true;
}

void Notification::send(const xaap::vrte::saf::recovery::supervision::SupervisionErrorInfo& f_executionErrorInfo_r)
{
    if (isNotificationConfigAvailable)
    {
        if (currentState == State::kIdle)
        {
            currentState = State::kSending;
        }
    }
    else
    {
        setFinalTimeoutState();
    }
}

void Notification::cyclicTrigger(void)
{
    if (currentState == State::kSending)
    {
        invokeRecoveryHandler();
    }
    if (currentState == State::kWaitingForResponse)
    {
        verifyRecoveryHandlerResponse();
    }
}

bool Notification::isFinalTimeoutStateReached(void) const noexcept
{
    return (currentState == State::kTimeout);
}

void Notification::invokeRecoveryHandler(void)
{
    recoveryStateFutureOutput = recoveryClient.SetState(recoveryProcessGroup, recoveryProcessGroupState);

    startTimestamp = timers::OsClock::getMonotonicSystemClock();

    logger_r.LogInfo() << messageHeader << "Recovery state" << k_notificationConfig.processGroupMetaModelIdentifier << "requested, timestamp:"
                        << static_cast<int>(timers::TimeConversion::convertNanoSecToMilliSec(startTimestamp))
                        << "[msec]";

    currentState = State::kWaitingForResponse;
}

void Notification::verifyRecoveryHandlerResponse(void)
{
    if(!recoveryStateFutureOutput.Valid()) {
        logger_r.LogDebug() << messageHeader << "The future result of the Recovery has invalid state";

        startTimestamp = 0U;
        setFinalTimeoutState();
        return;
    }

    if(!is_ready(recoveryStateFutureOutput))
    {
        timers::NanoSecondType endTimeStamp{timers::OsClock::getMonotonicSystemClock()};
        timers::NanoSecondType lapsedTime{endTimeStamp - startTimestamp};

        if (lapsedTime > k_notificationConfig.timeout)
        {
            logger_r.LogDebug() << messageHeader << "Timeout occurred for the requested Recovery state. End timestamp:"
                                << static_cast<int>(timers::TimeConversion::convertNanoSecToMilliSec(endTimeStamp))
                                << "[msec]";

            startTimestamp = 0U;
            setFinalTimeoutState();
        }
        return;
    }

    const auto result = recoveryStateFutureOutput.Get(score::cpp::stop_token{});
    if (!result.has_value())
    {
        logger_r.LogWarn() << messageHeader << "Recovery state request returned with error:" << result.error().Message();
        logger_r.LogDebug() << messageHeader << "Incorrect response received from the Recovery state request call";

        startTimestamp = 0U;
        setFinalTimeoutState();
        return;
    }

    logger_r.LogDebug() << messageHeader << "Correct response received from the Recovery state request call";
    startTimestamp = 0U;
    currentState = State::kIdle;
}

void Notification::setFinalTimeoutState(void)
{
    logger_r.LogWarn() << messageHeader << "Final timeout state reached for the RecoveryHandler";

    currentState = State::kTimeout;
}

/* RULECHECKER_comment(1:0,2:0, check_min_instructions, "False positive", false) */
/* RULECHECKER_comment(1:0,1:0, check_member_function_missing_static, "Member function uses non-static members and\
cannot be made static", false) */
const std::string& Notification::getConfigName(void) const noexcept
{
    return k_notificationConfig.configName;
}

}  // namespace recovery
}  // namespace saf
}  // namespace vrte
}  // namespace etas
