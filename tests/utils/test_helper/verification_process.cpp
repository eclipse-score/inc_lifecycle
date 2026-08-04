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
#include <iostream>
#include <string_view>

/// @file  verification_process.cpp
/// @brief Test process that touches signal files selected via command line
///        arguments. Pass "fallback" to touch the fallback file and/or
///        "test_end" to touch the test_end file.

int main(int argc, char** argv)
{
    bool touch_fallback = false;
    bool touch_test_end = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg{argv[i]};
        if (arg == "fallback")
        {
            touch_fallback = true;
        }
        else if (arg == "test_end")
        {
            std::cout << "Touching test_end file." << std::endl;
            touch_test_end = true;
        }
        else
        {
            std::cerr << "Unknown argument '" << arg << "', expected 'fallback' and/or 'test_end'" << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (touch_fallback && !touch_file(fallback_file))
    {
        std::cout << "Failed to write fallback file!" << std::endl;
        return EXIT_FAILURE;
    }

    if (touch_test_end && !touch_file(test_end_location))
    {
        std::cout << "Failed to write test_end file!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
