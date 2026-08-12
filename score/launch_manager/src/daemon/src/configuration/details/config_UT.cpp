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
#include "score/mw/launch_manager/configuration/config.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <string_view>

namespace score::mw::lifecycle::internal::configuration
{
namespace
{

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::StrEq;

class EnvironmentVariableTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(EnvironmentVariableTest, KeyAndValueAreAccessible)
{
    RecordProperty("Description", "Key and value are correctly split from the internal key=value storage.");

    EnvironmentVariable ev{"PATH", "/usr/bin"};

    EXPECT_THAT(ev.key(), Eq("PATH"));
    EXPECT_THAT(ev.value(), Eq("/usr/bin"));
}

TEST_F(EnvironmentVariableTest, CStrReturnsKeyEqualsValue)
{
    RecordProperty("Description", "c_str() returns the full key=value string.");

    EnvironmentVariable ev{"HOME", "/root"};

    EXPECT_THAT(ev.c_str(), StrEq("HOME=/root"));
}

TEST_F(EnvironmentVariableTest, EmptyValue)
{
    RecordProperty("Description", "Empty value is handled correctly.");

    EnvironmentVariable ev{"KEY", ""};

    EXPECT_THAT(ev.key(), Eq("KEY"));
    EXPECT_THAT(ev.value(), Eq(""));
    EXPECT_THAT(ev.c_str(), StrEq("KEY="));
}

TEST_F(EnvironmentVariableTest, ValueContainingEquals)
{
    RecordProperty("Description", "A value containing '=' is preserved correctly.");

    EnvironmentVariable ev{"OPTS", "a=1,b=2"};

    EXPECT_THAT(ev.key(), Eq("OPTS"));
    EXPECT_THAT(ev.value(), Eq("a=1,b=2"));
    EXPECT_THAT(ev.c_str(), StrEq("OPTS=a=1,b=2"));
}

class EnvironmentTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(EnvironmentTest, DefaultConstructedIsEmpty)
{
    RecordProperty("Description", "A default-constructed Environment has size 0 and begin == end.");

    Environment env;

    EXPECT_THAT(env.size(), Eq(0U));
    EXPECT_THAT(env.begin(), Eq(env.end()));
}

TEST_F(EnvironmentTest, AddIncreasesSize)
{
    RecordProperty("Description", "Adding entries increases size and they are iterable.");

    Environment env;
    env.add("A", "1");
    env.add("B", "2");

    EXPECT_THAT(env.size(), Eq(2U));
    auto it = env.begin();
    EXPECT_THAT(it->key(), Eq("A"));
    EXPECT_THAT(it->value(), Eq("1"));
    ++it;
    EXPECT_THAT(it->key(), Eq("B"));
    EXPECT_THAT(it->value(), Eq("2"));
    ++it;
    EXPECT_THAT(it, Eq(env.end()));
}

TEST_F(EnvironmentTest, EnvpReturnsNullTerminatedArray)
{
    RecordProperty("Description", "envp() returns a null-terminated array suitable for execve.");

    Environment env;
    env.add("KEY", "val");

    char* const* envp = env.envp();

    ASSERT_THAT(envp, NotNull());
    EXPECT_THAT(envp[0], StrEq("KEY=val"));
    EXPECT_THAT(envp[1], IsNull());
}

TEST_F(EnvironmentTest, EnvpWithMultipleEntries)
{
    RecordProperty("Description", "envp() contains all entries in order followed by nullptr.");

    Environment env;
    env.add("A", "1");
    env.add("B", "2");
    env.add("C", "3");

    char* const* envp = env.envp();

    EXPECT_THAT(envp[0], StrEq("A=1"));
    EXPECT_THAT(envp[1], StrEq("B=2"));
    EXPECT_THAT(envp[2], StrEq("C=3"));
    EXPECT_THAT(envp[3], IsNull());
}

TEST_F(EnvironmentTest, ReserveAndAddAvoidReallocation)
{
    RecordProperty("Description", "reserve() followed by add() produces correct entries and envp.");

    Environment env;
    env.reserve(3);
    env.add("X", "10");
    env.add("Y", "20");
    env.add("Z", "30");

    EXPECT_THAT(env.size(), Eq(3U));

    char* const* envp = env.envp();
    EXPECT_THAT(envp[0], StrEq("X=10"));
    EXPECT_THAT(envp[1], StrEq("Y=20"));
    EXPECT_THAT(envp[2], StrEq("Z=30"));
    EXPECT_THAT(envp[3], IsNull());
}

TEST_F(EnvironmentTest, MoveConstructorPreservesEntries)
{
    RecordProperty("Description", "Move-constructed Environment has the original entries and valid envp.");

    Environment original;
    original.add("FOO", "bar");
    original.add("BAZ", "qux");

    Environment moved{std::move(original)};

    EXPECT_THAT(moved.size(), Eq(2U));
    auto it = moved.begin();
    EXPECT_THAT(it->key(), Eq("FOO"));
    EXPECT_THAT(it->value(), Eq("bar"));
    ++it;
    EXPECT_THAT(it->key(), Eq("BAZ"));
    EXPECT_THAT(it->value(), Eq("qux"));

    char* const* envp = moved.envp();
    EXPECT_THAT(envp[0], StrEq("FOO=bar"));
    EXPECT_THAT(envp[1], StrEq("BAZ=qux"));
    EXPECT_THAT(envp[2], IsNull());
}

TEST_F(EnvironmentTest, MoveAssignmentPreservesEntries)
{
    RecordProperty("Description", "Move-assigned Environment has the original entries and valid envp.");

    Environment original;
    original.add("K", "V");

    Environment target;
    target = std::move(original);

    EXPECT_THAT(target.size(), Eq(1U));
    EXPECT_THAT(target.begin()->key(), Eq("K"));

    char* const* envp = target.envp();
    EXPECT_THAT(envp[0], StrEq("K=V"));
    EXPECT_THAT(envp[1], IsNull());
}

TEST_F(EnvironmentTest, SsoLengthStringSurvivesReallocation)
{
    RecordProperty("Description", "Short strings within SSO threshold have valid envp after reallocation.");

    Environment env;
    for (int i = 0; i < 100; ++i)
    {
        env.add("K", std::to_string(i));
    }

    char* const* envp = env.envp();
    EXPECT_THAT(envp[0], StrEq("K=0"));
    EXPECT_THAT(envp[99], StrEq("K=99"));
    EXPECT_THAT(envp[100], IsNull());
}

TEST_F(EnvironmentTest, EmptyEnvironmentEnvpIsNullTerminated)
{
    RecordProperty("Description", "envp() on an empty Environment returns a null-terminated array.");

    Environment env;

    char* const* envp = env.envp();

    ASSERT_THAT(envp, NotNull());
    EXPECT_THAT(envp[0], IsNull());
}

TEST_F(EnvironmentTest, RangeBasedForLoopWorks)
{
    RecordProperty("Description", "Environment supports range-based for loop.");

    Environment env;
    env.add("A", "1");
    env.add("B", "2");

    std::size_t count = 0;
    for (const auto& ev : env)
    {
        EXPECT_THAT(ev.c_str(), NotNull());
        ++count;
    }
    EXPECT_THAT(count, Eq(2U));
}

}  // namespace
}  // namespace score::mw::lifecycle::internal::configuration
