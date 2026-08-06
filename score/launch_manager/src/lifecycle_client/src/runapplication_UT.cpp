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

#include "score/mw/lifecycle/applicationcontextmock.h"
#include "score/mw/lifecycle/lifecyclemanagermock.h"
#include "score/mw/lifecycle/runapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Eq;
using ::testing::Return;

namespace
{

class DummyApplication : public score::mw::lifecycle::Application
{
  public:
    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext&) override
    {
        return 0;
    }
    std::int32_t Run(const score::cpp::stop_token&) override
    {
        return 0;
    }
};

class DummyApplicationWithIntArg : public score::mw::lifecycle::Application
{
  public:
    explicit DummyApplicationWithIntArg(int value) : value_{value}
    {
    }
    std::int32_t Initialize(const score::mw::lifecycle::ApplicationContext&) override
    {
        return 0;
    }
    std::int32_t Run(const score::cpp::stop_token&) override
    {
        return 0;
    }
    int value_;
};

class RunTest : public ::testing::Test
{
  protected:
    // Mocks must be alive before any Run<> object is constructed so that the
    // static callbacks used by the link-time substitution pattern are registered.
    score::mw::lifecycle::ApplicationContextMock context_mock_;
    score::mw::lifecycle::LifeCycleManagerMock lifecycle_mock_;

    static constexpr int kArgc = 1;
    const char* kArgv[1] = {"test_app"};

    void SetUp() override
    {
        EXPECT_CALL(context_mock_, ctor(_, _)).Times(::testing::AnyNumber());
        EXPECT_CALL(lifecycle_mock_, ctor()).Times(::testing::AnyNumber());
        EXPECT_CALL(lifecycle_mock_, dtor()).Times(::testing::AnyNumber());
    }
};

}  // namespace

using ::testing::_;
using ::testing::Eq;
using ::testing::Return;

TEST_F(RunTest, AsPosixProcessReturnsZeroExitCode)
{
    RecordProperty("Description", "AsPosixProcess() forwards the zero return value of LifeCycleManager::run().");

    EXPECT_CALL(lifecycle_mock_, run(_, _)).WillOnce(Return(0));

    score::mw::lifecycle::Run<DummyApplication> runner(kArgc, kArgv);
    EXPECT_EQ(runner.AsPosixProcess(), 0);
}

TEST_F(RunTest, AsPosixProcessForwardsNonZeroExitCode)
{
    RecordProperty("Description", "AsPosixProcess() forwards a non-zero return value of LifeCycleManager::run().");

    EXPECT_CALL(lifecycle_mock_, run(_, _)).WillOnce(Return(42));

    score::mw::lifecycle::Run<DummyApplication> runner(kArgc, kArgv);
    EXPECT_EQ(runner.AsPosixProcess(), 42);
}

TEST_F(RunTest, AsPosixProcessCreatesAndDestroysLifeCycleManager)
{
    RecordProperty(
        "Description", "AsPosixProcess() constructs exactly one LifeCycleManager and destroys it before returning.");

    EXPECT_CALL(lifecycle_mock_, ctor()).Times(1);
    EXPECT_CALL(lifecycle_mock_, dtor()).Times(1);
    EXPECT_CALL(lifecycle_mock_, run(_, _)).WillOnce(Return(0));

    score::mw::lifecycle::Run<DummyApplication> runner(kArgc, kArgv);
    runner.AsPosixProcess();
}

TEST_F(RunTest, AsPosixProcessPassesCorrectApplicationTypeToRun)
{
    RecordProperty(
        "Description",
        "The Application instance passed to LifeCycleManager::run() is of the type given as "
        "the Run template argument.");

    EXPECT_CALL(lifecycle_mock_, run(_, _))
        .WillOnce([](score::mw::lifecycle::Application& app, const score::mw::lifecycle::ApplicationContext&) {
            EXPECT_NE(dynamic_cast<DummyApplication*>(&app), nullptr);
            return 0;
        });

    score::mw::lifecycle::Run<DummyApplication> runner(kArgc, kArgv);
    runner.AsPosixProcess();
}

TEST_F(RunTest, AsPosixProcessForwardsConstructorArgsToApplication)
{
    RecordProperty(
        "Description", "Extra arguments passed to AsPosixProcess() are forwarded to the ApplicationType constructor.");

    EXPECT_CALL(lifecycle_mock_, run(_, _))
        .WillOnce([](score::mw::lifecycle::Application& app, const score::mw::lifecycle::ApplicationContext&) {
            auto* typed_app = dynamic_cast<DummyApplicationWithIntArg*>(&app);
            EXPECT_NE(typed_app, nullptr);
            if (typed_app != nullptr)
            {
                EXPECT_EQ(typed_app->value_, 99);
            }
            return 0;
        });

    score::mw::lifecycle::Run<DummyApplicationWithIntArg> runner(kArgc, kArgv);
    runner.AsPosixProcess(99);
}

TEST_F(RunTest, RunApplicationFreeFunctionDelegatesToAsPosixProcess)
{
    RecordProperty(
        "Description",
        "The run_application<>() free function delegates to Run<>::AsPosixProcess() and returns "
        "the same exit code.");

    EXPECT_CALL(lifecycle_mock_, run(_, _)).WillOnce(Return(7));

    const auto result = score::mw::lifecycle::run_application<DummyApplication>(kArgc, kArgv);
    EXPECT_EQ(result, 7);
}

TEST_F(RunTest, RunConstructorPassesArgcArgvToApplicationContext)
{
    RecordProperty("Description", "The Run constructor forwards argc and argv to the ApplicationContext constructor.");

    EXPECT_CALL(context_mock_, ctor(Eq(kArgc), Eq(static_cast<const char* const*>(kArgv)))).Times(1);
    EXPECT_CALL(lifecycle_mock_, run(_, _)).WillOnce(Return(0));

    score::mw::lifecycle::Run<DummyApplication> runner(kArgc, kArgv);
    runner.AsPosixProcess();
}
