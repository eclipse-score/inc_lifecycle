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
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessCfg.hpp"
#include "score/mw/launch_manager/alive_monitor/details/ifexm/ProcessState.hpp"

using namespace testing;

namespace score::lcm::saf
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
    static constexpr std::string_view kProcessName = "test_proc";
    static constexpr std::string_view kCheckpointName = "test_cp";
    static constexpr uint32_t kCheckpointId = 1U;
    static constexpr common::ProcessId kProcessId = 42U;
    static constexpr std::string_view kInterfaceName = "test_interface";

    ifexm::ProcessState processState;
    ifappl::Checkpoint checkpoint;
    ifappl::CheckpointIpcServer ipcServer;
    ifappl::MonitorIfDaemon monitor;
    CheckpointMock checkpointMock;

    MonitorIfDaemonFixture()
        : processState(makeProcessCfg()),
          checkpoint(kCheckpointName.data(), kCheckpointId, &processState),
          ipcServer{},
          monitor(ipcServer, kInterfaceName.data())
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

    /// Drive the process to the 'running' state and notify observers.
    void activateProcess(timers::NanoSecondType ts)
    {
        processState.setTimestamp(ts);
        processState.setState(ifexm::ProcessState::EProcState::running);
        processState.pushData();
    }

    /// Drive the process to the 'starting' state and notify observers.
    void startProcess(timers::NanoSecondType ts)
    {
        processState.setTimestamp(ts);
        processState.setState(ifexm::ProcessState::EProcState::starting);
        processState.pushData();
    }

    /// Drive the process to the 'off' state and notify observers.
    void deactivateProcess(timers::NanoSecondType ts)
    {
        processState.setTimestamp(ts);
        processState.setState(ifexm::ProcessState::EProcState::off);
        processState.pushData();
    }

    /// Write a single checkpoint element into the IPC ring buffer.
    void sendCheckpoint(uint32_t id, timers::NanoSecondType ts)
    {
        ipcServer.sendEmplace(ts, id);
    }

    /// Fill the IPC ring buffer past its capacity to set the overflow flag.
    void fillIpcBufferToTriggerOverflow()
    {
        // Sending one element beyond capacity sets the ring-buffer overflow flag.
        for (uint32_t i = 0U; i <= ifappl::k_maxCheckpointBufferElements; ++i)
        {
            ipcServer.sendEmplace(static_cast<timers::NanoSecondType>(i), 0U);
        }
    }

  private:
    static ifexm::ProcessCfg makeProcessCfg()
    {
        ifexm::ProcessCfg cfg{};
        cfg.processShortName = kProcessName;
        cfg.processId = kProcessId;
        return cfg;
    }
};

}  // namespace

class MonitorIfDaemonTest : public ::testing::Test
{
  private:
    timers::NanoSecondType time_ = 0U;
    static constexpr timers::NanoSecondType kTimeStep = 100U;

  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
        time_ = 0;
    }

  public:
    /// @brief Clock that increases at fixed intervals with each call
    [[nodiscard]]
    timers::NanoSecondType mockClock()
    {
        return time_ += kTimeStep;
    }

    /// @brief Increase the time by @c count mockClock() calls
    timers::NanoSecondType mockClockSkip(int count)
    {
        return time_ += (kTimeStep * count);
    }

    /// @brief Get the current time plus an offset smaller than the tick size
    [[nodiscard]]
    timers::NanoSecondType mockClockOffset() const
    {
        return time_ + 50U;
    }

    /// @brief Get the time @c count mockClock() calls from now
    [[nodiscard]]
    timers::NanoSecondType mockClockFuture(int count) const
    {
        return time_ + (kTimeStep * count);
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
        "Before any process-state update the monitor is kInactive; "
        "checkForNewData must not forward any data.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(Exactly(0));
    fix.monitor.checkForNewData(mockClock());
}

