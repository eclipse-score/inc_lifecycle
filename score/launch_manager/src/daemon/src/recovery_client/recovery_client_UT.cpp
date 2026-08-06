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
#include <gtest/gtest.h>

#include <vector>

#include "score/mw/launch_manager/recovery_client/recovery_client.hpp"

namespace score
{
namespace lcm
{

class RecoveryClientTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
    }
};

TEST_F(RecoveryClientTest, SendRecoveryRequestInvokesRegisteredCallback)
{
    RecordProperty(
        "Description", "RecoveryClient invokes the registered callback with the provided process identifier.");

    RecoveryClient client;
    IdentifierHash received{""};
    client.setRecoveryRequestCallback([&received](const IdentifierHash& process_identifier) {
        received = process_identifier;
    });

    const bool result = client.sendRecoveryRequest(IdentifierHash("proc_a"));
    EXPECT_TRUE(result);
    EXPECT_EQ(received, IdentifierHash("proc_a"));
}

TEST_F(RecoveryClientTest, SendRecoveryRequestReturnsFalseWithoutRegisteredCallback)
{
    RecordProperty("Description", "RecoveryClient returns false and does not crash when no callback is registered.");

    RecoveryClient client;
    EXPECT_FALSE(client.sendRecoveryRequest(IdentifierHash("proc_b")));
}

TEST_F(RecoveryClientTest, MultipleRequestsInvokeCallbackInOrder)
{
    RecordProperty(
        "Description", "RecoveryClient invokes the callback once per request in the same order requests are sent.");

    RecoveryClient client;
    std::vector<IdentifierHash> received;
    client.setRecoveryRequestCallback([&received](const IdentifierHash& process_identifier) {
        received.push_back(process_identifier);
    });

    const IdentifierHash proc_first("proc_first");
    const IdentifierHash proc_second("proc_second");
    const IdentifierHash proc_third("proc_third");

    EXPECT_TRUE(client.sendRecoveryRequest(proc_first));
    EXPECT_TRUE(client.sendRecoveryRequest(proc_second));
    EXPECT_TRUE(client.sendRecoveryRequest(proc_third));

    ASSERT_EQ(received.size(), 3U);
    EXPECT_EQ(received[0], proc_first);
    EXPECT_EQ(received[1], proc_second);
    EXPECT_EQ(received[2], proc_third);
}

TEST_F(RecoveryClientTest, ReRegisteringCallbackReplacesPreviousCallback)
{
    RecordProperty(
        "Description", "RecoveryClient uses the latest callback when setRecoveryRequestCallback is called again.");

    RecoveryClient client;
    std::size_t callback1_calls = 0U;
    std::size_t callback2_calls = 0U;

    client.setRecoveryRequestCallback([&callback1_calls](const IdentifierHash&) {
        ++callback1_calls;
    });
    ASSERT_TRUE(client.sendRecoveryRequest(IdentifierHash("proc_before_replace")));

    client.setRecoveryRequestCallback([&callback2_calls](const IdentifierHash&) {
        ++callback2_calls;
    });
    ASSERT_TRUE(client.sendRecoveryRequest(IdentifierHash("proc_after_replace")));

    EXPECT_EQ(callback1_calls, 1U);
    EXPECT_EQ(callback2_calls, 1U);
}

}  // namespace lcm
}  // namespace score
