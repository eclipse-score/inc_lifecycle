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

#include <cerrno>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "score/mw/launch_manager/configuration/config.hpp"
#include "score/mw/launch_manager/watchdog/details/Watchdog.hpp"
#include "score/mw/launch_manager/watchdog/details/WatchdogImpl.hpp"
#include "score/os/errno.h"
#include "score/os/mocklib/fcntl_mock.h"
#include "score/os/mocklib/ioctl_mock.h"
#include "score/os/mocklib/unistdmock.h"

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

using score::lcm::watchdog::IWatchdogIf;
using score::lcm::watchdog::WatchdogImpl;
using score::mw::launch_manager::configuration::WatchdogConfig;

namespace
{

/// @brief Cycle time of 50ms in nanoseconds, used as the default for tests that don't care about the exact value.
constexpr std::int64_t kDefaultCycleTimeNs{50'000'000};

// Succesful return value of ioctl operation
score::cpp::expected_blank<score::os::Error> IoctlOk()
{
    return score::cpp::expected_blank<score::os::Error>{};
}

// Error retun value of IoctlErr
score::cpp::expected_blank<score::os::Error> IoctlErr(std::int32_t errnoCode = EIO)
{
    return score::cpp::unexpected(score::os::Error::createFromErrno(errnoCode));
}

// Successful return value for open()
score::cpp::expected<std::int32_t, score::os::Error> OpenOk(std::int32_t fd)
{
    return fd;
}

// Error return value for open()
score::cpp::expected<std::int32_t, score::os::Error> OpenErr()
{
    return score::cpp::unexpected(score::os::Error::createFromErrno(ENOENT));
}

// Successful return value for close()
score::cpp::expected_blank<score::os::Error> CloseOk()
{
    return score::cpp::expected_blank<score::os::Error>{};
}

// Successful return value for write()
score::cpp::expected<ssize_t, score::os::Error> WriteOk(ssize_t n)
{
    return n;
}

/// @brief Writes `value` into the ioctl out-param and reports success. Used for WDIOC_GETTIMEOUT/WDIOC_GETTIMELEFT.
auto SetOutParam(std::int32_t value)
{
    return testing::Invoke([value](std::int32_t, std::int32_t, void* arg) {
        if (arg != nullptr)
        {
            *static_cast<std::int32_t*>(arg) = value;
        }
        return score::cpp::expected_blank<score::os::Error>{};
    });
}

/// @brief Simulates a device altering the requested WDIOC_SETTIMEOUT value by `delta`.
auto AlterOutParamBy(std::int32_t delta)
{
    return testing::Invoke([delta](std::int32_t, std::int32_t, void* arg) {
        if (arg != nullptr)
        {
            *static_cast<std::int32_t*>(arg) += delta;
        }
        return score::cpp::expected_blank<score::os::Error>{};
    });
}

/// @brief WatchdogImpl subclass that mocks waitForever() so fireWatchdogReaction() returns for testing.
class WatchdogImpl_FireWatchdogMock : public WatchdogImpl
{
  public:
    explicit WatchdogImpl_FireWatchdogMock(
        score::os::Ioctl& ioctl,
        score::os::Fcntl& fcntl,
        score::os::Unistd& unistd) noexcept
        : WatchdogImpl{ioctl, fcntl, unistd}
    {
    }

