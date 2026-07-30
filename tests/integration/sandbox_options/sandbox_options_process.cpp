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
#include <sched.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "tests/integration/sandbox_options/verify_sandbox.hpp"
#include "tests/utils/test_helper/test_helper.hpp"
#include <score/mw/lifecycle/report_running.h>

namespace
{

using sandbox_options::ExpectedValues;

// Expected sandbox option values, supplied to this process on the command line.
//
// The launch manager applies the sandbox options from sandbox_options.json and passes the same
// values to the managed process via 'process_arguments', so the test verifies the applied state
// against the configured expectation without duplicating any literals here.
//
// Populated from argv in main() before the tests run.
ExpectedValues expected;

int parse_policy(const std::string& name)
{
    if (name == "SCHED_FIFO")
    {
        return SCHED_FIFO;
    }
    if (name == "SCHED_RR")
    {
        return SCHED_RR;
    }
    if (name == "SCHED_OTHER")
    {
        return SCHED_OTHER;
    }
    return -1;
}

// Parse a comma separated list of group ids, e.g. "123,321,456".
std::vector<gid_t> parse_groups(const std::string& csv)
{
    std::vector<gid_t> groups;
    std::stringstream stream(csv);
    std::string token;
    while (std::getline(stream, token, ','))
    {
        if (!token.empty())
        {
            groups.push_back(static_cast<gid_t>(std::stoul(token)));
        }
    }
    return groups;
}

// If 'arg' has the form "--<key>=<value>", store the value in 'value' and return true.
bool match_option(const std::string& arg, const std::string_view key, std::string& value)
{
    const std::string prefix = "--" + std::string(key) + "=";
    if (arg.rfind(prefix, 0) == 0)
    {
        value = arg.substr(prefix.size());
        return true;
    }
    return false;
}

// Populate 'out' from the command line arguments. Only the options that are actually present are
// set; any option left out stays unset and is therefore not verified. Returns false if an
// unrecognized option is encountered.
bool parse_arguments(int argc, char** argv, ExpectedValues& out)
{
    bool all_recognized = true;

    std::string value;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (match_option(arg, "uid", value))
        {
            out.uid = static_cast<uid_t>(std::stoul(value));
        }
        else if (match_option(arg, "gid", value))
        {
            out.gid = static_cast<gid_t>(std::stoul(value));
        }
        else if (match_option(arg, "supplementary-groups", value))
        {
            out.supplementary_groups = parse_groups(value);
        }
        else if (match_option(arg, "scheduling-policy", value))
        {
            out.policy = parse_policy(value);
        }
        else if (match_option(arg, "scheduling-priority", value))
        {
            out.priority = std::stoi(value);
        }
        else if (match_option(arg, "working-dir", value))
        {
            out.working_dir = value;
        }
        else
        {
            std::cerr << "Unrecognized argument: " << arg << std::endl;
            all_recognized = false;
        }
    }

    return all_recognized;
}

}  // namespace

TEST(SandboxOptions, RunAndVerify)
{
    EXPECT_TRUE(sandbox_options::verifySandbox(expected)) << "Sandbox options verification failed";
    score::mw::lifecycle::report_running();
}

int main(int argc, char** argv)
{
    if (!parse_arguments(argc, argv, expected))
    {
        std::cerr << "Recognized sandbox options: --uid, --gid, --supplementary-groups, "
                     "--scheduling-policy, --scheduling-priority, --working-dir"
                  << std::endl;
        return 1;
    }
    // Derive the GTest XML result file name from the executable name (argv[0]) rather than the
    // shared source file, so that the two binaries built from this source (sandbox_options_process_a
    // and sandbox_options_process_b) write distinct result files. Dereference rather than index to
    // avoid the no-pointer-arithmetic lint rule.
    const char* const executable_path = (argc > 0) ? *argv : __FILE__;
    return TestRunner(executable_path).RunTests();
}
