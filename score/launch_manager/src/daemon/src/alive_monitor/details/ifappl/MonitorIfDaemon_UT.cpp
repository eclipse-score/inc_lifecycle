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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <string>

#include "score/mw/launch_manager/alive_monitor/details/ifappl/Checkpoint.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/DataStructures.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifappl/MonitorIfDaemon.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ObservableEvent.hpp"

using namespace testing;

namespace score::mw::lifecycle::internal::saf
{

namespace
{

/// Counter used to generate unique POSIX shared-memory names across tests.
std::atomic<int> g_ipcCounter{0};

std::string makeUniqueIpcName()
{
    // The ipc_dropin::Socket prepends '/' when the name doesn't start with it.
    return "test_monifd_ipc_" + std::to_string(g_ipcCounter.fetch_add(1));
}

class CheckpointMock : public common::Observer<ifappl::Checkpoint>
{
  public:
    MOCK_METHOD(void, updateData, (const ifappl::Checkpoint&), (noexcept));
};

struct MonitorIfDaemonFixture
{
    inline static const IdentifierHash kProcessId{"test_proc"};
    static constexpr std::string_view kInterfaceName = "test_interface";

    ifexm::ObservableEvent processState;
    ifappl::Checkpoint checkpoint;
    ifappl::CheckpointIpcServer ipcServer;
    ifappl::MonitorIfDaemon monitor;
    CheckpointMock checkpointMock;

    MonitorIfDaemonFixture()
        : processState(kProcessId), checkpoint(&processState), ipcServer{}, monitor(ipcServer, kInterfaceName.data())
    {
        processState.attachObserver(monitor);
        monitor.attachCheckpoint(checkpoint);
        checkpoint.attachObserver(checkpointMock);
    }

    /// Initialize the IPC server so that peek/pop/hasOverflow use real shared memory.
    void initIpc()
    {
        ASSERT_TRUE(ipcServer.init(makeUniqueIpcName()) == ifappl::CheckpointIpcServer::EIpcInitResult::kOk)
            << "CheckpointIpcServer init failed";
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

    /// Write a single checkpoint element into the IPC ring buffer.
    void sendCheckpoint(timers::NanoSecondType ts)
    {
        ipcServer.sendEmplace(ts);
    }

    /// Fill the IPC ring buffer past its capacity to set the overflow flag.
    void fillIpcBufferToTriggerOverflow()
    {
        // Sending one element beyond capacity sets the ring-buffer overflow flag.
        for (uint32_t i = 0U; i <= ifappl::k_maxCheckpointBufferElements; ++i)
        {
            ipcServer.sendEmplace(static_cast<timers::NanoSecondType>(i));
        }
    }
};

}  // namespace

class MonitorIfDaemonTest : public ::testing::Test
{
  private:
    timespec time_{};
    static constexpr long kTimeStep = 100U;

  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
        time_.tv_nsec = 0;
    }

  public:
    /// @brief Clock that increases at fixed intervals with each call
    [[nodiscard]]
    timers::NanoSecondType mockClock()
    {
        return time_.tv_nsec += kTimeStep;
    }

    /// @brief Increase the time by @c count mockClock() calls
    timers::NanoSecondType mockClockSkip(int count)
    {
        return time_.tv_nsec += (kTimeStep * count);
    }

    /// @brief Get the current time plus an offset smaller than the tick size
    [[nodiscard]]
    timers::NanoSecondType mockClockOffset() const
    {
        return time_.tv_nsec + 50U;
    }

