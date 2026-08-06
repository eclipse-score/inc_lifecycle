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

#include <cstdint>
#include <ostream>

namespace score::lcm::internal
{

enum class ConcurrencyErrc : std::uint8_t
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
            return os << static_cast<std::uint8_t>(errc);
    }
}

}  // namespace score::lcm::internal

#ifdef LC_LOG_SCORE_MW_LOG
#include "score/mw/log/logger.h"

namespace score::lcm::internal
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
            return os << static_cast<std::uint8_t>(errc);
    }
}

}  // namespace score::lcm::internal

#endif  // LC_LOG_SCORE_MW_LOG

#endif  // CONCURRENCY_ERROR_DOMAIN_HPP_INCLUDED
