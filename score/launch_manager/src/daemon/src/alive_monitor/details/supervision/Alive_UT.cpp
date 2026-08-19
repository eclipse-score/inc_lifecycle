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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"
#include "score/mw/launch_manager/alive_monitor/details/supervision/Alive.hpp"
#include "score/mw/launch_manager/common/constants.hpp"
#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include "score/mw/launch_manager/recovery_client/irecovery_client.h"

using namespace testing;

using EStatus = score::mw::lifecycle::internal::saf::supervision::Alive::EStatus;
using score::mw::lifecycle::internal::configuration::ComponentAliveSupervision;

namespace
{

class MockRecoveryClient : public score::mw::lifecycle::IRecoveryClient
{
  public:
    MOCK_METHOD(
        void,
        setRecoveryRequestCallback,
        (score::mw::lifecycle::IRecoveryClient::RecoveryRequestCallback callback),
        (noexcept, override));
    MOCK_METHOD(
        bool,
        sendRecoveryRequest,
        (const score::mw::lifecycle::IdentifierHash& process_group_identifier),
        (noexcept, override));
};

/// Helper: build a minimal Alive under test.
/// Owns all supporting objects so they outlive the Alive.
struct AliveFixture
{
    inline static const score::mw::lifecycle::IdentifierHash kProcessId{"42U"};

    struct Builder
    {
        uint32_t failedCyclesTolerance = 0U;
        uint32_t minIndications = 1U;
        uint32_t maxIndications = 3U;
        uint32_t reportingCycleMs = 1U;

        Builder& withFailedCyclesTolerance(uint32_t val)
        {
            failedCyclesTolerance = val;
            return *this;
        }
        Builder& withMinIndications(uint32_t val)
        {
            minIndications = val;
            return *this;
        }
        Builder& withMaxIndications(uint32_t val)
        {
            maxIndications = val;
            return *this;
        }
        Builder& withReportingCycleMs(uint32_t val)
        {
            reportingCycleMs = val;
            return *this;
        }

        [[nodiscard]] AliveFixture build() const
        {
            return AliveFixture{*this};
        }
    };

    const score::mw::lifecycle::IdentifierHash kProcessIdentifier{"test_proc"};

    std::shared_ptr<MockRecoveryClient> mockClient = std::make_shared<MockRecoveryClient>();

    score::mw::lifecycle::internal::saf::ifexm::ObservableEvent processState;
    score::mw::lifecycle::internal::saf::ifappl::Checkpoint checkpoint;

    std::unique_ptr<score::mw::lifecycle::internal::saf::supervision::Alive> alive;

    explicit AliveFixture(const Builder& bld) : processState(kProcessId), checkpoint(&processState)
    {
        ComponentAliveSupervision cfg{};
        cfg.min_indications = bld.minIndications;
        cfg.max_indications = bld.maxIndications;
        cfg.failed_cycles_tolerance = bld.failedCyclesTolerance;
        cfg.reporting_cycle_ms = bld.reportingCycleMs;

        alive = std::make_unique<score::mw::lifecycle::internal::saf::supervision::Alive>(
            kProcessIdentifier,
            cfg,
            mockClient,
            checkpoint,
            score::mw::lifecycle::internal::kDefaultAliveSupCheckpointBufferElements);
        processState.attachObserver(*alive);
    }

    /// Send an activation event and notify observers.
    void activateProcess(long ts)
    {
        processState.event.systemClockTimestamp.tv_nsec = ts;
        processState.event.eventType = score::mw::lifecycle::SupervisionEventType::kActivation;
        processState.pushData();
    }

    /// Send a deactivation event and notify observers.
    void deactivateProcess(long ts)
    {
        processState.event.systemClockTimestamp.tv_nsec = ts;
        processState.event.eventType = score::mw::lifecycle::SupervisionEventType::kDeactivation;
        processState.pushData();
    }

    /// Report one alive heartbeat checkpoint at the given timestamp.
    void reportHeartbeat(score::mw::lifecycle::internal::saf::timers::NanoSecondType timestamp)
    {
        checkpoint.pushData(timestamp);
    }
};

}  // namespace

class AliveSupervisionTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing ");
    }
};

