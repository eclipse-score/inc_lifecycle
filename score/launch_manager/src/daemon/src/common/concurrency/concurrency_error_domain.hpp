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

#ifndef CONCURRENCY_ERROR_DOMAIN_HPP_INCLUDED
#define CONCURRENCY_ERROR_DOMAIN_HPP_INCLUDED

#include "score/result/error_domain.h"
#include "score/result/result.h"

#include <ostream>

namespace score::mw::lifecycle::internal
{

enum class ConcurrencyErrc : score::result::ErrorCode
{
    /// @brief An OS call returned an error.
    kOsError = 1,

    // @brief The container has overflowed.
    kOverflow = 2,

    // @brief The container has stopped.
    kStopped = 3,

    // @brief A timeout was triggered.
    kTimeout = 4,
};

/// @brief Error domain for concurrency-related error codes.
class ConcurrencyErrorDomain final : public score::result::ErrorDomain
{
  public:
    std::string_view MessageFor(const score::result::ErrorCode& code) const noexcept override
    {
        switch (static_cast<ConcurrencyErrc>(code))
        {
            case ConcurrencyErrc::kOsError:
                return "OS call returned an error";
            case ConcurrencyErrc::kOverflow:
                return "Container has overflowed";
            case ConcurrencyErrc::kStopped:
                return "Container has stopped";
            case ConcurrencyErrc::kTimeout:
                return "Timeout was triggered";
            default:
                return "Unknown concurrency error";
        }
    }
};

/// @brief Global domain instance — required for ADL-based MakeError() lookup.
constexpr ConcurrencyErrorDomain kConcurrencyErrorDomain{};

/// @brief Creates a score::result::Error from a ConcurrencyErrc value (enables score::MakeUnexpected).
inline score::result::Error MakeError(ConcurrencyErrc code, std::string_view user_message = "") noexcept
{
    return {static_cast<score::result::ErrorCode>(code), kConcurrencyErrorDomain, user_message};
}

inline std::ostream& operator<<(std::ostream& os, ConcurrencyErrc errc) noexcept
{
    switch (errc)
    {
        case ConcurrencyErrc::kOsError:
            return os << "kOsError";
        case ConcurrencyErrc::kOverflow:
            return os << "kOverflow";
        case ConcurrencyErrc::kStopped:
            return os << "kStopped";
        case ConcurrencyErrc::kTimeout:
            return os << "kTimeout";
        default:
            return os << static_cast<score::result::ErrorCode>(errc);
    }
}

}  // namespace score::mw::lifecycle::internal

#ifdef LC_LOG_SCORE_MW_LOG
#include "score/mw/log/logger.h"

namespace score::mw::lifecycle::internal
{

inline score::mw::log::LogStream& operator<<(score::mw::log::LogStream& os, ConcurrencyErrc errc) noexcept
{
    switch (errc)
    {
        case ConcurrencyErrc::kOsError:
            return os << "kOsError";
        case ConcurrencyErrc::kOverflow:
            return os << "kOverflow";
        case ConcurrencyErrc::kStopped:
            return os << "kStopped";
        case ConcurrencyErrc::kTimeout:
            return os << "kTimeout";
        default:
            return os << static_cast<score::result::ErrorCode>(errc);
    }
}

}  // namespace score::mw::lifecycle::internal

#endif  // LC_LOG_SCORE_MW_LOG

#endif  // CONCURRENCY_ERROR_DOMAIN_HPP_INCLUDED