TEST_F(MonitorIfDaemonTest, ProcessOffBeforeActivation_RemainsInactive)
{
    RecordProperty(
        "Description",
        "A process-off event before the monitor has been activated "
        "must not cause checkForNewData to read IPC data.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(Exactly(0));
    fix.initIpc();
    fix.deactivateProcess(mockClock());
    fix.monitor.checkForNewData(mockClock());

    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, mockClockOffset());
    fix.monitor.checkForNewData(mockClock());
}

TEST_F(MonitorIfDaemonTest, ProcessRunning_ActivatesMonitorOnNextCheckForNewData)
{
    RecordProperty(
        "Description",
        "A running process-state update must set isActivateRequest so that "
        "the next checkForNewData transitions the monitor to kActive.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(1);
    fix.initIpc();
    fix.activateProcess(mockClock());
    const auto checkpoint_time = mockClockOffset();
    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, checkpoint_time);
    fix.monitor.checkForNewData(mockClock());  // activates AND reads in the same call

    EXPECT_EQ(fix.checkpoint.getTimestamp(), checkpoint_time);
}

TEST_F(MonitorIfDaemonTest, ProcessStarting_AlsoActivatesMonitor)
{
    RecordProperty(
        "Description",
        "EProcState::starting must be treated as an activation trigger, "
        "identical to running.");

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(1);
    fix.initIpc();
    fix.startProcess(mockClock());
    const auto checkpoint_time = mockClockOffset();
    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, checkpoint_time);
    fix.monitor.checkForNewData(mockClock());

    EXPECT_EQ(fix.checkpoint.getTimestamp(), checkpoint_time);
}

TEST_F(MonitorIfDaemonTest, ProcessOff_DeactivatesMonitor_NoFurtherDataForwarded)
{
    RecordProperty(
        "Description",
        "After a process-off event checkForNewData drains the IPC for the "
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
    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, mockClockOffset());
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
    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, checkpoint_time);
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
    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, mockClockFuture(5));
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
    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, future_time);
    fix.monitor.checkForNewData(mockClock());  // not consumed yet
    mockClockSkip(2);
    fix.monitor.checkForNewData(mockClock());  // now within window -> consumed
    EXPECT_EQ(fix.checkpoint.getTimestamp(), future_time);
}

TEST_F(MonitorIfDaemonTest, Active_NonMatchingCheckpointId_NotForwarded)
{
    RecordProperty(
        "Description",
        "An IPC element whose checkpointId does not match any attached "
        "Checkpoint must be silently discarded.");

    constexpr uint32_t kNonMatchingId = 99U;

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(Exactly(0));
    fix.initIpc();
    fix.activateProcess(mockClock());
    fix.sendCheckpoint(kNonMatchingId, mockClock());
    fix.monitor.checkForNewData(mockClock());
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
    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, mockClock());
    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, mockClock());
    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, mockClock());
    fix.monitor.checkForNewData(mockClock());
}

TEST_F(MonitorIfDaemonTest, Active_TwoAttachedCheckpoints_RoutedByCheckpointId)
{
    RecordProperty(
        "Description",
        "When two Checkpoints with different IDs are attached, each IPC "
        "element must be forwarded only to the Checkpoint whose ID matches.");

    constexpr uint32_t kCheckpointId2 = 2U;

    MonitorIfDaemonFixture fix;
    EXPECT_CALL(fix.checkpointMock, updateData).Times(1);
    fix.initIpc();

    ifappl::Checkpoint checkpoint2("test_cp2", kCheckpointId2, &fix.processState);
    CheckpointMock mock2;
    EXPECT_CALL(mock2, updateData).Times(1);
    checkpoint2.attachObserver(mock2);
    fix.monitor.attachCheckpoint(checkpoint2);

    fix.activateProcess(mockClock());
    const auto checkpoint_time = mockClock();
    fix.sendCheckpoint(MonitorIfDaemonFixture::kCheckpointId, checkpoint_time);
    fix.sendCheckpoint(kCheckpointId2, checkpoint_time);
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

}  // namespace score::lcm::saf
