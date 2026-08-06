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

#include "score/mw/health/deadline_monitor.h"
#include "score/mw/health/health_monitor_builder.h"

#include <gtest/gtest.h>

using namespace score::mw::health;

class DeadlineMonitorConfigurationFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");
    }
};

TEST_F(DeadlineMonitorConfigurationFixture, New_Succeeds)
{
    RecordProperty("Description", "Object successfully constructed.");
    DeadlineMonitorConfiguration cfg;
}

TEST_F(DeadlineMonitorConfigurationFixture, AddDeadline_Succeeds)
{
    RecordProperty("Description", "Deadline successfully added.");
    using namespace std::chrono_literals;
    TimeRange range{50ms, 150ms};
    DeadlineMonitorConfiguration cfg;
    cfg.AddDeadline(Tag("deadline"), range);
}

class DeadlineMonitorFixture : public ::testing::Test
{
  protected:
    std::unique_ptr<DeadlineMonitor> deadline_monitor_;

    void SetUp() override
    {
        RecordProperty("TestType", "interface-test");
        RecordProperty("DerivationTechnique", "explorative-testing");

        using namespace std::chrono_literals;
        TimeRange range{50ms, 150ms};
        DeadlineMonitorConfiguration cfg;
        cfg.AddDeadline(Tag("deadline"), range);

        auto hmon_build_result{
            HealthMonitorBuilder::Create()->AddDeadlineMonitor(Tag("deadline_monitor"), std::move(cfg)).Build()};
        ASSERT_TRUE(hmon_build_result.has_value());
        auto hmon{std::move(hmon_build_result.value())};

        auto get_deadline_monitor_result{hmon->GetDeadlineMonitor(Tag("deadline_monitor"))};
        ASSERT_TRUE(get_deadline_monitor_result.has_value());
        deadline_monitor_ = std::move(get_deadline_monitor_result.value());
    }
};

TEST_F(DeadlineMonitorFixture, Start_Succeeds)
{
    RecordProperty("Description", "Deadline successfully started and stopped.");
    auto deadline_result{deadline_monitor_->GetDeadline(Tag("deadline"))};
    ASSERT_TRUE(deadline_result.has_value());
    auto deadline = std::move(deadline_result.value());

    auto start_result{deadline->Start()};
    ASSERT_TRUE(start_result.has_value());
    auto stop_result{deadline->Stop()};
    ASSERT_TRUE(stop_result.has_value());
}

TEST_F(DeadlineMonitorFixture, DeadlineGuard_StartsAndStops)
{
    RecordProperty("Description", "DeadlineGuard starts the deadline on construction and stops it on destruction.");
    auto deadline_result{deadline_monitor_->GetDeadline(Tag("deadline"))};
    ASSERT_TRUE(deadline_result.has_value());
    auto deadline = std::move(deadline_result.value());

    {
        DeadlineGuard guard(*deadline);
    }
}

TEST_F(DeadlineMonitorFixture, Start_AlreadyRunning)
{
    RecordProperty("Description", "Deadline failed to start twice.");
    auto deadline_result{deadline_monitor_->GetDeadline(Tag("deadline"))};
    ASSERT_TRUE(deadline_result.has_value());
    auto deadline = std::move(deadline_result.value());

    auto first_start{deadline->Start()};
    ASSERT_TRUE(first_start.has_value());

    auto second_start{deadline->Start()};
    ASSERT_FALSE(second_start.has_value());
    ASSERT_EQ(second_start.error(), Error::kFailed);

    auto stop_result{deadline->Stop()};
    ASSERT_TRUE(stop_result.has_value());
}

TEST_F(DeadlineMonitorFixture, Start_Unknown)
{
    RecordProperty("Description", "Deadline failed to start due to unknown tag.");
    auto deadline_result{deadline_monitor_->GetDeadline(Tag("unknown"))};
    ASSERT_FALSE(deadline_result.has_value());
    ASSERT_EQ(deadline_result.error(), Error::kNotFound);
}
