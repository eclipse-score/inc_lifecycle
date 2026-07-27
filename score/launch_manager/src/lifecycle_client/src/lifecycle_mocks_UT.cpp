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

/// @file cpp_lifecycle_app_UT.cpp
/// @brief Unit tests for cpp_lifecycle_app that demonstrate how to use
///        LifeCycleManagerMock and ApplicationContextMock from external packages.
///
/// These tests guard the mock API against accidental regressions and serve as
/// a reference for other components (e.g. config_management) that depend on the
/// lifecycle mocks in their own test suites.

#include "score/mw/lifecycle/applicationcontextmock.h"
#include "score/mw/lifecycle/lifecyclemanagermock.h"
#include "score/mw/lifecycle/runapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Eq;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{

/// Minimal Application that records which argument was retrieved during Initialize().
class ArgCapturingApp final : public score::mw::lifecycle::Application
{
  public:
    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext& ctx) override
    {
        captured_arg_ = ctx.get_argument("--config");
        return captured_arg_.empty() ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    std::int32_t Run(const score::cpp::stop_token&) override
    {
        return EXIT_SUCCESS;
    }

    std::string captured_arg_;
};

/// Minimal Application that always initialises and runs successfully.
class AlwaysOkApp final : public score::mw::lifecycle::Application
{
  public:
    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext&) override
    {
        return EXIT_SUCCESS;
    }

    std::int32_t Run(const score::cpp::stop_token&) override
    {
        return EXIT_SUCCESS;
    }
};

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

/// @brief Tests for lifecycle mock usage from an external-package perspective.
///
/// The mocks must be constructed *before* any score::mw::lifecycle::Run<> object
/// so that the static link-time substitution callbacks are installed in time.
class LifecycleMockUsageTest : public ::testing::Test
{
  protected:
    score::mw::lifecycle::ApplicationContextMock context_mock_;
    score::mw::lifecycle::LifeCycleManagerMock   lifecycle_mock_;

    static constexpr int kArgc     = 1;
    const char*          kArgv[1]  = {"test_app"};

    void SetUp() override
    {
        EXPECT_CALL(context_mock_, ctor(_, _)).Times(AnyNumber());
        EXPECT_CALL(lifecycle_mock_, ctor()).Times(AnyNumber());
        EXPECT_CALL(lifecycle_mock_, dtor()).Times(AnyNumber());
    }
};

}  // namespace

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

/// Verify that LifeCycleManagerMock::run() is called and its return value
/// is propagated through AsPosixProcess() — basic mock wiring smoke test.
TEST_F(LifecycleMockUsageTest, MockRunReturnValueIsForwardedToCaller)
{
    RecordProperty("Description",
                   "LifeCycleManagerMock::run() return value reaches AsPosixProcess().");

    EXPECT_CALL(lifecycle_mock_, run(_, _)).WillOnce(Return(EXIT_SUCCESS));

    score::mw::lifecycle::Run<AlwaysOkApp> runner(kArgc, kArgv);
    EXPECT_EQ(runner.AsPosixProcess(), EXIT_SUCCESS);
}

/// Verify that a non-zero exit code coming from the mock is forwarded correctly,
/// matching the behaviour that external callers (e.g. config_management) rely on.
TEST_F(LifecycleMockUsageTest, MockRunNonZeroReturnValueIsForwarded)
{
    RecordProperty("Description",
                   "A non-zero return from LifeCycleManagerMock::run() is forwarded unchanged.");

    EXPECT_CALL(lifecycle_mock_, run(_, _)).WillOnce(Return(EXIT_FAILURE));

    score::mw::lifecycle::Run<AlwaysOkApp> runner(kArgc, kArgv);
    EXPECT_EQ(runner.AsPosixProcess(), EXIT_FAILURE);
}

/// Verify that ApplicationContextMock::get_argument() is called during Initialize()
/// and that the returned value is visible to the application logic.
/// This is the pattern used by config_management's unit tests to simulate
/// command-line arguments without spawning a real process.
TEST_F(LifecycleMockUsageTest, ApplicationReceivesArgumentFromContextMock)
{
    RecordProperty("Description",
                   "ApplicationContextMock::get_argument() result is visible inside Initialize().");

    const std::string expected_config{"--config=/etc/app.cfg"};

    EXPECT_CALL(lifecycle_mock_, run(_, _))
        .WillOnce([&](score::mw::lifecycle::Application& app,
                      const score::mw::lifecycle::ApplicationContext& ctx) {
            EXPECT_CALL(context_mock_, get_argument(std::string_view{"--config"}))
                .WillOnce(Return(expected_config));
            return app.Initialize(ctx);
        });

    score::mw::lifecycle::Run<ArgCapturingApp> runner(kArgc, kArgv);
    EXPECT_EQ(runner.AsPosixProcess(), EXIT_SUCCESS);
}

/// Verify that ApplicationContextMock::get_arguments() can provide a full
/// argument list — mirroring how config_management passes a simulated argv.
TEST_F(LifecycleMockUsageTest, ContextMockProvidesArgumentList)
{
    RecordProperty("Description",
                   "ApplicationContextMock::get_arguments() returns the configured argument list.");

    const std::vector<std::string> expected_args{"my_app", "--verbose", "--config=/tmp/cfg"};

    EXPECT_CALL(context_mock_, get_arguments()).WillRepeatedly(ReturnRef(expected_args));

    const auto& args = context_mock_.get_arguments();
    EXPECT_EQ(args, expected_args);
}

/// Verify that exactly one LifeCycleManager is constructed and destroyed per
/// AsPosixProcess() call — important for callers that own RAII resources.
TEST_F(LifecycleMockUsageTest, ExactlyOneLifecycleManagerLifetimePerRun)
{
    RecordProperty("Description",
                   "Exactly one ctor/dtor pair of LifeCycleManager occurs per AsPosixProcess().");

    EXPECT_CALL(lifecycle_mock_, ctor()).Times(1);
    EXPECT_CALL(lifecycle_mock_, dtor()).Times(1);
    EXPECT_CALL(lifecycle_mock_, run(_, _)).WillOnce(Return(EXIT_SUCCESS));

    score::mw::lifecycle::Run<AlwaysOkApp> runner(kArgc, kArgv);
    runner.AsPosixProcess();
}
