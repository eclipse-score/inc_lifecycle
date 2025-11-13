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

#include <score/lcm/identifier_hash.hpp>
#include <functional>

namespace score {

namespace lcm {

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
//
// We need to discus when and if std::hash should be replaced in LCM.
// At the moment, usage of std::hash is deemed good enough for our purposes.
// One thing to note: hashing function is implementation specific.
// So if this should work between different compilers, we may need to go for our own hash function.

IdentifierHash::IdentifierHash(const std::string& id) {
    hash_id_ = std::hash<std::string>{}(id);
}

IdentifierHash::IdentifierHash(std::string_view id) {
    hash_id_ = std::hash<std::string_view>{}(id);
}

IdentifierHash::IdentifierHash(const char* id) {
    hash_id_ =
        std::hash<std::string_view>{}(id != nullptr ? std::string_view(id) : std::string_view(""));
}

bool IdentifierHash::operator==(const IdentifierHash& other) const {
    return hash_id_ == other.hash_id_;
}

bool IdentifierHash::operator!=(const IdentifierHash& other) const {
    return !operator==(other);
}

bool IdentifierHash::operator==(const std::string_view& other) const {
    return hash_id_ == (IdentifierHash{other}).hash_id_;
}

bool IdentifierHash::operator!=(const std::string_view& other) const {
    return !operator==(IdentifierHash{other});
}

bool IdentifierHash::operator<(const IdentifierHash& other) const {
    return hash_id_ < other.hash_id_;
}

IdentifierHash::IdentifierHash() {
    hash_id_ = std::hash<std::string_view>{}(std::string_view(""));
}

std::size_t IdentifierHash::data() const {
    return hash_id_;
}

}  // namespace lcm

}  // namespace score
