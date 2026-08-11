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

#include <charconv>
#include <unistd.h>
#include <array>
#include <cstdlib>
#include <string_view>

#if (_GNU_SOURCE && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 32) || __QNXNTO__
#include <cstring>
#endif

#ifndef SIGNAL_SAFE_LOG_HPP_INCLUDED
#define SIGNAL_SAFE_LOG_HPP_INCLUDED

namespace
{

/// @brief Utility for building log messages in an async signal safe context.
template <std::size_t C>
class signal_safe_buffer
{
  public:
    /// @brief Append the given string to the buffer.
    template <typename T, std::enable_if_t<std::is_convertible_v<T, std::string_view>, bool> = true>
    void append(const T& value)
    {
        const auto string = std::string_view(value);

        std::size_t length = std::min(string.length(), buffer_.size() - end_);

        for (std::size_t index = 0; index < length; ++index)
        {
            buffer_[end_++] = string[index];
        }
    }

    /// @brief Append the given integer to the buffer.
    template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
    void append(const T& value)
    {
        const auto [pointer, error] = std::to_chars(&buffer_[end_], &buffer_.back(), value);

        if (error == std::errc{})
        {
            end_ = pointer - &buffer_.front();
        }
    }

    /// @brief Write out the buffer to standard error.
    /// @return Whether the write succeeded. True means the whole message was
    ///         written. False means nothing, or only part was written.
    [[nodiscard]] bool flush()
    {
        while (start_ < end_)
        {
            const auto written = write(STDERR_FILENO, &buffer_[start_], end_ - start_);

            if (written >= 0)
            {
                start_ += written;
            }
            else if (errno != EINTR)
            {
                return false;
            }
        }

        // Rewind the indices in case the buffer is used again.
        start_ = 0;
        end_ = 0;

        return true;
    }

  private:
    std::array<char, C> buffer_ = {};
    std::size_t start_ = 0;
    std::size_t end_ = 0;
};

}  // namespace

namespace score::mw::lifecycle::internal
{

template <typename... T>
[[nodiscard]] bool signal_safe_log(const T&... values)
{
    const int original_errno = errno;  // For reentrancy

    signal_safe_buffer<1024U> buffer = {};
    (buffer.append(values), ...);
    buffer.append("\n");
    const bool written = buffer.flush();

    errno = original_errno;  // For reentrancy

    return written;
}

template <typename... T>
[[nodiscard]] bool signal_safe_log_errno(int log_errno, const T&... values)
{
#if _GNU_SOURCE && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 32
    // strerrordesc_np is documented as async signal safe.
    return signal_safe_log(values..., " (", strerrordesc_np(log_errno), ")");
#elif __QNXNTO__
    // QNX strerror is documented as async signal safe.
    return signal_safe_log(values..., " (", strerror(log_errno), ")");
#else
    // POSIX strerror is not async signal safe. Log the raw number instead.
    return signal_safe_log(values..., " (errno ", log_errno, ")");
#endif
}

}  // namespace score::mw::lifecycle::internal

#endif
