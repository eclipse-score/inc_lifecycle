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

#include "score/mw/launch_manager/common/identifier_hash.hpp"
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace score::mw::lifecycle
{

// Please note that a lot of the following info, would normally belong to identifier_hash.hpp file.
// However, decision was made to publish identifier_hash.hpp alongside other headers.
// This was done to simplify implementation of ProcessGroup and ProcessGroupState classes.
// Following info should usually be in the header file, but since identifier_hash.hpp is public,
// we will keep our internal documentation inside identifier_hash.cpp.
//
// IdentifierHash class represents an identity, also known as identifier.
// Usually this is a path to a short name of an element,
// for example a path to a port (or Process Group) short name.
//
// For performance reasons, the ID will be turned into hash value.
// This way it can be easily copied and compared.
// However there is a downside to using hashes as, hash collisions cannot be avoided.
// To counter this, we will need to reject all user configuration that triggers collisions.
// This will be responsibility of configuration manager.
//
//
// Which hashing algorithm we should use (MD5, SHA-256)?
//
// After a quick analysis, it was concluded that std::hash should be good enough
// for initial implementation.
//
// Material used:
// - https://cplusplus.com/reference/functional/hash/
// - https://en.cppreference.com/w/cpp/utility/hash
// - https://en.cppreference.com/w/cpp/named_req/Hash
//
// In short:
// - If k1 == k2 is true, h(k1) == h(k2) is also true.
// - The probability of h(a) == h(b) for a != b should approach 1.0 / std::numeric_limits<std::size_t>::max().
//     - This is small enough for initial implementation.

namespace
{

// Portable hash function FNV-1a https://www.rfc-editor.org/info/rfc9923/

#if SIZE_MAX == UINT32_MAX
constexpr std::size_t kFnvOffsetBasis = 2166136261U;
constexpr std::size_t kFnvPrime = 16777619U;
#else
constexpr std::size_t kFnvOffsetBasis = 14695981039346656037ULL;
constexpr std::size_t kFnvPrime = 1099511628211ULL;
#endif

std::size_t Fnv1aHash(std::string_view data) noexcept
{
    std::size_t hash = kFnvOffsetBasis;
    for (const unsigned char byte : data)
    {
        hash ^= static_cast<std::size_t>(byte);
        hash *= kFnvPrime;
    }
    return hash;
}

}  // namespace

IdentifierHash::IdentifierHash(const std::string& id)
{
    hash_id_ = Fnv1aHash(id);
    const std::lock_guard<std::mutex> lock(get_registry_mutex());
    get_registry()[hash_id_] = id;
}

IdentifierHash::IdentifierHash(std::string_view id)
{
    hash_id_ = Fnv1aHash(id);
    const std::lock_guard<std::mutex> lock(get_registry_mutex());
    get_registry()[hash_id_] = id;
}

IdentifierHash::IdentifierHash(const char* id)
{
    const std::string_view sv = (id != nullptr) ? std::string_view(id) : std::string_view("");
    hash_id_ = Fnv1aHash(sv);
    const std::lock_guard<std::mutex> lock(get_registry_mutex());
    get_registry()[hash_id_] = sv;
}

bool IdentifierHash::operator==(const IdentifierHash& other) const
{
    return hash_id_ == other.hash_id_;
}

bool IdentifierHash::operator!=(const IdentifierHash& other) const
{
    return !operator==(other);
}

bool IdentifierHash::operator==(const std::string_view& other) const
{
    return hash_id_ == (IdentifierHash{other}).hash_id_;
}

bool IdentifierHash::operator!=(const std::string_view& other) const
{
    return !operator==(IdentifierHash{other});
}

bool IdentifierHash::operator<(const IdentifierHash& other) const
{
    return hash_id_ < other.hash_id_;
}

IdentifierHash::IdentifierHash()
{
    hash_id_ = Fnv1aHash(std::string_view(""));
    const std::lock_guard<std::mutex> lock(get_registry_mutex());
    get_registry()[hash_id_] = "";
}

std::size_t IdentifierHash::data() const
{
    return hash_id_;
}

std::unordered_map<std::size_t, std::string>& IdentifierHash::get_registry()
{
    /// Static registry, which gets initialized per process.
    static std::unordered_map<std::size_t, std::string> registry;
    return registry;
}

std::mutex& IdentifierHash::get_registry_mutex()
{
    /// Static mutex protecting the registry from concurrent access, since IdentifierHash
    /// instances are constructed and their string representation is read from multiple threads.
    static std::mutex registry_mutex;
    return registry_mutex;
}

}  // namespace score::mw::lifecycle
