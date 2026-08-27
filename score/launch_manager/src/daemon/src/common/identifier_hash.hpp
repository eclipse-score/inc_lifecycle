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

#ifndef IDENTIFIER_HASH_H_
#define IDENTIFIER_HASH_H_

#include <cstddef>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace score::mw::lifecycle
{

/// @file identifier_hash.hpp
/// @brief This file contains the declaration of the IdentifierHash class,
///        which is used to store an identifier in implementation defined format.

/// @brief This class provides implementation defined representation of an ID.
/// This class represents an identifier.
/// Usually this is a path to a short name of an element,
/// for example path to a port (or Process Group) short name.
///
/// @details The class is designed to be used in contexts where IDs needs to be managed,
/// compared, and manipulated. It ensures that IDs are handled efficiently and provides
/// necessary operators and functions for common operations.
class IdentifierHash final
{
  public:
    /// @brief Constructs an IdentifierHash object from the given ID.
    /// @param id A const reference to std::string representing an ID.
    explicit IdentifierHash(const std::string& id);

    /// @brief Constructs an IdentifierHash object from the given ID.
    /// @param id A std::string_view representing an ID.
    explicit IdentifierHash(std::string_view id);

    /// @brief Constructs an IdentifierHash object with the given ID.
    /// @param A C-string representing an ID.
    explicit IdentifierHash(const char* id);

    // This class is trivially copyable / movable
    // For this reason we are applying the rule of zero

    /// @brief Overloaded equality operator for comparing two IdentifierHash objects.
    ///
    /// This operator compares the current IdentifierHash object with another IdentifierHash object
    /// to determine if they are equal.
    ///
    /// @param other The IdentifierHash object to compare with.
    /// @return true if the ID strings of both objects are equal, false otherwise.
    bool operator==(const IdentifierHash& other) const;  // Overloaded operator for comparison

    bool operator==(const std::string_view& other) const;

    /// @brief Overloaded not equal operator for comparing two IdentifierHash objects.
    ///
    /// This operator compares the current IdentifierHash object with another IdentifierHash object
    /// to determine if they are not equal.
    ///
    /// @param other The IdentifierHash object to compare with.
    /// @return true if the ID strings of both objects are not equal, false otherwise.
    bool operator!=(const IdentifierHash& other) const;  // Overloaded operator for comparison

    bool operator!=(const std::string_view& other) const;

    /// @brief Overloaded less than operator for comparing two IdentifierHash objects.
    ///
    /// This operator compares the current IdentifierHash object with another IdentifierHash object
    /// to determine if one is less than the other in an arbitrary ordering based upon the hash values.
    ///
    /// @param other The IdentifierHash object to compare with.
    /// @return true if the hash of this object is less than the hash of the other, false otherwise.
    bool operator<(const IdentifierHash& other) const;  // Overloaded operator for comparison

    ///@brief Default constructor for the IdentifierHash class.
    /// This constructor initializes the IdentifierHash object with a default ID value.
    /// The ID value is calculated by hashing an empty string using std::hash<std::string>.

    // this constructor is used in the code that is not part of the POC
    // not sure if we should have this constructor or not, but there is code like this:
    //
    //      In Configurationmanager.cpp
    //      ---------------------------
    //      ProcessGroup process_group_data;
    //      OsProcess instance;
    //      ProcessGroupStateID pg_info;
    //
    //      In Configurationmanager.hpp
    //      ---------------------------
    //      struct ProcessGroup {
    //      IdentifierHash                    name_;
    //      };
    //      struct ProcessGroupStateID {
    //      IdentifierHash                    pg_name_;
    //      IdentifierHash                    pg_state_;
    //      };
    //
    //     probably the above code should be removed or modified
    //     but for temporary purpose we are keeping this constructor
    IdentifierHash();

    /// @brief Returns the data associated with the IdentifierHash.
    /// This function returns the data stored in the IdentifierHash object.
    /// @return A constant reference to the data stored in the IdentifierHash object.
    std::size_t data() const;

    /// @brief Returns the registry mapping hash values to their corresponding string representations.
    ///
    /// Static registry, which gets initialized per process.
    ///
    /// @note The registry is mutated by every IdentifierHash constructor and may be read
    /// concurrently (e.g. via operator<<) from multiple threads. Callers accessing the
    /// registry directly (e.g. tests) must hold get_registry_mutex() for the duration of
    /// the access.
    ///
    /// @return A reference to the static unordered_map that serves as the registry.
    static std::unordered_map<std::size_t, std::string>& get_registry();

    /// @brief Returns the mutex protecting the static registry from concurrent access.
    /// @return A reference to the static mutex guarding get_registry().
    static std::mutex& get_registry_mutex();

  private:
    /// internal representation of the ID, that was passed in constructor
    std::size_t hash_id_ = 0;
};

/// @brief Overloaded stream insertion operator for IdentifierHash.
///
/// This operator allows IdentifierHash objects to be output to an ostream.
/// It uses the the static registry to find the string representation of the hash ID.
///
/// If there is no value stored in the registry for the given hash
/// (i.e. the IdentifierHash is not constructed in this process,
/// instead it has been transferred from another process via e.g. shared memory),
/// it outputs an error message.
///
/// @param os The output stream.
/// @param id The IdentifierHash object to output.
/// @return A reference to the output stream.
inline std::ostream& operator<<(std::ostream& stream, const IdentifierHash& id_hash) noexcept(false)
{
    const std::lock_guard<std::mutex> lock(IdentifierHash::get_registry_mutex());
    const auto& reg = IdentifierHash::get_registry();
    const auto it = reg.find(id_hash.data());
    if (it != reg.end())
    {
        stream << it->second;
    }
    else
    {
        stream << "<Unknown IdentifierHash: " << id_hash.data() << ">";
    }
    return stream;
}

}  // namespace score::mw::lifecycle

namespace std
{

/// @brief Specialization of std::hash for IdentifierHash, so it can be used directly as a key in
/// std::unordered_map/std::unordered_set. Delegates to IdentifierHash::data(), which is already
/// computed with the FNV-1a algorithm above — keeping a single, portable hash implementation
/// instead of letting callers fall back to the (implementation-defined) default std::hash.
template <>
struct hash<score::mw::lifecycle::IdentifierHash>
{
    std::size_t operator()(const score::mw::lifecycle::IdentifierHash& id_hash) const noexcept
    {
        return id_hash.data();
    }
};

}  // namespace std

#ifdef LC_LOG_SCORE_MW_LOG
#include "score/mw/log/logger.h"

namespace score::mw::lifecycle
{

inline score::mw::log::LogStream& operator<<(score::mw::log::LogStream& stream, const IdentifierHash& id_hash) noexcept(
    false)
{
    const std::lock_guard<std::mutex> lock(IdentifierHash::get_registry_mutex());
    const auto& reg = IdentifierHash::get_registry();
    const auto it = reg.find(id_hash.data());
    if (it != reg.end())
    {
        stream << it->second;
    }
    else
    {
        stream << "<Unknown IdentifierHash: " << id_hash.data() << ">";
    }
    return stream;
}

}  // namespace score::mw::lifecycle

#endif  // LC_LOG_SCORE_MW_LOG

#endif  // IDENTIFIER_HASH_H_