    /// @brief Get the time @c count mockClock() calls from now
    [[nodiscard]]
    timers::NanoSecondType mockClockFuture(int count) const
    {
        return time_.tv_nsec + (kTimeStep * count);
    }
};

TEST_F(MonitorIfDaemonTest, GetInterfaceName_ReturnsNameGivenAtConstruction)
{
    RecordProperty("Description", "Verify that getInterfaceName() returns the string supplied to the constructor.");

    MonitorIfDaemonFixture fix;
    EXPECT_EQ(fix.monitor.getInterfaceName(), MonitorIfDaemonFixture::kInterfaceName);
}

TEST_F(MonitorIfDaemonTest, InitiallyInactive_CheckForNewData_DoesNotNotifyCheckpoint)
{
    RecordProperty(
        "Description",
        "Before any supervision event the monitor is kInactive; "
        "checkForNewData must not forward any data.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(Exactly(0));
    fix.monitor.checkForNewData(mockClock());
}

TEST_F(MonitorIfDaemonTest, DeactivationBeforeActivation_RemainsInactive)
{
    RecordProperty(
        "Description",
        "A deactivation event before the monitor has been activated "
        "must not cause checkForNewData to read IPC data.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(Exactly(0));
    fix.initIpc();
    fix.deactivateProcess(mockClock());
    fix.monitor.checkForNewData(mockClock());

    fix.sendCheckpoint(mockClockOffset());
    fix.monitor.checkForNewData(mockClock());
}

TEST_F(MonitorIfDaemonTest, ActivationEvent_ActivatesMonitorOnNextCheckForNewData)
{
    RecordProperty(
        "Description",
        "An activation event must set isActivateRequest so that "
        "the next checkForNewData transitions the monitor to kActive.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(1);
    fix.initIpc();
    fix.activateProcess(mockClock());
    const auto checkpoint_time = mockClockOffset();
    fix.sendCheckpoint(checkpoint_time);
    fix.monitor.checkForNewData(mockClock());  // activates AND reads in the same call

    EXPECT_EQ(fix.checkpoint.getTimestamp(), checkpoint_time);
}

TEST_F(MonitorIfDaemonTest, DeactivationEvent_DeactivatesMonitor_NoFurtherDataForwarded)
{
    RecordProperty(
        "Description",
        "After a deactivation event checkForNewData drains the IPC for the "
        "current cycle, then transitions to kInactive.  Subsequent cycles "
        "must not forward data even when the IPC buffer is non-empty.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(Exactly(0));
    fix.initIpc();
    fix.activateProcess(mockClock());
    fix.monitor.checkForNewData(mockClock());  // now kActive

    fix.deactivateProcess(mockClock());
    fix.monitor.checkForNewData(mockClock());  // reads remaining data, then -> kInactive

    // Data written AFTER the deactivation cycle must not reach the checkpoint.
    fix.sendCheckpoint(mockClockOffset());
    fix.monitor.checkForNewData(mockClock());  // kInactive, nothing read
}

TEST_F(MonitorIfDaemonTest, Active_CheckpointDataForwarded)
{
    RecordProperty(
        "Description",
        "When active, an IPC element whose timestamp is within the current "
        "sync window must be forwarded to the matching checkpoint observer.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(1);
    fix.initIpc();
    fix.activateProcess(mockClock());
    const auto checkpoint_time = mockClock();
    fix.sendCheckpoint(checkpoint_time);
    fix.monitor.checkForNewData(mockClock());

    EXPECT_EQ(fix.checkpoint.getTimestamp(), checkpoint_time);
}

TEST_F(MonitorIfDaemonTest, Active_FutureTimestamp_NotForwardedInCurrentCycle)
{
    RecordProperty(
        "Description",
        "An IPC element whose timestamp exceeds the syncTimestamp must be "
        "left in the buffer and not forwarded during that cycle.");
    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(Exactly(0));
    fix.initIpc();
    fix.activateProcess(mockClock());
    fix.sendCheckpoint(mockClockFuture(5));
    fix.monitor.checkForNewData(mockClock());  // future checkpoint not consumed
}

TEST_F(MonitorIfDaemonTest, Active_FutureTimestampCheckpoint_ConsumedInLaterCycle)
{
    RecordProperty(
        "Description",
        "An element held back because its timestamp was in the future must "
        "be consumed and forwarded when the sync window catches up.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(1);
    fix.initIpc();
    fix.activateProcess(mockClock());
    const auto future_time = mockClockFuture(2);
    fix.sendCheckpoint(future_time);
    fix.monitor.checkForNewData(mockClock());  // not consumed yet
    mockClockSkip(2);
    fix.monitor.checkForNewData(mockClock());  // now within window -> consumed
    EXPECT_EQ(fix.checkpoint.getTimestamp(), future_time);
}

TEST_F(MonitorIfDaemonTest, Active_MultipleCheckpointsInOneCycle_AllForwarded)
{
    RecordProperty(
        "Description",
        "All IPC elements whose timestamps fall within the sync window must "
        "be forwarded in a single checkForNewData call.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(3);
    fix.initIpc();
    fix.activateProcess(mockClock());
    fix.sendCheckpoint(mockClock());
    fix.sendCheckpoint(mockClock());
    fix.sendCheckpoint(mockClock());
    fix.monitor.checkForNewData(mockClock());
}

TEST_F(MonitorIfDaemonTest, Active_OverflowDetected_TransitionsToInactiveOverflow)
{
    RecordProperty(
        "Description",
        "When hasOverflow() is true while in kActive, handleOverflow must be "
        "called: the checkpoint observer is notified with a data-loss event "
        "and the monitor transitions to kInactiveOverflow.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(1);  // one overflow notification
    fix.initIpc();
    fix.activateProcess(mockClock());
    fix.monitor.checkForNewData(mockClock());  // -> kActive

    fix.fillIpcBufferToTriggerOverflow();
    fix.monitor.checkForNewData(mockClock());  // overflow detected -> kInactiveOverflow

    EXPECT_TRUE(fix.checkpoint.getDataLossEvent());
}

TEST_F(MonitorIfDaemonTest, InactiveOverflow_NoProcessRestart_DoesNotRepeatNotification)
{
    RecordProperty(
        "Description",
        "While in kInactiveOverflow, repeated checkForNewData calls without "
        "a process restart must not push additional overflow notifications.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(Exactly(1));
    fix.initIpc();
    fix.activateProcess(mockClock());
    fix.monitor.checkForNewData(mockClock());
    fix.fillIpcBufferToTriggerOverflow();
    fix.monitor.checkForNewData(mockClock());  // -> kInactiveOverflow, mock notified once
    fix.monitor.checkForNewData(mockClock());  // still kInactiveOverflow, no restart
    fix.monitor.checkForNewData(mockClock());
}

TEST_F(MonitorIfDaemonTest, InactiveOverflow_ProcessRestarts_PushesOverflowNotificationAgain)
{
    RecordProperty(
        "Description",
        "When the supervised process restarts while the monitor is in "
        "kInactiveOverflow, the observers must be notified again so that "
        "supervisions remain aware the shared memory is still corrupted.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(Exactly(2));
    fix.initIpc();
    fix.activateProcess(mockClock());
    fix.monitor.checkForNewData(mockClock());
    fix.fillIpcBufferToTriggerOverflow();
    fix.monitor.checkForNewData(mockClock());  // -> kInactiveOverflow, mock notified once

    // Simulate: process goes off, then comes back.
    fix.deactivateProcess(mockClock());        // sets isDeactivateRequest
    fix.activateProcess(mockClock());          // isProcessRestarted = true
    fix.monitor.checkForNewData(mockClock());  // still kInactiveOverflow -> push again
}

TEST_F(MonitorIfDaemonTest, InactiveOverflow_ProcessRestartFlag_ClearedAfterNotification)
{
    RecordProperty(
        "Description",
        "isProcessRestarted must be cleared after the overflow notification "
        "is re-sent, so that a further checkForNewData without a new restart "
        "does not produce another notification.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(Exactly(2));
    fix.initIpc();
    fix.activateProcess(mockClock());
    fix.monitor.checkForNewData(mockClock());
    fix.fillIpcBufferToTriggerOverflow();
    fix.monitor.checkForNewData(mockClock());

    fix.deactivateProcess(mockClock());
    fix.activateProcess(mockClock());
    fix.monitor.checkForNewData(mockClock());  // restart handled, mock notified twice total

    fix.monitor.checkForNewData(mockClock());  // no new restart -> no additional notification
}

}  // namespace score::mw::lifecycle::internal::saf