    MOCK_METHOD(void, waitForever, (), (const, noexcept, override));
};

class WatchdogImplTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "equivalence-classes");
    }

    /// @brief Creates a WatchdogConfig instance with the given parameters.
    static WatchdogConfig makeCfg(
        std::string fileName,
        std::uint32_t maxTimeoutMs,
        bool canBeDeactivated = true,
        bool needsMagicClose = false)
    {
        WatchdogConfig cfg{};
        cfg.device_file_path = std::move(fileName);
        cfg.max_timeout_ms = maxTimeoutMs;
        cfg.deactivate_on_shutdown = canBeDeactivated;
        cfg.require_magic_close = needsMagicClose;
        return cfg;
    }

    /// @brief Creates a new WatchdogImpl with mocked OS interfaces injected.
    std::unique_ptr<WatchdogImpl> makeWatchdog()
    {
        return std::make_unique<WatchdogImpl>(*ioctlMock_, *fcntlMock_, *unistdMock_);
    }

    /// @brief Creates a WatchdogImpl_FireWatchdogMock with mocked OS interfaces injected.
    std::unique_ptr<WatchdogImpl_FireWatchdogMock> makeWatchdogFireMock()
    {
        return std::make_unique<WatchdogImpl_FireWatchdogMock>(*ioctlMock_, *fcntlMock_, *unistdMock_);
    }

    /// @brief Programs the mocks so that enabling the device for `cfg` goes through the full enable
    /// sequence (open -> GETTIMEOUT -> GETTIMELEFT -> SETTIMEOUT -> SETOPTIONS) and succeeds.
    void expectFullEnable(const WatchdogConfig& cfg, std::int32_t fd = 1)
    {
        EXPECT_CALL(*fcntlMock_, open(StrEq(cfg.device_file_path), _)).WillOnce(Return(OpenOk(fd)));
        // Report a current timeout of 0, which never matches the configured device timeout, so that the
        // enable sequence always goes through GETTIMELEFT + SETTIMEOUT.
        EXPECT_CALL(*ioctlMock_, ioctl(fd, WDIOC_GETTIMEOUT, _)).WillOnce(SetOutParam(0));
        EXPECT_CALL(*ioctlMock_, ioctl(fd, WDIOC_GETTIMELEFT, _)).WillOnce(Return(IoctlOk()));
        EXPECT_CALL(*ioctlMock_, ioctl(fd, WDIOC_SETTIMEOUT, _)).WillOnce(Return(IoctlOk()));
        EXPECT_CALL(*ioctlMock_, ioctl(fd, WDIOC_SETOPTIONS, _)).WillOnce(Return(IoctlOk()));
    }

    /// @brief Programs the mocks for a successful disableDevice() sequence on `fd`.
    void expectDisable(const WatchdogConfig& cfg, std::int32_t fd = 1)
    {
        if (cfg.require_magic_close)
        {
            EXPECT_CALL(*unistdMock_, write(fd, _, _)).WillOnce(Return(WriteOk(2)));
        }
        EXPECT_CALL(*ioctlMock_, ioctl(fd, WDIOC_SETOPTIONS, _)).WillOnce(Return(IoctlOk()));
        EXPECT_CALL(*unistdMock_, close(fd)).WillOnce(Return(CloseOk()));
    }

    score::os::MockGuard<score::os::FcntlMock> fcntlMock_;
    score::os::MockGuard<score::os::IoctlMock> ioctlMock_;
    score::os::MockGuard<score::os::UnistdMock> unistdMock_;
};

// ═══════════════════════════════════════════════════════════
// init() tests
// ═══════════════════════════════════════════════════════════

TEST_F(WatchdogImplTest, WdgInit_FailsIfNotInIdleState)
{
    RecordProperty("Description", "init() fails when the watchdog is already in the activated state.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    expectFullEnable(cfg);
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));
    ASSERT_TRUE(wdg->enable());

    auto cfg2 = makeCfg("/dev/watchdog_2", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    EXPECT_FALSE(wdg->init(cfg2, kDefaultCycleTimeNs));
}

