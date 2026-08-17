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

#ifndef SCORE_MW_LIFECYCLE_FIXED_STRING_HPP
#define SCORE_MW_LIFECYCLE_FIXED_STRING_HPP

#include <array>
#include <cstddef>
#include <cstring>
#include <string_view>

namespace score::mw::lifecycle
{

/// @brief Allocation-free, fixed-capacity string with a null-termination invariant.
///
/// Stores a null-terminated string in a plain `std::array` of size `MaxLength + 1`.
/// Designed for use across API boundaries where heap allocation is undesirable and
/// a bounded string length is acceptable.
///
/// @tparam MaxLength Maximum number of usable characters, excluding the null terminator.
///                   The internal storage is `MaxLength + 1` bytes so the terminator
///                   always fits.
///
/// @par Invariant
/// The stored string is always null-terminated. Every constructor and assignment path
/// enforces this. Characters beyond the null terminator are indeterminate and must
/// not be read directly - use `as_string_view()` or `data()` instead.
///
/// @par Copying and moving
/// Copy and move are compiler-generated (rule of zero). The `std::array` storage
/// carries no heap resource, so the generated forms are correct and efficient.
///
/// @par Ordering
/// No `operator<` is provided. `FixedString` values have no meaningful ordering.
///
/// @par Cross-capacity comparison
/// `FixedString<N>` and `FixedString<M>` are distinct types, but free-function
/// `operator==` / `operator!=` overloads allow comparing them by string content.
template <std::size_t MaxLength>
struct FixedString
{
    /// @brief Construct an empty string (equivalent to "").
    ///
    /// Only `storage_[0]` is set to `'\0'`; the rest of the array is indeterminate.
    /// This is intentional - zeroing `MaxLength + 1` bytes on every default
    /// construction would be wasteful when the value is immediately overwritten.
    constexpr FixedString() noexcept
    {
        storage_[0] = '\0';
    }

    /// @brief Construct from a string literal or compile-time char array.
    ///
    /// Accepts only string literals and `const char[N]` arrays - not runtime
    /// `const char*` pointers. This prevents null pointer and dangling pointer
    /// bugs that a `const char*` constructor would allow implicitly.
    ///
    /// If the literal is longer than `MaxLength` characters (excluding the null
    /// terminator), the build fails with a `static_assert` - no silent truncation.
    ///
    /// @tparam N Size of the char array, including the null terminator.
    ///           For a string literal `"foo"`, N == 4.
    /// @param s  Source char array. Must be null-terminated (guaranteed for literals).
    template <std::size_t N>
    constexpr FixedString(const char (&s)[N]) noexcept
    {
        static_assert(
            N - 1U <= MaxLength,
            "String literal exceeds FixedString capacity - use a shorter literal or increase MaxLength");
        for (std::size_t i = 0U; i < N - 1U; ++i)
        {
            storage_[i] = s[i];
        }
        storage_[N - 1U] = '\0';
    }

    /// @brief Construct from a string_view, truncating if necessary.
    ///
    /// If `s.size() > MaxLength`, only the first `MaxLength` characters are stored.
    /// No error is raised at this level - detection and logging of over-length
    /// strings must happen at the user side where the source and context are
    /// available.
    ///
    /// @param s Source string. Does not require to be null-terminated.
    constexpr explicit FixedString(std::string_view s) noexcept
    {
        const auto len = std::min(s.size(), MaxLength);
        for (std::size_t i = 0; i < len; ++i)
        {
            storage_[i] = s[i];
        }
        storage_[len] = '\0';
    }

    /// @brief Assign from a string_view, truncating if necessary.
    ///
    /// Behaves identically to the string_view constructor: copies up to
    /// `MaxLength` characters and always null-terminates. If `s.size() >
    /// MaxLength`, only the first `MaxLength` characters are stored - no
    /// error is raised at this level.
    ///
    /// Allows direct assignment from string literals and std::string_view
    /// without constructing a temporary FixedString:
    /// @code
    ///   FixedString name;
    ///   name = "Running";
    /// @endcode
    ///
    /// @param s Source string. Does not require to be null-terminated.
    ///
    /// @returns Reference to this instance.
    constexpr FixedString& operator=(std::string_view s) noexcept
    {
        const auto len = std::min(s.size(), MaxLength);
        for (std::size_t i = 0; i < len; ++i)
        {
            storage_[i] = s[i];
        }
        storage_[len] = '\0';
        return *this;
    }