TEST_F(AliveSupervisionTest, AliveTransitionsOkToExpiredOnMissingHeartbeat)
{
    RecordProperty(
        "Description",
        "Verify that Alive transitions from deactivated -> ok -> expired when no heartbeats are reported "
        "and failedCyclesTolerance == 0. sendRecoveryRequest must be called exactly once with the "
        "configured recovery target hash.");
    AliveFixture fix = AliveFixture::Builder{}.build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(fix.kProcessIdentifier)).Times(1).WillOnce(Return(true));

    EXPECT_EQ(fix.alive->getStatus(), EStatus::kDeactivated);

    fix.activateProcess(10U);
    fix.alive->evaluate(11U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // No heartbeats; reference cycle ends at 10 + 100000 = 1000010
    fix.alive->evaluate(1000011U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
}

TEST_F(AliveSupervisionTest, AliveStaysOkWithCorrectHeartbeats)
{
    RecordProperty(
        "Description", "Verify that sending at least minIndications heartbeats per cycle keeps Alive in ok.");
    AliveFixture fix = AliveFixture::Builder{}.build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(_)).Times(0);

    fix.activateProcess(10U);
    fix.alive->evaluate(11U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // Cycle 1: one heartbeat at t=500 (within [10, 1010]), evaluate at t=1011
    fix.reportHeartbeat(500U);
    fix.alive->evaluate(1011U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // Cycle 2: one heartbeat at t=1500 (within [1010, 2010]), evaluate at t=2011
    fix.reportHeartbeat(1500U);
    fix.alive->evaluate(2011U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);
}

TEST_F(AliveSupervisionTest, AliveReportsEnqueueFailureWhenRingBufferFull)
{
    RecordProperty(
        "Description",
        "Verify that when sendRecoveryRequest fails (ring buffer full), hasRecoveryEnqueueFailed reports true.");

    AliveFixture fix = AliveFixture::Builder{}.build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(fix.kProcessIdentifier)).Times(1).WillOnce(Return(false));

    fix.activateProcess(10U);
    fix.alive->evaluate(11U);

    EXPECT_FALSE(fix.alive->hasRecoveryEnqueueFailed());

    fix.alive->evaluate(1000011U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
    EXPECT_TRUE(fix.alive->hasRecoveryEnqueueFailed());
}

TEST_F(AliveSupervisionTest, AliveDebouncesThroughFailedBeforeExpired)
{
    RecordProperty(
        "Description",
        "Verify that failedCyclesTolerance debouncing works: with tolerance=1 the supervision passes "
        "through failed before reaching expired.");
    AliveFixture fix = AliveFixture::Builder{}.withFailedCyclesTolerance(1U).build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(fix.kProcessIdentifier))
        .Times(1)
        .WillOnce(::testing::Return(true));

    fix.activateProcess(10U);
    fix.alive->evaluate(11U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // First missed cycle: ok -> failed (tolerance not yet exceeded)
    fix.alive->evaluate(1000011U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kFailed);

    // Second missed cycle: tolerance exceeded -> expired
    fix.alive->evaluate(2000011U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
}

TEST_F(AliveSupervisionTest, DeactivatesOnSupervisionDeactivation)
{
    RecordProperty("Description", "Verify that a deactivation event deactivates the supervision from ok.");
    AliveFixture fix = AliveFixture::Builder{}.build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(_)).Times(0);

    fix.activateProcess(10U);
    fix.alive->evaluate(11U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    fix.deactivateProcess(20U);
    fix.alive->evaluate(21U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kDeactivated);
}

TEST_F(AliveSupervisionTest, ReactivatesAfterDeactivation)
{
    RecordProperty(
        "Description",
        "Verify that after a deactivation the supervision can be reactivated when an activation"
        " event is received again.");
    AliveFixture fix = AliveFixture::Builder{}.build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(_)).Times(0);

    fix.activateProcess(10U);
    fix.alive->evaluate(11U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    fix.deactivateProcess(20U);
    fix.alive->evaluate(21U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kDeactivated);

    fix.activateProcess(30U);
    fix.alive->evaluate(31U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);
}

TEST_F(AliveSupervisionTest, MaxIndicationViolationExpires)
{
    RecordProperty("Description", "Verify that exceeding the maximum allowed heartbeats per cycle leads to failure.");
    // max=1, tolerance=0: more than 1 heartbeat per cycle expires immediately
    AliveFixture fix = AliveFixture::Builder{}.withMaxIndications(1U).build();

    EXPECT_CALL(*fix.mockClient, sendRecoveryRequest(fix.kProcessIdentifier)).Times(1).WillOnce(Return(true));

    fix.activateProcess(10U);
    fix.alive->evaluate(11U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kOk);

    // Two heartbeats in one cycle violates max=1
    fix.reportHeartbeat(100U);
    fix.reportHeartbeat(200U);
    fix.alive->evaluate(1000011U);
    EXPECT_EQ(fix.alive->getStatus(), EStatus::kExpired);
}
