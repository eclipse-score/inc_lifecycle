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

#include "tests/utils/test_helper/test_helper.hpp"
#include <gtest/gtest.h>
#include <score/mw/lifecycle/report_running.h>
#include <unistd.h>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <thread>

enum class Operation : std::uint8_t
{
    Create = 0,
    Delete
};
std::string_view g_file_path;
std::chrono::milliseconds g_modify_delay;
Operation g_operation;

TEST(ModifyFile, ModifyFile)
{
    std::this_thread::sleep_for(g_modify_delay);
    switch (g_operation)
    {
        case (Operation::Create):
            ASSERT_TRUE(touch_file(g_file_path));
            std::cout << "created file " << g_file_path << "By " << ::getpid() << std::endl;
            break;
        case (Operation::Delete):
            std::error_code error{};
            ASSERT_TRUE(std::filesystem::remove(g_file_path, error))
                << "Could not remove file " << g_file_path << ": "
                << (error ? error.message() : "the file did not exist");
            std::cout << "deleted file " << g_file_path << "By " << ::getpid() << std::endl;
            break;
    }
    std::cout << "Operation complete!" << std::endl;
}

int main(int argc, char** argv)
{
    std::cout << "Starting" << std::endl;
    if (argc != 5)
    {
        std::cerr << "USAGE:" << argv[0]
                  << "(action [delete|create]) (file path) (milliseconds to wait before doing the operation) "
                     "(shall_report [0|1])"
                  << std::endl;
        return EXIT_FAILURE;
    }

    const std::string_view action{argv[1]};
    if (action == "create")
    {
        g_operation = Operation::Create;
    }
    else if (action == "delete")
    {
        g_operation = Operation::Delete;
    }
    else
    {
        std::cerr << "Program called with wrong action" << std::endl;
        return EXIT_FAILURE;
    }

    g_file_path = std::string_view{argv[2]};
    g_modify_delay = std::chrono::milliseconds{std::stoi(argv[3])};

    std::string xml_result{argv[0]};

    const std::string_view shall_report{argv[4]};
    if (shall_report == "report")
    {
        std::cout << "REPORTING RUNNING from " << ::getpid() << std::endl;
        score::mw::lifecycle::report_running();
        std::cout << "REPORTED!" << ::getpid() << std::endl;
        xml_result.append("_reporting");
    }

    return TestRunner(xml_result).RunTests();
}