    /// @brief Returns the fixed character capacity of the FixedString.
    ///
    /// The returned value is equal to the template parameter `MaxLength` and
    /// represents the maximum number of characters that can be stored, excluding
    /// the terminating null character.
    ///
    /// @return Maximum number of usable characters.
    static constexpr std::size_t max_size() noexcept
    {
        return MaxLength;
    }

    /// @brief Returns the current length of the FixedString.
    ///
    /// The returned value represents the number of characters currently stored,
    /// excluding the terminating null character. Returns 0 for a
    /// default-constructed or cleared instance.
    ///
    /// Equivalent to `as_string_view().size()`.
    ///
    /// @return Number of stored characters.
    std::size_t size() const noexcept
    {
        return as_string_view().size();
    }

    /// @brief Checks whether the FixedString is empty.
    ///
    /// A FixedString is considered empty when it contains no characters.
    /// Returns true for a default-constructed or cleared instance.
    ///
    /// Equivalent to `size() == 0`.
    ///
    /// @return true if the FixedString is empty; otherwise, false.
    bool empty() const noexcept
    {
        return storage_[0] == '\0';
    }

    /// @brief Clears the contents of the FixedString.
    ///
    /// After this call, the FixedString is empty, `size()` returns 0,
    /// and `as_string_view()` returns an empty string.
    ///
    /// Only the terminating null character at the beginning of the internal
    /// storage is written; the remaining storage is left unchanged.
    void clear() noexcept
    {
        storage_[0] = '\0';
    }

    /// @brief Returns the contents of the FixedString as a std::string_view.
    ///
    /// The returned view refers directly to the internal storage of this
    /// FixedString and does not own the underlying character data.
    ///
    /// The view remains valid only while this FixedString instance exists and
    /// its contents are not modified.
    ///
    /// @return A std::string_view representing the stored character sequence.
    std::string_view as_string_view() const noexcept
    {
        return std::string_view{storage_.data()};
    }

    /// @brief Returns a pointer to the stored null-terminated character sequence.
    ///
    /// The returned pointer refers directly to the internal storage of this
    /// FixedString and is suitable for use with APIs expecting a C-style string.
    ///
    /// The pointer remains valid only while this FixedString instance exists and
    /// its contents are not modified.
    ///
    /// @return Pointer to the stored null-terminated character sequence.
    const char* data() const noexcept
    {
        return storage_.data();
    }

    /// @brief Equality comparison with another FixedString of the same capacity.
    ///
    /// Equality is determined by comparing the string representations of
    /// both instances.
    ///
    /// @param other The FixedString instance to compare against.
    ///
    /// @return true if both FixedString objects represent the same string;
    ///         otherwise, false.
    bool operator==(const FixedString& other) const noexcept
    {
        // strcmp does a single optimised pass - avoids the two strlen calls
        // that constructing string_views would require.
        return std::strcmp(storage_.data(), other.storage_.data()) == 0;
    }

    /// @brief Equality comparison with a string_view.
    ///
    /// @param other The string value to compare against.
    ///
    /// @return true if this FixedString represents the same string as
    ///         @p other; otherwise, false.
    bool operator==(std::string_view other) const noexcept
    {
        // other.size() is already known - use it as a length guard and for
        // strncmp, avoiding strlen on our storage.
        if (other.size() > MaxLength)
        {
            return false;
        }
        return std::strncmp(storage_.data(), other.data(), other.size()) == 0 && storage_[other.size()] == '\0';
    }

