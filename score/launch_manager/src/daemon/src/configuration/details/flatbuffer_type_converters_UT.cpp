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
#include "score/launch_manager/src/daemon/src/configuration/details/flatbuffer_type_converters.hpp"
#include "score/launch_manager/src/daemon/src/configuration/details/lm_flatcfg_generated.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sched.h>
#include <sys/types.h>
#include <cstdint>
#include <limits>
#include <vector>

namespace score::mw::lifecycle::internal::configuration
{
namespace
{

namespace fb = score::mw::lifecycle::internal::configuration::fb;

using ::testing::Eq;
using ::testing::IsFalse;
using ::testing::IsTrue;

// ============================================================================
// Helper: build a default Sandbox flatbuffer offset
// ============================================================================

::flatbuffers::Offset<fb::Sandbox> buildDefaultSandbox(::flatbuffers::FlatBufferBuilder& fbb)
{
    return fb::CreateSandbox(
        fbb,
        0 /*uid*/,
        0 /*gid*/,
        0 /*supplementary_group_ids*/,
        0 /*security_policy*/,
        fb::SchedulingPolicy::OTHER,
        0 /*scheduling_priority*/);
}

// ============================================================================
// Common base fixture
// ============================================================================

class TypeConverterTestBase : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

// ============================================================================
// secondsToMs tests
// ============================================================================

class SecondsToMsTest : public TypeConverterTestBase
{
};

TEST_F(SecondsToMsTest, ConvertsPositiveValue)
{
    RecordProperty("Description", "A positive seconds value converts to milliseconds.");

    auto result = secondsToMs(1.5);

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(*result, Eq(1500U));
}

TEST_F(SecondsToMsTest, ConvertsZero)
{
    RecordProperty("Description", "Zero seconds converts to zero milliseconds.");

    auto result = secondsToMs(0.0);

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(*result, Eq(0U));
}

TEST_F(SecondsToMsTest, RejectsNegativeValue)
{
    RecordProperty("Description", "A negative seconds value returns InvalidFormat.");

    auto result = secondsToMs(-1.0);

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(SecondsToMsTest, RejectsOverflow)
{
    RecordProperty("Description", "A value exceeding uint32_t max milliseconds returns InvalidFormat.");

    auto result = secondsToMs(5000000.0);

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(SecondsToMsTest, RejectsSubMillisecond)
{
    RecordProperty("Description", "A positive value that rounds to 0ms returns InvalidFormat.");

    auto result = secondsToMs(0.0001);

    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

// ============================================================================
// Enum conversion tests
// ============================================================================

class EnumConversionTest : public TypeConverterTestBase
{
};

struct ApplicationTypeTestParam
{
    fb::ApplicationType fb_type;
    ApplicationType expected;
    const char* name;
};

class ApplicationTypeTest : public TypeConverterTestBase, public ::testing::WithParamInterface<ApplicationTypeTestParam>
{
};

TEST_P(ApplicationTypeTest, ConvertsToExpectedValue)
{
    RecordProperty("Description", "ApplicationType enum maps to correct config value.");

    EXPECT_THAT(convertApplicationType(GetParam().fb_type), Eq(GetParam().expected));
}

INSTANTIATE_TEST_SUITE_P(
    ApplicationTypes,
    ApplicationTypeTest,
    ::testing::Values(
        ApplicationTypeTestParam{fb::ApplicationType::Native, ApplicationType::Native, "Native"},
        ApplicationTypeTestParam{fb::ApplicationType::Reporting, ApplicationType::Reporting, "Reporting"},
        ApplicationTypeTestParam{
            fb::ApplicationType::Reporting_And_Supervised,
            ApplicationType::ReportingAndSupervised,
            "ReportingAndSupervised"},
        ApplicationTypeTestParam{fb::ApplicationType::State_Manager, ApplicationType::StateManager, "StateManager"}),
    [](const ::testing::TestParamInfo<ApplicationTypeTestParam>& info) {
        return info.param.name;
    });

struct ProcessStateTestParam
{
    fb::ProcessState fb_state;
    ProcessState expected;
    const char* name;
};

class ProcessStateTest : public TypeConverterTestBase, public ::testing::WithParamInterface<ProcessStateTestParam>
{
};

TEST_P(ProcessStateTest, ConvertsToExpectedValue)
{
    RecordProperty("Description", "ProcessState enum maps to correct config value.");

    EXPECT_THAT(convertProcessState(GetParam().fb_state), Eq(GetParam().expected));
}

INSTANTIATE_TEST_SUITE_P(
    ProcessStates,
    ProcessStateTest,
    ::testing::Values(
        ProcessStateTestParam{fb::ProcessState::Running, ProcessState::Running, "Running"},
        ProcessStateTestParam{fb::ProcessState::Terminated, ProcessState::Terminated, "Terminated"}),
    [](const ::testing::TestParamInfo<ProcessStateTestParam>& info) {
        return info.param.name;
    });

struct SchedulingPolicyTestParam
{
    fb::SchedulingPolicy fb_policy;
    int32_t expected_posix_value;
    const char* name;
};

class SchedulingPolicyTest : public TypeConverterTestBase,
                             public ::testing::WithParamInterface<SchedulingPolicyTestParam>
{
};

TEST_P(SchedulingPolicyTest, ConvertsToExpectedPosixValue)
{
    RecordProperty("Description", "Scheduling policy enum maps to correct POSIX value.");

    auto result = convertSchedulingPolicy(GetParam().fb_policy);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(*result, Eq(GetParam().expected_posix_value));
}

INSTANTIATE_TEST_SUITE_P(
    SchedulingPolicies,
    SchedulingPolicyTest,
    ::testing::Values(
        SchedulingPolicyTestParam{fb::SchedulingPolicy::OTHER, SCHED_OTHER, "SCHED_OTHER"},
        SchedulingPolicyTestParam{fb::SchedulingPolicy::FIFO, SCHED_FIFO, "SCHED_FIFO"},
        SchedulingPolicyTestParam{fb::SchedulingPolicy::RR, SCHED_RR, "SCHED_RR"}),
    [](const ::testing::TestParamInfo<SchedulingPolicyTestParam>& info) {
        return info.param.name;
    });

TEST_F(EnumConversionTest, ConvertSchedulingPolicyUnsupported)
{
    RecordProperty("Description", "An unsupported scheduling policy value returns InvalidFormat.");
    auto result = convertSchedulingPolicy(static_cast<fb::SchedulingPolicy>(99));
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

// ============================================================================
// String/vector helper tests
// ============================================================================

class StringHelperTest : public TypeConverterTestBase
{
};

TEST_F(StringHelperTest, SafeStringReturnsValueWhenNotNull)
{
    RecordProperty("Description", "safeString returns the string value when pointer is non-null.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto str = fbb.CreateString("hello");
    fbb.Finish(str);
    const auto* ptr = ::flatbuffers::GetRoot<::flatbuffers::String>(fbb.GetBufferPointer());
    EXPECT_THAT(safeString(ptr), Eq("hello"));
}

TEST_F(StringHelperTest, SafeStringReturnsEmptyWhenNull)
{
    RecordProperty("Description", "safeString returns empty string when pointer is null.");
    EXPECT_THAT(safeString(nullptr), Eq(""));
}

TEST_F(StringHelperTest, ConvertStringVectorReturnsEmptyWhenNull)
{
    RecordProperty("Description", "convertStringVector returns empty vector when pointer is null.");
    auto result = convertStringVector(nullptr);
    EXPECT_THAT(result.empty(), IsTrue());
}

TEST_F(StringHelperTest, ConvertGidVectorReturnsEmptyWhenNull)
{
    RecordProperty("Description", "convertGidVector returns empty vector when pointer is null.");
    auto result = convertGidVector(nullptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->empty(), IsTrue());
}

// ============================================================================
// validateRange typed tests
// ============================================================================

template <typename T>
class ValidateRangeTest : public TypeConverterTestBase
{
};

using ValidateRangeTypes = ::testing::Types<uint16_t, int16_t, uint32_t, int32_t, int64_t>;
TYPED_TEST_SUITE(ValidateRangeTest, ValidateRangeTypes);

TYPED_TEST(ValidateRangeTest, AcceptsMinBoundary)
{
    this->RecordProperty("Description", "The minimum value of the target type is accepted.");

    auto result = validateRange<TypeParam>(static_cast<int64_t>(std::numeric_limits<TypeParam>::min()), "test_field");

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(*result, Eq(std::numeric_limits<TypeParam>::min()));
}

TYPED_TEST(ValidateRangeTest, AcceptsMaxBoundary)
{
    this->RecordProperty("Description", "The maximum value of the target type is accepted.");

    auto result = validateRange<TypeParam>(static_cast<int64_t>(std::numeric_limits<TypeParam>::max()), "test_field");

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(*result, Eq(std::numeric_limits<TypeParam>::max()));
}

TYPED_TEST(ValidateRangeTest, AcceptsZero)
{
    this->RecordProperty("Description", "Zero is accepted for all target types.");

    auto result = validateRange<TypeParam>(0, "test_field");

    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(*result, Eq(static_cast<TypeParam>(0)));
}

TYPED_TEST(ValidateRangeTest, RejectsValueAboveMax)
{
    this->RecordProperty("Description", "A value above the target type maximum is rejected.");

    if constexpr (sizeof(TypeParam) == sizeof(int64_t))
    {
        GTEST_SKIP() << "No int64_t value exceeds int64_t max";
    }
    else
    {
        auto result =
            validateRange<TypeParam>(static_cast<int64_t>(std::numeric_limits<TypeParam>::max()) + 1, "test_field");

        ASSERT_THAT(result.has_value(), IsFalse());
        EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
    }
}

TYPED_TEST(ValidateRangeTest, RejectsValueBelowMin)
{
    this->RecordProperty("Description", "A value below the target type minimum is rejected.");

    if constexpr (sizeof(TypeParam) == sizeof(int64_t) && std::is_signed_v<TypeParam>)
    {
        GTEST_SKIP() << "No int64_t value is below int64_t min";
    }
    else
    {
        auto result =
            validateRange<TypeParam>(static_cast<int64_t>(std::numeric_limits<TypeParam>::min()) - 1, "test_field");

        ASSERT_THAT(result.has_value(), IsFalse());
        EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
    }
}

// ============================================================================
// Struct converter tests
// ============================================================================

class ConverterTest : public TypeConverterTestBase
{
};

TEST_F(ConverterTest, ConvertRestartActionNullReturnsNullopt)
{
    RecordProperty("Description", "convertRestartAction with nullptr returns nullopt.");
    auto result = convertRestartAction(nullptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->has_value(), IsFalse());
}

TEST_F(ConverterTest, ConvertRestartActionValid)
{
    RecordProperty("Description", "convertRestartAction with valid fields returns correct values.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto ra = fb::CreateRestartAction(fbb, 3 /*number_of_attempts*/, 1500 /*delay_before_restart_ms*/);
    fbb.Finish(ra);
    const auto* ptr = ::flatbuffers::GetRoot<fb::RestartAction>(fbb.GetBufferPointer());

    auto result = convertRestartAction(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->has_value(), IsTrue());
    EXPECT_THAT(result->value().number_of_attempts, Eq(3U));
    EXPECT_THAT(result->value().delay_before_restart_ms, Eq(1500U));
}

TEST_F(ConverterTest, ConvertRestartActionMissingFieldsReturnsError)
{
    RecordProperty("Description", "convertRestartAction without required fields returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto ra = fb::CreateRestartAction(fbb);
    fbb.Finish(ra);
    const auto* ptr = ::flatbuffers::GetRoot<fb::RestartAction>(fbb.GetBufferPointer());

    auto result = convertRestartAction(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertSwitchRunTargetActionNullReturnsNullopt)
{
    RecordProperty("Description", "convertSwitchRunTargetAction with nullptr returns nullopt.");
    auto result = convertSwitchRunTargetAction(nullptr);
    EXPECT_THAT(result.has_value(), IsFalse());
}

TEST_F(ConverterTest, ConvertSwitchRunTargetActionValid)
{
    RecordProperty("Description", "convertSwitchRunTargetAction returns the run target name.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto target = fbb.CreateString("Fallback");
    auto sa = fb::CreateSwitchRunTargetAction(fbb, target);
    fbb.Finish(sa);
    const auto* ptr = ::flatbuffers::GetRoot<fb::SwitchRunTargetAction>(fbb.GetBufferPointer());

    auto result = convertSwitchRunTargetAction(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->run_target, Eq("Fallback"));
}

TEST_F(ConverterTest, ConvertComponentAliveSupervisionNullReturnsDefault)
{
    RecordProperty("Description", "convertComponentAliveSupervision with nullptr returns default-constructed value.");
    auto result = convertComponentAliveSupervision(nullptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->reporting_cycle_ms, Eq(0U));
    EXPECT_THAT(result->failed_cycles_tolerance, Eq(0U));
}

TEST_F(ConverterTest, ConvertComponentAliveSupervisionValid)
{
    RecordProperty("Description", "convertComponentAliveSupervision maps all fields correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto cas = fb::CreateComponentAliveSupervision(
        fbb, 500 /*reporting_cycle_ms*/, 2 /*failed_cycles_tolerance*/, 1 /*min_indications*/, 3 /*max_indications*/);
    fbb.Finish(cas);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ComponentAliveSupervision>(fbb.GetBufferPointer());

    auto result = convertComponentAliveSupervision(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->reporting_cycle_ms, Eq(500U));
    EXPECT_THAT(result->failed_cycles_tolerance, Eq(2U));
    ASSERT_THAT(result->min_indications.has_value(), IsTrue());
    EXPECT_THAT(*result->min_indications, Eq(1U));
    ASSERT_THAT(result->max_indications.has_value(), IsTrue());
    EXPECT_THAT(*result->max_indications, Eq(3U));
}

TEST_F(ConverterTest, ConvertComponentAliveSupervisionMissingReportingCycleReturnsError)
{
    RecordProperty("Description", "Missing reporting_cycle_ms returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto cas = fb::CreateComponentAliveSupervision(
        fbb, ::flatbuffers::nullopt /*reporting_cycle_ms*/, 3 /*failed_cycles_tolerance*/);
    fbb.Finish(cas);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ComponentAliveSupervision>(fbb.GetBufferPointer());

    auto result = convertComponentAliveSupervision(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertComponentAliveSupervisionMissingToleranceReturnsError)
{
    RecordProperty("Description", "Missing failed_cycles_tolerance returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto cas = fb::CreateComponentAliveSupervision(
        fbb, 1000 /*reporting_cycle_ms*/, ::flatbuffers::nullopt /*failed_cycles_tolerance*/);
    fbb.Finish(cas);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ComponentAliveSupervision>(fbb.GetBufferPointer());

    auto result = convertComponentAliveSupervision(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertComponentAliveSupervisionBothIndicationsAbsentReturnsError)
{
    RecordProperty("Description", "Both min/max_indications absent returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto cas = fb::CreateComponentAliveSupervision(fbb, 1000 /*reporting_cycle_ms*/, 3 /*failed_cycles_tolerance*/);
    fbb.Finish(cas);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ComponentAliveSupervision>(fbb.GetBufferPointer());

    auto result = convertComponentAliveSupervision(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertComponentAliveSupervisionOnlyMinIndicationsPresent)
{
    RecordProperty("Description", "Only min_indications set is accepted, max_indications remains nullopt.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto cas = fb::CreateComponentAliveSupervision(
        fbb, 1000 /*reporting_cycle_ms*/, 3 /*failed_cycles_tolerance*/, 2 /*min_indications*/);
    fbb.Finish(cas);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ComponentAliveSupervision>(fbb.GetBufferPointer());

    auto result = convertComponentAliveSupervision(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->min_indications.has_value(), IsTrue());
    EXPECT_THAT(*result->min_indications, Eq(2U));
    EXPECT_THAT(result->max_indications.has_value(), IsFalse());
}

TEST_F(ConverterTest, ConvertComponentAliveSupervisionOnlyMaxIndicationsPresent)
{
    RecordProperty("Description", "Only max_indications set is accepted, min_indications remains nullopt.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto cas = fb::CreateComponentAliveSupervision(
        fbb,
        1000 /*reporting_cycle_ms*/,
        3 /*failed_cycles_tolerance*/,
        ::flatbuffers::nullopt /*min_indications*/,
        5 /*max_indications*/);
    fbb.Finish(cas);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ComponentAliveSupervision>(fbb.GetBufferPointer());

    auto result = convertComponentAliveSupervision(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->min_indications.has_value(), IsFalse());
    ASSERT_THAT(result->max_indications.has_value(), IsTrue());
    EXPECT_THAT(*result->max_indications, Eq(5U));
}

TEST_F(ConverterTest, ConvertApplicationProfileValid)
{
    RecordProperty("Description", "convertApplicationProfile maps all fields correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto ap =
        fb::CreateApplicationProfile(fbb, fb::ApplicationType::Reporting_And_Supervised, true /*is_self_terminating*/);
    fbb.Finish(ap);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ApplicationProfile>(fbb.GetBufferPointer());

    auto result = convertApplicationProfile(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->application_type, Eq(ApplicationType::ReportingAndSupervised));
    EXPECT_THAT(result->is_self_terminating, IsTrue());
}

TEST_F(ConverterTest, ConvertApplicationProfileMissingTypeReturnsError)
{
    RecordProperty("Description", "Missing application_type returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto ap =
        fb::CreateApplicationProfile(fbb, ::flatbuffers::nullopt /*application_type*/, false /*is_self_terminating*/);
    fbb.Finish(ap);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ApplicationProfile>(fbb.GetBufferPointer());

    auto result = convertApplicationProfile(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertApplicationProfileMissingSelfTerminatingReturnsError)
{
    RecordProperty("Description", "Missing is_self_terminating returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto ap =
        fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, ::flatbuffers::nullopt /*is_self_terminating*/);
    fbb.Finish(ap);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ApplicationProfile>(fbb.GetBufferPointer());

    auto result = convertApplicationProfile(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertReadyConditionNullDeath)
{
    RecordProperty("Description", "convertReadyCondition fires an assertion when passed nullptr.");
    EXPECT_DEATH(static_cast<void>(convertReadyCondition(nullptr)), ".*");
}

TEST_F(ConverterTest, ConvertReadyConditionWithProcessState)
{
    RecordProperty("Description", "convertReadyCondition maps process_state correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto rc = fb::CreateReadyCondition(fbb, fb::ProcessState::Terminated);
    fbb.Finish(rc);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ReadyCondition>(fbb.GetBufferPointer());

    auto result = convertReadyCondition(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(*result, ::testing::VariantWith<ProcessState>(ProcessState::Terminated));
}

TEST_F(ConverterTest, ConvertReadyConditionWithFileState)
{
    RecordProperty("Description", "convertReadyCondition maps file_state correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto fs = fb::CreateFileStateDirect(fbb, "/tmp/ready", fb::FileExistenceState::Exists, 0.01 /*polling_interval*/);
    auto rc = fb::CreateReadyCondition(fbb, ::flatbuffers::nullopt /*process_state*/, fs);
    fbb.Finish(rc);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ReadyCondition>(fbb.GetBufferPointer());

    auto result = convertReadyCondition(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(*result, ::testing::VariantWith<FileState>(::testing::Field(&FileState::file_path, Eq("/tmp/ready"))));
}

TEST_F(ConverterTest, ConvertReadyConditionWithBothStatesReturnsError)
{
    RecordProperty(
        "Description", "convertReadyCondition with both process_state and file_state returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto fs = fb::CreateFileStateDirect(fbb, "/tmp/ready", fb::FileExistenceState::Exists);
    auto rc = fb::CreateReadyCondition(fbb, fb::ProcessState::Running, fs);
    fbb.Finish(rc);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ReadyCondition>(fbb.GetBufferPointer());

    auto result = convertReadyCondition(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertReadyConditionWithNeitherStateDeath)
{
    RecordProperty(
        "Description",
        "convertReadyCondition fires an assertion if neither process_state nor file_state is configured, as the "
        "configuration script always defaults one of them.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto rc = fb::CreateReadyCondition(fbb);
    fbb.Finish(rc);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ReadyCondition>(fbb.GetBufferPointer());

    EXPECT_DEATH(static_cast<void>(convertReadyCondition(ptr)), ".*");
}

TEST_F(ConverterTest, ConvertReadyConditionWithInvalidPollingIntervalReturnsError)
{
    RecordProperty("Description", "convertReadyCondition propagates an invalid FileState::polling_interval.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto fs = fb::CreateFileStateDirect(fbb, "/tmp/ready", fb::FileExistenceState::Exists, -1.0 /*polling_interval*/);
    auto rc = fb::CreateReadyCondition(fbb, ::flatbuffers::nullopt /*process_state*/, fs);
    fbb.Finish(rc);
    const auto* ptr = ::flatbuffers::GetRoot<fb::ReadyCondition>(fbb.GetBufferPointer());

    auto result = convertReadyCondition(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertFileExistenceStateMapsDeath)
{
    RecordProperty("Description", "convertFileExistenceState Fires an assertion if an undefined enum is given.");
    EXPECT_DEATH(
        static_cast<void>(convertFileExistenceState(
            static_cast<fb::FileExistenceState>(static_cast<int>(fb::FileExistenceState::MAX) + 1))),
        ".*");
}

TEST_F(ConverterTest, ConvertFileExistenceStateMapsBothValues)
{
    RecordProperty("Description", "convertFileExistenceState maps both enum values correctly.");
    EXPECT_THAT(convertFileExistenceState(fb::FileExistenceState::Exists), Eq(FileExistenceState::Exists));
    EXPECT_THAT(convertFileExistenceState(fb::FileExistenceState::NotExisting), Eq(FileExistenceState::NotExisting));
}

TEST_F(ConverterTest, ConvertFileStateValid)
{
    RecordProperty("Description", "convertFileState maps file_path, an explicit state and polling_interval correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto fs =
        fb::CreateFileStateDirect(fbb, "/tmp/ready", fb::FileExistenceState::NotExisting, 0.3 /*polling_interval*/);
    fbb.Finish(fs);
    const auto* ptr = ::flatbuffers::GetRoot<fb::FileState>(fbb.GetBufferPointer());

    auto result = convertFileState(*ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->file_path, Eq("/tmp/ready"));
    EXPECT_THAT(result->state, Eq(FileExistenceState::NotExisting));
    EXPECT_THAT(result->polling_interval, Eq(std::chrono::milliseconds{300}));
}

TEST_F(ConverterTest, ConvertFileStateDefaultsToExists)
{
    RecordProperty("Description", "convertFileState defaults state to Exists if it is not set.");
    ::flatbuffers::FlatBufferBuilder fbb;
    // state is omitted from the buffer since it matches the schema default
    auto fs = fb::CreateFileStateDirect(fbb, "/tmp/ready", fb::FileExistenceState::Exists, 0.01 /*polling_interval*/);
    fbb.Finish(fs);
    const auto* ptr = ::flatbuffers::GetRoot<fb::FileState>(fbb.GetBufferPointer());

    auto result = convertFileState(*ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->state, Eq(FileExistenceState::Exists));
    EXPECT_THAT(result->polling_interval, Eq(std::chrono::milliseconds{10}));
}

TEST_F(ConverterTest, ConvertFileStateWithoutPollingIntervalDeath)
{
    RecordProperty(
        "Description",
        "convertFileState fires an assertion if polling_interval is not configured, as the configuration script "
        "always defaults it.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto fs = fb::CreateFileStateDirect(fbb, "/tmp/ready");
    fbb.Finish(fs);
    const auto* ptr = ::flatbuffers::GetRoot<fb::FileState>(fbb.GetBufferPointer());

    EXPECT_DEATH(static_cast<void>(convertFileState(*ptr)), ".*");
}

TEST_F(ConverterTest, ConvertFileStateNegativePollingIntervalReturnsError)
{
    RecordProperty("Description", "convertFileState returns InvalidFormat for a negative polling_interval.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto fs = fb::CreateFileStateDirect(fbb, "/tmp/ready", fb::FileExistenceState::Exists, -0.5 /*polling_interval*/);
    fbb.Finish(fs);
    const auto* ptr = ::flatbuffers::GetRoot<fb::FileState>(fbb.GetBufferPointer());

    auto result = convertFileState(*ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertFileStateSubMillisecondPollingIntervalReturnsError)
{
    RecordProperty("Description", "convertFileState returns InvalidFormat for a sub-millisecond polling_interval.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto fs = fb::CreateFileStateDirect(fbb, "/tmp/ready", fb::FileExistenceState::Exists, 0.0001 /*polling_interval*/);
    fbb.Finish(fs);
    const auto* ptr = ::flatbuffers::GetRoot<fb::FileState>(fbb.GetBufferPointer());

    auto result = convertFileState(*ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertSandboxValid)
{
    RecordProperty("Description", "convertSandbox maps all fields including optional ones.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto sec_policy = fbb.CreateString("strict");
    auto supp_gids = fbb.CreateVector(std::vector<int64_t>{100, 200});
    auto sandbox = fb::CreateSandbox(
        fbb,
        1000 /*uid*/,
        1000 /*gid*/,
        supp_gids,
        sec_policy,
        fb::SchedulingPolicy::FIFO,
        50 /*scheduling_priority*/,
        4096 /*max_memory_usage*/,
        80 /*max_cpu_usage*/);
    fbb.Finish(sandbox);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Sandbox>(fbb.GetBufferPointer());

    auto result = convertSandbox(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->uid, Eq(1000U));
    EXPECT_THAT(result->gid, Eq(1000U));
    EXPECT_THAT(result->supplementary_group_ids, Eq(std::vector<gid_t>{100, 200}));
    ASSERT_THAT(result->security_policy.has_value(), IsTrue());
    EXPECT_THAT(result->security_policy.value(), Eq("strict"));
    EXPECT_THAT(result->scheduling_policy, Eq(SCHED_FIFO));
    EXPECT_THAT(result->scheduling_priority, Eq(50));
    ASSERT_THAT(result->max_memory_usage.has_value(), IsTrue());
    EXPECT_THAT(result->max_memory_usage.value(), Eq(4096U));
    ASSERT_THAT(result->max_cpu_usage.has_value(), IsTrue());
    EXPECT_THAT(result->max_cpu_usage.value(), Eq(80U));
}

TEST_F(ConverterTest, ConvertSandboxMissingUidReturnsError)
{
    RecordProperty("Description", "Sandbox without uid returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto sandbox = fb::CreateSandbox(
        fbb,
        ::flatbuffers::nullopt /*uid*/,
        0 /*gid*/,
        0 /*supplementary_group_ids*/,
        0 /*security_policy*/,
        fb::SchedulingPolicy::OTHER,
        0 /*scheduling_priority*/);
    fbb.Finish(sandbox);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Sandbox>(fbb.GetBufferPointer());

    auto result = convertSandbox(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertSandboxMissingGidReturnsError)
{
    RecordProperty("Description", "Sandbox without gid returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto sandbox = fb::CreateSandbox(
        fbb,
        0 /*uid*/,
        ::flatbuffers::nullopt /*gid*/,
        0 /*supplementary_group_ids*/,
        0 /*security_policy*/,
        fb::SchedulingPolicy::OTHER,
        0 /*scheduling_priority*/);
    fbb.Finish(sandbox);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Sandbox>(fbb.GetBufferPointer());

    auto result = convertSandbox(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertSandboxMissingSchedulingPolicyReturnsError)
{
    RecordProperty("Description", "Sandbox without scheduling_policy returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto sandbox = fb::CreateSandbox(
        fbb,
        0 /*uid*/,
        0 /*gid*/,
        0 /*supplementary_group_ids*/,
        0 /*security_policy*/,
        ::flatbuffers::nullopt /*scheduling_policy*/,
        0 /*scheduling_priority*/);
    fbb.Finish(sandbox);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Sandbox>(fbb.GetBufferPointer());

    auto result = convertSandbox(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertSandboxMissingSchedulingPriorityReturnsError)
{
    RecordProperty("Description", "Sandbox without scheduling_priority returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto sandbox = fb::CreateSandbox(
        fbb,
        0 /*uid*/,
        0 /*gid*/,
        0 /*supplementary_group_ids*/,
        0 /*security_policy*/,
        fb::SchedulingPolicy::OTHER,
        ::flatbuffers::nullopt /*scheduling_priority*/);
    fbb.Finish(sandbox);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Sandbox>(fbb.GetBufferPointer());

    auto result = convertSandbox(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertSandboxUidOutOfRangeReturnsError)
{
    RecordProperty("Description", "Sandbox uid exceeding uid_t range returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto sandbox = fb::CreateSandbox(
        fbb,
        std::numeric_limits<int64_t>::max() /*uid*/,
        1000 /*gid*/,
        0 /*supplementary_group_ids*/,
        0 /*security_policy*/,
        fb::SchedulingPolicy::OTHER,
        0 /*scheduling_priority*/);
    fbb.Finish(sandbox);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Sandbox>(fbb.GetBufferPointer());

    auto result = convertSandbox(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertSandboxSupplementaryGidOutOfRangeReturnsError)
{
    RecordProperty("Description", "Supplementary gid exceeding gid_t range returns InvalidFormat.");

    ::flatbuffers::FlatBufferBuilder fbb;
    auto supp_gids = fbb.CreateVector(std::vector<int64_t>{100, std::numeric_limits<int64_t>::max()});
    auto sandbox = fb::CreateSandbox(
        fbb,
        1000 /*uid*/,
        1000 /*gid*/,
        supp_gids,
        0 /*security_policy*/,
        fb::SchedulingPolicy::OTHER,
        0 /*scheduling_priority*/);
    fbb.Finish(sandbox);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Sandbox>(fbb.GetBufferPointer());

    auto result = convertSandbox(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertDeploymentConfigValid)
{
    RecordProperty("Description", "convertDeploymentConfig maps all fields correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto bin_dir = fbb.CreateString("/opt/bin");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb,
        1500 /*ready_timeout_ms*/,
        2500 /*shutdown_timeout_ms*/,
        0 /*environmental_variables*/,
        bin_dir,
        work_dir,
        0 /*ready_recovery_action*/,
        0 /*recovery_action*/,
        sandbox);
    fbb.Finish(deploy);
    const auto* ptr = ::flatbuffers::GetRoot<fb::DeploymentConfig>(fbb.GetBufferPointer());

    auto result = convertDeploymentConfig(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->ready_timeout_ms, Eq(1500U));
    EXPECT_THAT(result->shutdown_timeout_ms, Eq(2500U));
    EXPECT_THAT(result->bin_dir, Eq("/opt/bin"));
    EXPECT_THAT(result->working_dir, Eq("/tmp"));
    EXPECT_THAT(result->ready_recovery_action.has_value(), IsFalse());
    EXPECT_THAT(result->recovery_action.has_value(), IsFalse());
}

TEST_F(ConverterTest, ConvertDeploymentConfigMissingReadyTimeoutReturnsError)
{
    RecordProperty("Description", "Missing ready_timeout_ms returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb,
        ::flatbuffers::nullopt /*ready_timeout_ms*/,
        1000 /*shutdown_timeout_ms*/,
        0 /*environmental_variables*/,
        bin_dir,
        work_dir,
        0 /*ready_recovery_action*/,
        0 /*recovery_action*/,
        sandbox);
    fbb.Finish(deploy);
    const auto* ptr = ::flatbuffers::GetRoot<fb::DeploymentConfig>(fbb.GetBufferPointer());

    auto result = convertDeploymentConfig(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertDeploymentConfigMissingShutdownTimeoutReturnsError)
{
    RecordProperty("Description", "Missing shutdown_timeout_ms returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb,
        1000 /*ready_timeout_ms*/,
        ::flatbuffers::nullopt /*shutdown_timeout_ms*/,
        0 /*environmental_variables*/,
        bin_dir,
        work_dir,
        0 /*ready_recovery_action*/,
        0 /*recovery_action*/,
        sandbox);
    fbb.Finish(deploy);
    const auto* ptr = ::flatbuffers::GetRoot<fb::DeploymentConfig>(fbb.GetBufferPointer());

    auto result = convertDeploymentConfig(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertComponentValid)
{
    RecordProperty("Description", "convertComponent maps name, description, and nested structs.");
    ::flatbuffers::FlatBufferBuilder fbb;

    auto app_profile = fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false /*is_self_terminating*/);
    auto bin_name = fbb.CreateString("my_binary");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(
        fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);

    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb,
        1000 /*ready_timeout_ms*/,
        1000 /*shutdown_timeout_ms*/,
        0 /*environmental_variables*/,
        bin_dir,
        work_dir,
        0 /*ready_recovery_action*/,
        0 /*recovery_action*/,
        sandbox);

    auto comp_name = fbb.CreateString("TestComp");
    auto comp_desc = fbb.CreateString("A test component");
    auto component = fb::CreateComponent(fbb, comp_name, comp_desc, comp_props, deploy);
    fbb.Finish(component);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Component>(fbb.GetBufferPointer());

    auto result = convertComponent(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->name, Eq("TestComp"));
    EXPECT_THAT(result->description, Eq("A test component"));
    EXPECT_THAT(result->component_properties.binary_name, Eq("my_binary"));
    EXPECT_THAT(result->deployment_config.ready_timeout_ms, Eq(1000U));
}

TEST_F(ConverterTest, ConvertComponentsValid)
{
    RecordProperty("Description", "convertComponents maps multiple components in order.");
    ::flatbuffers::FlatBufferBuilder fbb;

    auto build_comp = [&](const char* name) {
        auto app_profile =
            fb::CreateApplicationProfile(fbb, fb::ApplicationType::Native, false /*is_self_terminating*/);
        auto bin_name = fbb.CreateString(name);
        auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
        auto comp_props = fb::CreateComponentProperties(
            fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);
        auto bin_dir = fbb.CreateString("/opt");
        auto work_dir = fbb.CreateString("/tmp");
        auto sandbox = buildDefaultSandbox(fbb);
        auto deploy = fb::CreateDeploymentConfig(
            fbb,
            1000 /*ready_timeout_ms*/,
            1000 /*shutdown_timeout_ms*/,
            0 /*environmental_variables*/,
            bin_dir,
            work_dir,
            0 /*ready_recovery_action*/,
            0 /*recovery_action*/,
            sandbox);
        auto comp_name = fbb.CreateString(name);
        return fb::CreateComponent(fbb, comp_name, 0 /*description*/, comp_props, deploy);
    };

    auto comp_a = build_comp("CompA");
    auto comp_b = build_comp("CompB");
    auto comp_c = build_comp("CompC");
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp_a, comp_b, comp_c});
    fbb.Finish(comps);
    const auto* ptr =
        ::flatbuffers::GetRoot<::flatbuffers::Vector<::flatbuffers::Offset<fb::Component>>>(fbb.GetBufferPointer());

    auto result = convertComponents(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->size(), Eq(3U));
    EXPECT_THAT((*result)[0].name, Eq("CompA"));
    EXPECT_THAT((*result)[1].name, Eq("CompB"));
    EXPECT_THAT((*result)[2].name, Eq("CompC"));
}

TEST_F(ConverterTest, ConvertComponentsWithInvalidComponentReturnsError)
{
    RecordProperty("Description", "convertComponents returns error when a component has invalid fields.");
    ::flatbuffers::FlatBufferBuilder fbb;

    auto app_profile =
        fb::CreateApplicationProfile(fbb, ::flatbuffers::nullopt /*application_type*/, false /*is_self_terminating*/);
    auto bin_name = fbb.CreateString("bad_bin");
    auto ready_cond = fb::CreateReadyCondition(fbb, fb::ProcessState::Running);
    auto comp_props = fb::CreateComponentProperties(
        fbb, bin_name, app_profile, 0 /*depends_on*/, 0 /*process_arguments*/, ready_cond);
    auto bin_dir = fbb.CreateString("/opt");
    auto work_dir = fbb.CreateString("/tmp");
    auto sandbox = buildDefaultSandbox(fbb);
    auto deploy = fb::CreateDeploymentConfig(
        fbb,
        1000 /*ready_timeout_ms*/,
        1000 /*shutdown_timeout_ms*/,
        0 /*environmental_variables*/,
        bin_dir,
        work_dir,
        0 /*ready_recovery_action*/,
        0 /*recovery_action*/,
        sandbox);
    auto comp_name = fbb.CreateString("BadComp");
    auto comp = fb::CreateComponent(fbb, comp_name, 0 /*description*/, comp_props, deploy);
    auto comps = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::Component>>{comp});
    fbb.Finish(comps);
    const auto* ptr =
        ::flatbuffers::GetRoot<::flatbuffers::Vector<::flatbuffers::Offset<fb::Component>>>(fbb.GetBufferPointer());

    auto result = convertComponents(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertRunTargetsValid)
{
    RecordProperty("Description", "convertRunTargets maps multiple run targets in order.");
    ::flatbuffers::FlatBufferBuilder fbb;

    auto build_rt = [&](const char* name, const char* recovery_target) {
        auto switch_target = fbb.CreateString(recovery_target);
        auto switch_action = fb::CreateSwitchRunTargetAction(fbb, switch_target);
        auto rt_name = fbb.CreateString(name);
        return fb::CreateRunTarget(
            fbb, rt_name, 0 /*description*/, 0 /*depends_on*/, 1000 /*transition_timeout_ms*/, switch_action);
    };

    auto rt_a = build_rt("Startup", "SafeState");
    auto rt_b = build_rt("Running", "Fallback");
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt_a, rt_b});
    fbb.Finish(rts);
    const auto* ptr =
        ::flatbuffers::GetRoot<::flatbuffers::Vector<::flatbuffers::Offset<fb::RunTarget>>>(fbb.GetBufferPointer());

    auto result = convertRunTargets(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->size(), Eq(2U));
    EXPECT_THAT((*result)[0].name, Eq("Startup"));
    EXPECT_THAT((*result)[0].recovery_action.run_target, Eq("SafeState"));
    EXPECT_THAT((*result)[1].name, Eq("Running"));
    EXPECT_THAT((*result)[1].recovery_action.run_target, Eq("Fallback"));
}

TEST_F(ConverterTest, ConvertRunTargetsWithInvalidRunTargetReturnsError)
{
    RecordProperty("Description", "convertRunTargets returns error when a run target has invalid fields.");
    ::flatbuffers::FlatBufferBuilder fbb;

    auto switch_target = fbb.CreateString("SafeState");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, switch_target);
    auto rt_name = fbb.CreateString("BadRT");
    auto rt = fb::CreateRunTarget(
        fbb,
        rt_name,
        0 /*description*/,
        0 /*depends_on*/,
        ::flatbuffers::nullopt /*transition_timeout_ms*/,
        switch_action);
    auto rts = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::RunTarget>>{rt});
    fbb.Finish(rts);
    const auto* ptr =
        ::flatbuffers::GetRoot<::flatbuffers::Vector<::flatbuffers::Offset<fb::RunTarget>>>(fbb.GetBufferPointer());

    auto result = convertRunTargets(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertRunTargetValid)
{
    RecordProperty("Description", "convertRunTarget maps all fields correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;

    auto switch_target = fbb.CreateString("SafeState");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, switch_target);
    auto rt_name = fbb.CreateString("Startup");
    auto rt_desc = fbb.CreateString("Initial state");
    auto rt_dep = fbb.CreateString("component_a");
    auto rt_deps = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{rt_dep});
    auto rt = fb::CreateRunTarget(fbb, rt_name, rt_desc, rt_deps, 5000 /*transition_timeout_ms*/, switch_action);
    fbb.Finish(rt);
    const auto* ptr = ::flatbuffers::GetRoot<fb::RunTarget>(fbb.GetBufferPointer());

    auto result = convertRunTarget(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->name, Eq("Startup"));
    EXPECT_THAT(result->description, Eq("Initial state"));
    ASSERT_THAT(result->depends_on.size(), Eq(1U));
    EXPECT_THAT(result->depends_on[0], Eq("component_a"));
    EXPECT_THAT(result->transition_timeout_ms, Eq(5000U));
    EXPECT_THAT(result->recovery_action.run_target, Eq("SafeState"));
}

TEST_F(ConverterTest, ConvertRunTargetMissingTransitionTimeoutReturnsError)
{
    RecordProperty("Description", "Missing transition_timeout_ms returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto switch_target = fbb.CreateString("SafeState");
    auto switch_action = fb::CreateSwitchRunTargetAction(fbb, switch_target);
    auto rt_name = fbb.CreateString("Startup");
    auto rt = fb::CreateRunTarget(
        fbb,
        rt_name,
        0 /*description*/,
        0 /*depends_on*/,
        ::flatbuffers::nullopt /*transition_timeout_ms*/,
        switch_action);
    fbb.Finish(rt);
    const auto* ptr = ::flatbuffers::GetRoot<fb::RunTarget>(fbb.GetBufferPointer());

    auto result = convertRunTarget(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertFallbackRunTargetValid)
{
    RecordProperty("Description", "convertFallbackRunTarget maps all fields correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto desc = fbb.CreateString("Fallback state");
    auto dep = fbb.CreateString("critical_comp");
    auto deps = fbb.CreateVector(std::vector<::flatbuffers::Offset<::flatbuffers::String>>{dep});
    auto frt = fb::CreateFallbackRunTarget(fbb, desc, deps, 10000 /*transition_timeout_ms*/);
    fbb.Finish(frt);
    const auto* ptr = ::flatbuffers::GetRoot<fb::FallbackRunTarget>(fbb.GetBufferPointer());

    auto result = convertFallbackRunTarget(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->description, Eq("Fallback state"));
    ASSERT_THAT(result->depends_on.size(), Eq(1U));
    EXPECT_THAT(result->depends_on[0], Eq("critical_comp"));
    EXPECT_THAT(result->transition_timeout_ms, Eq(10000U));
}

TEST_F(ConverterTest, ConvertFallbackRunTargetMissingTimeoutReturnsError)
{
    RecordProperty("Description", "Missing transition_timeout_ms returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto frt = fb::CreateFallbackRunTarget(fbb);
    fbb.Finish(frt);
    const auto* ptr = ::flatbuffers::GetRoot<fb::FallbackRunTarget>(fbb.GetBufferPointer());

    auto result = convertFallbackRunTarget(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertAliveSupervisionNullReturnsDefault)
{
    RecordProperty("Description", "convertAliveSupervision with nullptr returns default.");
    auto result = convertAliveSupervision(nullptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->evaluation_cycle_ms, Eq(0U));
}

TEST_F(ConverterTest, ConvertAliveSupervisionValid)
{
    RecordProperty("Description", "convertAliveSupervision maps evaluation_cycle_ms correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto as = fb::CreateAliveSupervision(fbb, 250 /*evaluation_cycle_ms*/);
    fbb.Finish(as);
    const auto* ptr = ::flatbuffers::GetRoot<fb::AliveSupervision>(fbb.GetBufferPointer());

    auto result = convertAliveSupervision(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->evaluation_cycle_ms, Eq(250U));
}

TEST_F(ConverterTest, ConvertAliveSupervisionMissingCycleReturnsError)
{
    RecordProperty("Description", "Missing evaluation_cycle_ms returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto as = fb::CreateAliveSupervision(fbb);
    fbb.Finish(as);
    const auto* ptr = ::flatbuffers::GetRoot<fb::AliveSupervision>(fbb.GetBufferPointer());

    auto result = convertAliveSupervision(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertWatchdogNullReturnsNullopt)
{
    RecordProperty("Description", "convertWatchdog with nullptr returns nullopt.");
    auto result = convertWatchdog(nullptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    EXPECT_THAT(result->has_value(), IsFalse());
}

TEST_F(ConverterTest, ConvertWatchdogValid)
{
    RecordProperty("Description", "convertWatchdog maps all fields correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto dev_path = fbb.CreateString("/dev/watchdog0");
    auto wd = fb::CreateWatchdog(fbb, dev_path, 30000 /*max_timeout_ms*/, true /*deactivate*/, false /*magic_close*/);
    fbb.Finish(wd);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Watchdog>(fbb.GetBufferPointer());

    auto result = convertWatchdog(ptr);
    ASSERT_THAT(result.has_value(), IsTrue());
    ASSERT_THAT(result->has_value(), IsTrue());
    EXPECT_THAT(result->value().device_file_path, Eq("/dev/watchdog0"));
    EXPECT_THAT(result->value().max_timeout_ms, Eq(30000U));
    EXPECT_THAT(result->value().deactivate_on_shutdown, IsTrue());
    EXPECT_THAT(result->value().require_magic_close, IsFalse());
}

TEST_F(ConverterTest, ConvertWatchdogMissingMaxTimeoutReturnsError)
{
    RecordProperty("Description", "Missing max_timeout_ms returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto dev_path = fbb.CreateString("/dev/watchdog0");
    auto wd = fb::CreateWatchdog(
        fbb,
        dev_path,
        ::flatbuffers::nullopt /*max_timeout_ms*/,
        true /*deactivate_on_shutdown*/,
        false /*require_magic_close*/);
    fbb.Finish(wd);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Watchdog>(fbb.GetBufferPointer());

    auto result = convertWatchdog(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertWatchdogMissingDeactivateReturnsError)
{
    RecordProperty("Description", "Missing deactivate_on_shutdown returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto dev_path = fbb.CreateString("/dev/watchdog0");
    auto wd = fb::CreateWatchdog(
        fbb,
        dev_path,
        30000 /*max_timeout_ms*/,
        ::flatbuffers::nullopt /*deactivate_on_shutdown*/,
        false /*require_magic_close*/);
    fbb.Finish(wd);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Watchdog>(fbb.GetBufferPointer());

    auto result = convertWatchdog(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertWatchdogMissingMagicCloseReturnsError)
{
    RecordProperty("Description", "Missing require_magic_close returns InvalidFormat.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto dev_path = fbb.CreateString("/dev/watchdog0");
    auto wd = fb::CreateWatchdog(
        fbb,
        dev_path,
        30000 /*max_timeout_ms*/,
        true /*deactivate_on_shutdown*/,
        ::flatbuffers::nullopt /*require_magic_close*/);
    fbb.Finish(wd);
    const auto* ptr = ::flatbuffers::GetRoot<fb::Watchdog>(fbb.GetBufferPointer());

    auto result = convertWatchdog(ptr);
    ASSERT_THAT(result.has_value(), IsFalse());
    EXPECT_THAT(result.error(), Eq(IConfigLoader::Error::InvalidFormat));
}

TEST_F(ConverterTest, ConvertEnvironmentalVariables)
{
    RecordProperty("Description", "convertEnvironmentalVariables maps key-value pairs correctly.");
    ::flatbuffers::FlatBufferBuilder fbb;
    auto k1 = fbb.CreateString("PATH");
    auto v1 = fbb.CreateString("/usr/bin");
    auto ev1 = fb::CreateEnvironmentalVariable(fbb, k1, v1);
    auto k2 = fbb.CreateString("HOME");
    auto v2 = fbb.CreateString("/root");
    auto ev2 = fb::CreateEnvironmentalVariable(fbb, k2, v2);
    auto env_vars = fbb.CreateVector(std::vector<::flatbuffers::Offset<fb::EnvironmentalVariable>>{ev1, ev2});
    fbb.Finish(env_vars);
    const auto* ptr = ::flatbuffers::GetRoot<::flatbuffers::Vector<::flatbuffers::Offset<fb::EnvironmentalVariable>>>(
        fbb.GetBufferPointer());

    auto result = convertEnvironmentalVariables(ptr);
    ASSERT_THAT(result.size(), Eq(2U));
    auto it = result.begin();
    EXPECT_THAT(it->key(), Eq("PATH"));
    EXPECT_THAT(it->value(), Eq("/usr/bin"));
    ++it;
    EXPECT_THAT(it->key(), Eq("HOME"));
    EXPECT_THAT(it->value(), Eq("/root"));
}

TEST_F(ConverterTest, ConvertEnvironmentalVariablesNullReturnsEmpty)
{
    RecordProperty("Description", "convertEnvironmentalVariables with nullptr returns empty Environment.");
    auto result = convertEnvironmentalVariables(nullptr);
    EXPECT_THAT(result.size(), Eq(0U));
}

}  // namespace
}  // namespace score::mw::lifecycle::internal::configuration
