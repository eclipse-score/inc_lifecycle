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

#include <cstdint>

#include "etas/vrte/saf/logging/PhmLogger.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace logging
{

#ifdef LC_LOG_SCORE_MW_LOG

PhmLogger::PhmLogger(std::string_view f_context, std::string_view f_description) noexcept(true)
    : logger_r(score::mw::log::CreateLogger(f_context, f_description))
{
}

#else  // LC_LOG_SCORE_MW_LOG

PhmLogger::PhmLogger(std::string_view f_context, std::string_view f_description) noexcept(true)
    :  logger_r(f_context, f_description)
{
}

#endif // LC_LOG_SCORE_MW_LOG

/* RULECHECKER_comment(0, 10, check_static_object_dynamic_initialization, "PhmLogger object is initialized\
 with static qualifier for singleton access", true_low) */
/* RULECHECKER_comment(0, 30, check_switch_clause_break_return, "Singleton object is returned as part of \
the switch case evaluation", true_low) */
PhmLogger& PhmLogger::getLogger(const EContext f_context) noexcept(true)
{
    static PhmLogger factoryLogger{"Fcty", "Context for error/info logging from PHM Factory"};
    static PhmLogger supervisionLogger{"Sprv", "Context for error/info logging from PHM Supervision"};
    static PhmLogger recoveryLogger{"Rcvy", "Context for error/info logging from PHM Recovery"};
    static PhmLogger watchdogLogger{"Wdg", "Context for error/info logging from PHM Watchdog"};

    switch (f_context)
    {
        case EContext::factory:
        {
            return factoryLogger;
        }
        case EContext::supervision:
        {
            return supervisionLogger;
        }
        case EContext::recovery:
        {
            return recoveryLogger;
        }
        case EContext::watchdog:
        {
            return watchdogLogger;
        }
        default:
        {
            // Always return a working logger
            return supervisionLogger;
        }
    }
}

Stream PhmLogger::LogFatal() noexcept
{
    return logger_r.LogFatal();
}

Stream PhmLogger::LogError() noexcept
{
    return logger_r.LogError();
}

Stream PhmLogger::LogWarn() noexcept
{
    return logger_r.LogWarn();
}

Stream PhmLogger::LogInfo() noexcept
{
    return logger_r.LogInfo();
}

Stream PhmLogger::LogDebug() noexcept
{
    return logger_r.LogDebug();
}

Stream PhmLogger::LogVerbose() noexcept
{
    return logger_r.LogVerbose();
}

}  // namespace logging
}  // namespace saf
}  // namespace vrte
}  // namespace etas