TEST_F(WatchdogImplTest, WdgInit_FailsWatchdogTimeoutSmallerThenCycleTime)
{
    RecordProperty(
        "Description",
        "init() fails when the configured cycle time is larger than the device's max timeout, since "
        "serviceWatchdog() could not be called in time to prevent a reset.");

    constexpr std::uint32_t timeoutMs{2000U};
    constexpr std::uint32_t nanosecPerMillisec = 1'000'000;
    const std::int64_t cycleTimeNs{static_cast<std::int64_t>(timeoutMs + 1U) * nanosecPerMillisec};
    auto cfg = makeCfg("/dev/watchdog", timeoutMs, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();

    EXPECT_FALSE(wdg->init(cfg, cycleTimeNs));
}

TEST_F(WatchdogImplTest, WdgInit_FailsIfTimeoutExceedsUint16Max)
{
    RecordProperty(
        "Description",
        "init() fails when the configured max timeout does not fit into uint16_t, before the "
        "kTimeoutMaxMillis range check is even applied.");

    constexpr std::uint32_t timeoutExceedingUint16Max{
        static_cast<std::uint32_t>(std::numeric_limits<std::uint16_t>::max()) + 1U};
    auto cfg =
        makeCfg("/dev/watchdog", timeoutExceedingUint16Max, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();

    EXPECT_FALSE(wdg->init(cfg, kDefaultCycleTimeNs));
}

TEST_F(WatchdogImplTest, WdgInit_FailsIfTimeoutIsTooLarge)
{
    RecordProperty("Description", "init() fails when the configured timeout exceeds kTimeoutMaxMillis.");

    auto cfg = makeCfg(
        "/dev/watchdog",
        static_cast<std::uint32_t>(IWatchdogIf::kTimeoutMaxMillis) + 1U,
        true /*canBeDeactivated*/,
        false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    EXPECT_FALSE(wdg->init(cfg, kDefaultCycleTimeNs));
}

TEST_F(WatchdogImplTest, WdgInit_FailsIfTimeoutIsTooSmall)
{
    RecordProperty("Description", "init() fails when the configured timeout is below kTimeoutMinMillis.");

    auto cfg = makeCfg(
        "/dev/watchdog",
        static_cast<std::uint32_t>(IWatchdogIf::kTimeoutMinMillis) - 1U,
        true /*canBeDeactivated*/,
        false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    EXPECT_FALSE(wdg->init(cfg, kDefaultCycleTimeNs));
}

#ifndef __QNXNTO__
TEST_F(WatchdogImplTest, WdgInit_FailsIfTimeoutResolutionIsWrong)
{
    RecordProperty(
        "Description",
        "init() fails on Linux when the configured timeout is not a multiple of the 1 second "
        "resolution.");

    auto cfg = makeCfg("/dev/watchdog", 2123U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();

    EXPECT_FALSE(wdg->init(cfg, kDefaultCycleTimeNs));
}
#endif

TEST_F(WatchdogImplTest, WdgInit_FailsIfDeviceAlreadyConfigured)
{
    RecordProperty(
        "Description", "init() fails when called a second time, since only a single watchdog device is supported.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

    EXPECT_FALSE(wdg->init(cfg, kDefaultCycleTimeNs));
    EXPECT_FALSE(wdg->init(
        makeCfg("/dev/watchdog_2", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/), kDefaultCycleTimeNs));
}

class WatchdogImpl_UT_paramConfigName : public WatchdogImplTest, public ::testing::WithParamInterface<std::uint32_t>
{
};

TEST_P(WatchdogImpl_UT_paramConfigName, WdgInit_SucceedsWithValidDeviceConfiguration)
{
    RecordProperty("Description", "init() succeeds for boundary timeout values (min, mid, max).");

    const std::uint32_t timeoutMs{GetParam()};
    auto cfg = makeCfg("/dev/watchdog", timeoutMs, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    EXPECT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs)) << "Expected init() to succeed for timeout=" << timeoutMs << "ms";
}

INSTANTIATE_TEST_SUITE_P(
    Boundary,
    WatchdogImpl_UT_paramConfigName,
    ::testing::Values(
        static_cast<std::uint32_t>(IWatchdogIf::kTimeoutMinMillis),
        static_cast<std::uint32_t>(2000U),
        static_cast<std::uint32_t>(IWatchdogIf::kTimeoutMaxMillis)));

// ═══════════════════════════════════════════════════════════
// enable() tests
// ═══════════════════════════════════════════════════════════

TEST_F(WatchdogImplTest, WdgEnable_SucceedsIfNoDeviceConfigured)
{
    RecordProperty("Description", "enable() succeeds when no watchdog device has been configured.");

    auto wdg = makeWatchdog();
    EXPECT_TRUE(wdg->enable());
}

TEST_F(WatchdogImplTest, WdgEnable_FailsIfNotInIdleState)
{
    RecordProperty("Description", "enable() fails when called a second time after the watchdog is already activated.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    expectFullEnable(cfg);
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));
    ASSERT_TRUE(wdg->enable());

    EXPECT_FALSE(wdg->enable());
}

TEST_F(WatchdogImplTest, WdgEnable_FailsIfOpenFails)
{
    RecordProperty("Description", "enable() fails when opening the configured device file fails.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

    EXPECT_CALL(*fcntlMock_, open(StrEq(cfg.device_file_path), _)).WillOnce(Return(OpenErr()));
    EXPECT_CALL(*ioctlMock_, ioctl).Times(0);

    EXPECT_FALSE(wdg->enable());
}

TEST_F(WatchdogImplTest, WdgEnable_DoesNotSetConfiguredTimeoutValue_WhenTimeoutAlreadyCorrect)
{
    RecordProperty(
        "Description",
        "enable() skips WDIOC_GETTIMELEFT and WDIOC_SETTIMEOUT when the device already reports the "
        "desired timeout.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

#ifndef __QNXNTO__
    constexpr std::int32_t kMillisPerSecond = 1000U;
    const std::int32_t currentTimeoutRaw{static_cast<std::int32_t>(cfg.max_timeout_ms / kMillisPerSecond)};
#else
    const std::int32_t currentTimeoutRaw{static_cast<std::int32_t>(cfg.max_timeout_ms)};
#endif

    EXPECT_CALL(*fcntlMock_, open(StrEq(cfg.device_file_path), _)).WillOnce(Return(OpenOk(1)));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMEOUT, _)).WillOnce(SetOutParam(currentTimeoutRaw));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMELEFT, _)).Times(0);
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETTIMEOUT, _)).Times(0);
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETOPTIONS, _)).WillOnce(Return(IoctlOk()));

    EXPECT_TRUE(wdg->enable());
}

