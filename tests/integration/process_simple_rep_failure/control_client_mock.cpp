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
#include <gtest/gtest.h>

#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/control_client.h>
#include <score/mw/lifecycle/report_running.h>

// Given a correct configuration with:
//   - An initial Run Target named "Startup" containing component named
//   "control_client_mock"
//   - A Run Target named "run_target_app_does_report_krunning_in_time"
//   containing "control_client_mock" and
//   "component_does_report_krunning_in_time"
//   - A Run Target named "run_target_app_does_not_report_krunning_in_time"
//   containing "control_client_mock" and
//   "component_does_not_report_krunning_in_time"

TEST(RecoveryActionSimpleRepFailure, ControlClientMock) {
  score::mw::lifecycle::ControlClient client;

  ASSERT_TRUE(check_clean({test_end_location, fallback_file}));

  // Establish communication with launch manager
  TEST_STEP("Report running from ControlClientMock") {
    score::mw::lifecycle::report_running();
  }
  // Start the run target run_target_app_does_report_krunning_in_time
  TEST_STEP("Activate RunTarget run_target_app_does_report_krunning_in_time") {
    score::cpp::stop_token stop_token;
    auto result =
        client.ActivateRunTarget("run_target_app_does_report_krunning_in_time")
            .Get(stop_token);
    EXPECT_TRUE(result.has_value())
        << "Activating target run_target_app_does_report_krunning_in_time "
           "failed: "
        << result.error().Message();
  }
  // Limitation: we cannot wait for the transition to fallback to complete
  sleep(1);
  // Then, the LM should continue without triggering the fallback
  TEST_STEP("Verify fallback run target has not been activated") {
    EXPECT_FALSE(std::filesystem::exists(fallback_file))
        << "Fallback run target should have not been activated";
  }
  // Start the run target run_target_app_does_not_report_krunning_in_time
  TEST_STEP(
      "Activate RunTarget run_target_app_does_not_report_krunning_in_time") {
    score::cpp::stop_token stop_token;
    auto result = client
                      .ActivateRunTarget(
                          "run_target_app_does_not_report_krunning_in_time")
                      .Get(stop_token);
    EXPECT_FALSE(result.has_value())
        << "Activating target run_target_app_does_not_report_krunning_in_time "
           "did not fail as expected.";
  }
  // Limitation: we cannot wait for the transition to fallback to complete
  sleep(1);
  // Then, the LM should exhaust retries and trigger the fallback
  TEST_STEP("Verify fallback run target was activated") {
    EXPECT_TRUE(std::filesystem::exists(fallback_file))
        << "Fallback run target should have been activated";
  }

  TEST_STEP("Activate RunTarget Off") { client.ActivateRunTarget("Off"); }
}

int main() {
  return TestRunner(__FILE__, TerminationBehavior::kContinue,
                    TerminationNotification::kTestEnd)
      .RunTests();
}