    /// @brief Equality comparison with a string literal or compile-time char array.
    ///
    /// This overload is an exact match for string literals, resolving the
    /// ambiguity that would otherwise arise between operator==(const FixedString&)
    /// and operator==(std::string_view) when both require one implicit conversion
    /// from a char array. An exact-match template is always preferred.
    ///
    /// @tparam N Size of the char array including the null terminator.
    /// @return true if this FixedString represents the same string; otherwise false.
    template <std::size_t N>
    bool operator==(const char (&other)[N]) const noexcept
    {
        if (N - 1U > MaxLength)
        {
            return false;
        }
        return std::strncmp(storage_.data(), other, N - 1U) == 0 && storage_[N - 1U] == '\0';
    }

    /// @brief Inequality comparison with another FixedString of the same capacity.
    ///
    /// Inequality is determined by comparing the string representations of
    /// both instances.
    ///
    /// @param other The FixedString instance to compare against.
    ///
    /// @return true if the FixedString objects represent different strings;
    ///         otherwise, false.
    bool operator!=(const FixedString& other) const noexcept
    {
        return !(*this == other);
    }

    /// @brief Inequality comparison with a string_view.
    ///
    /// Allows comparisons such as `fs != "Running"` without constructing
    /// a temporary FixedString.
    ///
    /// @param other The string value to compare against.
    ///
    /// @return true if this FixedString represents a different string
    ///         than @p other; otherwise, false.
    bool operator!=(std::string_view other) const noexcept
    {
        return !(*this == other);
    }

    /// @brief Inequality comparison with a string literal or compile-time char array.
    ///
    /// Exact-match overload that mirrors operator==(const char(&)[N]) to avoid
    /// ambiguity when comparing against string literals.
    ///
    /// @tparam N Size of the char array including the null terminator.
    /// @return true if this FixedString represents a different string; otherwise false.
    template <std::size_t N>
    bool operator!=(const char (&other)[N]) const noexcept
    {
        return !(*this == other);
    }

  private:
    std::array<char, MaxLength + 1U> storage_;
};

/// @brief Equality comparison between FixedString instances of different capacities.
///
/// Equality is determined by comparing the string representations of both
/// instances, regardless of their storage capacities.
///
/// For example, a `FixedString<128>` and a `FixedString<64>` containing the
/// same character sequence compare equal.
///
/// @tparam N Capacity of the left-hand FixedString.
/// @tparam M Capacity of the right-hand FixedString.
/// @param lhs The left-hand FixedString instance.
/// @param rhs The right-hand FixedString instance.
/// @return true if both FixedString objects represent the same string;
///         otherwise, false.
template <std::size_t N, std::size_t M>
bool operator==(const FixedString<N>& lhs, const FixedString<M>& rhs) noexcept
{
    return std::strcmp(lhs.data(), rhs.data()) == 0;
}

/// @brief Inequality comparison between FixedString instances of different capacities.
///
/// Inequality is determined by comparing the string representations of both
/// instances, regardless of their storage capacities.
///
/// For example, a `FixedString<128>` and a `FixedString<64>` containing
/// different character sequences compare not equal.
///
/// @tparam N Capacity of the left-hand FixedString.
/// @tparam M Capacity of the right-hand FixedString.
/// @param lhs The left-hand FixedString instance.
/// @param rhs The right-hand FixedString instance.
/// @return true if the FixedString objects represent different strings;
///         otherwise, false.
template <std::size_t N, std::size_t M>
bool operator!=(const FixedString<N>& lhs, const FixedString<M>& rhs) noexcept
{
    return !(lhs == rhs);
}

/// @brief Stream insertion operator for FixedString.
///
/// Writes the string representation of the FixedString to the specified
/// output stream.
///
/// The operator is templated on the stream type so it can be used with any
/// ostream-compatible stream (for example, `std::ostream` or mw::log streams)
/// without introducing additional dependencies in this header.
///
/// @tparam Stream Type of the output stream.
/// @tparam MaxLength Maximum capacity of the FixedString.
/// @param os The output stream.
/// @param fs The FixedString instance to write.
/// @return A reference to the output stream.
template <typename Stream, std::size_t MaxLength>
Stream& operator<<(Stream& os, const FixedString<MaxLength>& fs)
{
    return os << fs.as_string_view();
}

}  // namespace score::mw::lifecycle

#endif  // SCORE_MW_LIFECYCLE_FIXED_STRING_HPP