TEST_F(WatchdogImplTest, WdgEnable_FailsIfGetTimeoutFails)
{
    RecordProperty("Description", "enable() fails and skips the remaining ioctls when WDIOC_GETTIMEOUT fails.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

    EXPECT_CALL(*fcntlMock_, open(StrEq(cfg.device_file_path), _)).WillOnce(Return(OpenOk(1)));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMEOUT, _)).WillOnce(Return(IoctlErr()));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMELEFT, _)).Times(0);
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETTIMEOUT, _)).Times(0);
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETOPTIONS, _)).Times(0);

    EXPECT_FALSE(wdg->enable());
}

TEST_F(WatchdogImplTest, WdgEnable_FailsIfGetRemainingTimeLeftFails)
{
    RecordProperty(
        "Description", "enable() fails and skips WDIOC_SETTIMEOUT/WDIOC_SETOPTIONS when WDIOC_GETTIMELEFT fails.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

    EXPECT_CALL(*fcntlMock_, open(StrEq(cfg.device_file_path), _)).WillOnce(Return(OpenOk(1)));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMEOUT, _)).WillOnce(SetOutParam(0));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMELEFT, _)).WillOnce(Return(IoctlErr()));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETTIMEOUT, _)).Times(0);
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETOPTIONS, _)).Times(0);

    EXPECT_FALSE(wdg->enable());
}

TEST_F(WatchdogImplTest, WdgEnable_FailsIfSetTimeoutFails)
{
    RecordProperty("Description", "enable() fails and skips WDIOC_SETOPTIONS when WDIOC_SETTIMEOUT fails.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

    EXPECT_CALL(*fcntlMock_, open(StrEq(cfg.device_file_path), _)).WillOnce(Return(OpenOk(1)));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMEOUT, _)).WillOnce(SetOutParam(0));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMELEFT, _)).WillOnce(Return(IoctlOk()));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETTIMEOUT, _)).WillOnce(Return(IoctlErr()));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETOPTIONS, _)).Times(0);

    EXPECT_FALSE(wdg->enable());
}

TEST_F(WatchdogImplTest, WdgEnable_FailsIfTimeoutValueIsAltered)
{
    RecordProperty("Description", "enable() fails when the device alters the requested timeout to a different value.");

    auto cfg = makeCfg("/dev/watchdog", 30'000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

    EXPECT_CALL(*fcntlMock_, open(StrEq(cfg.device_file_path), _)).WillOnce(Return(OpenOk(1)));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMEOUT, _)).WillOnce(SetOutParam(0));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMELEFT, _)).WillOnce(Return(IoctlOk()));
    // Device alters the requested timeout to a value that doesn't match what was requested.
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETTIMEOUT, _)).WillOnce(AlterOutParamBy(1));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETOPTIONS, _)).Times(0);

    EXPECT_FALSE(wdg->enable());
}

TEST_F(WatchdogImplTest, WdgEnable_FailsIfEnablecardFails)
{
    RecordProperty("Description", "enable() fails when the WDIOS_ENABLECARD ioctl fails.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

    EXPECT_CALL(*fcntlMock_, open(StrEq(cfg.device_file_path), _)).WillOnce(Return(OpenOk(1)));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMEOUT, _)).WillOnce(SetOutParam(0));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_GETTIMELEFT, _)).WillOnce(Return(IoctlOk()));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETTIMEOUT, _)).WillOnce(Return(IoctlOk()));
    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETOPTIONS, _)).WillOnce(Return(IoctlErr()));

    EXPECT_FALSE(wdg->enable());
}

// ═══════════════════════════════════════════════════════════
// serviceWatchdog() tests
// ═══════════════════════════════════════════════════════════

