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
// (c) 2025 ETAS GmbH. All rights reserved.

#ifndef PHMDAEMONCMDLINEPARSER_HPP_INCLUDED
#define PHMDAEMONCMDLINEPARSER_HPP_INCLUDED

#include <getopt.h>
#include <unistd.h>

#include <iostream>

#include "etas/vrte/saf/daemon/PhmDaemonConfig.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace daemon
{

/// @brief Parse and evaluate the arguments given to the PHM daemon
///
/// @tparam OStreamType Type of the output stream is configurable via this template
/// parameter to perform dependency injection for
/// - testing purposes primarily and
/// - to enable the exchange of the output stream used easily (for safety-relevant releases of the PHM)
///
/// @details The class is final, as no derived class is needed.
/// @todo Replace std::ostream as a default
/// @todo Provide an interface for getopt() function for:
/// - to be able to replace the getopt() with a safe option parsing function (as a replacement for getopt() function)
/// - to perform extended fault injection tests for the command line parser
/// @todo Provide a parser accepting a
template <typename OStreamType = std::ostream>
class PhmDaemonCmdLineParser final
{
public:
    /// @brief Ctor
    /// @param[in] f_ostream Reference to a stream object (like std::cout) to print out
    /// parse infos and errors
    explicit PhmDaemonCmdLineParser(OStreamType& f_ostream = std::cout) noexcept : ostream{f_ostream}
    {
    }

    /// @brief Parse the options given to the daemon process
    /// @param[in] f_argc Number of arguments (first argument is the executable name)
    /// @param[in] f_argv Array pointing to the arguments
    /// @return
    /// - 0 if parsing was successful and we can proceed with initialization
    /// - -1 if an error occurred or an option for direct termination
    /// was specified.
    ///
    /// @details Coverage justification for default case:
    /// the real getopt() function is used in unit tests as in the productive embedded code,
    /// which, on any unknown argument produces a '?' char, which is covered by tests.
    /// The default case is made as fallthrough for the unkown char case
    /// which will end the loop and returns the method. So even if getopt() would
    /// not produce a '?' char, the default fallthrough will terminate the loop.
    // coverity[exn_spec_violation:FALSE] std::length_error is not thrown from printVersion() as there is no resizing operation
    int8_t parseOptions(int f_argc, char* const* f_argv) const noexcept
    {
        const char* const short_opts = "vh";
        const option long_opts[] = {{"version", no_argument, nullptr, 'v'},
                                    {"help", no_argument, nullptr, 'h'},
                                    {nullptr, no_argument, nullptr, 0}};

        int8_t result{0};

        while (0 == result)
        {
            const int c{getopt_long(f_argc, f_argv, short_opts, long_opts, nullptr)};
            if (c == -1)
            {
                break;
            }
            else
            {
                // coverity[autosar_cpp14_a4_7_1_violation] getopt will return one of {char, -1}
                switch (static_cast<char>(c))
                {
                    case 'v':
                    {
                        std::cout << "0.0.0" << std::endl; 
                        result = 1;  // terminates the loop
                        break;
                    }
                    case 'h':
                    {
                        printUsage();
                        result = 1;  // terminates the loop
                        break;
                    }
                    default:
                    {
                        printUsage();
                        result = -1;  // terminates the loop
                        break;
                    }
                }
            }
        }

        // Resetting getopt, as it uses global variables in the process to operate properly.
        // All subsequent calls to getopt would fail and no parsing occurs, if it's not reset.
        optind = 1;

        return result;
    }

private:
    /// @brief Print out the help to the user
    void printUsage() const noexcept
    {
        ostream << "Syntax: rb-phmd -h/--help\n"
                << "        rb-phmd -v/--version\n"
                << "\n";

        ostream << "Options:\n"
                << " -h/--help Displays this help\n"
                << " -v/--version Displays version information\n"
                << "\n";
    }

    /// @brief Output stream for info/error messages for the user
    OStreamType& ostream;
};

}  // namespace daemon
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