TEST_F(WatchdogImplTest, WdgServiceWatchdog_NotInActivatedState)
{
    RecordProperty("Description", "serviceWatchdog() is a no-op when the watchdog has not been enabled.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

    EXPECT_CALL(*ioctlMock_, ioctl).Times(0);
    wdg->serviceWatchdog();
}

TEST_F(WatchdogImplTest, WdgServiceWatchdog_NoDevice)
{
    RecordProperty("Description", "serviceWatchdog() is a no-op when enabled with zero configured devices.");

    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->enable());

    EXPECT_CALL(*ioctlMock_, ioctl).Times(0);
    wdg->serviceWatchdog();
}

TEST_F(WatchdogImplTest, WdgServiceWatchdog_OneDevice)
{
    RecordProperty("Description", "serviceWatchdog() issues a single WDIOC_KEEPALIVE for one activated device.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();

    expectFullEnable(cfg);
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));
    ASSERT_TRUE(wdg->enable());

    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_KEEPALIVE, nullptr)).Times(1).WillOnce(Return(IoctlOk()));
    wdg->serviceWatchdog();
}

// ═══════════════════════════════════════════════════════════
// fireWatchdogReaction() tests
// ═══════════════════════════════════════════════════════════

TEST_F(WatchdogImplTest, WdgFireWatchdogReaction_FailsIfNotInActivatedState)
{
    RecordProperty(
        "Description",
        "fireWatchdogReaction() is a no-op and does not call waitForever() when the watchdog has not "
        "been enabled.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdogFireMock();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

    EXPECT_CALL(*ioctlMock_, ioctl(_, WDIOC_SETTIMEOUT, _)).Times(0);
    EXPECT_CALL(*wdg, waitForever).Times(0);
    wdg->fireWatchdogReaction();
}

TEST_F(WatchdogImplTest, WdgFireWatchdogReaction_OneDevice)
{
    RecordProperty(
        "Description",
        "fireWatchdogReaction() sets WDIOC_SETTIMEOUT to 0 and calls waitForever() for one activated "
        "device.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdogFireMock();

    expectFullEnable(cfg);
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));
    ASSERT_TRUE(wdg->enable());

    EXPECT_CALL(*ioctlMock_, ioctl(1, WDIOC_SETTIMEOUT, _)).Times(1).WillOnce(Return(IoctlOk()));
    EXPECT_CALL(*wdg, waitForever).Times(1);
    wdg->fireWatchdogReaction();
}

// ═══════════════════════════════════════════════════════════
// disable() tests
// ═══════════════════════════════════════════════════════════

TEST_F(WatchdogImplTest, WdgDisable_FailsIfNotInActivatedState)
{
    RecordProperty("Description", "disable() is a no-op when the watchdog has not been enabled.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));

    EXPECT_CALL(*ioctlMock_, ioctl).Times(0);
    EXPECT_CALL(*unistdMock_, close).Times(0);
    EXPECT_CALL(*unistdMock_, write).Times(0);

    wdg->disable();
}

TEST_F(WatchdogImplTest, WdgDisable_DisablesOneDevice)
{
    RecordProperty(
        "Description", "disable() issues WDIOS_DISABLECARD and closes the device file for a single activated device.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, false /*needsMagicClose*/);
    auto wdg = makeWatchdog();

    expectFullEnable(cfg);
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));
    ASSERT_TRUE(wdg->enable());

    expectDisable(cfg);
    wdg->disable();
}

TEST_F(WatchdogImplTest, WdgDisable_WritesMagicCloseCharacterWhenRequired)
{
    RecordProperty(
        "Description",
        "disable() writes the magic close character before issuing WDIOS_DISABLECARD for a device that "
        "requires it.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, true /*canBeDeactivated*/, true /*needsMagicClose*/);
    auto wdg = makeWatchdog();

    expectFullEnable(cfg);
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));
    ASSERT_TRUE(wdg->enable());

    expectDisable(cfg);
    wdg->disable();
}

TEST_F(WatchdogImplTest, WdgDisable_IgnoresDevicesThatCannotBeDisabled)
{
    RecordProperty(
        "Description", "disable() performs no ioctl/write/close calls for a device configured as non-deactivatable.");

    auto cfg = makeCfg("/dev/watchdog", 2000U, false /*canBeDeactivated*/, true /*needsMagicClose*/);
    auto wdg = makeWatchdog();

    expectFullEnable(cfg);
    ASSERT_TRUE(wdg->init(cfg, kDefaultCycleTimeNs));
    ASSERT_TRUE(wdg->enable());

    EXPECT_CALL(*ioctlMock_, ioctl).Times(0);
    EXPECT_CALL(*unistdMock_, write).Times(0);
    EXPECT_CALL(*unistdMock_, close).Times(0);

    wdg->disable();
}

}  // namespace
