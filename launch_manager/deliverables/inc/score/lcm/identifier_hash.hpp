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

#ifndef IDENTIFIER_HASH_H_
#define IDENTIFIER_HASH_H_

#include <cstddef>

#include <string>
#include <string_view>

namespace score {

namespace lcm {

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
class IdentifierHash final {
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
    ///This constructor initializes the IdentifierHash object with a default ID value.
    ///The ID value is calculated by hashing an empty string using std::hash<std::string>.

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

   private:
    /// internal representation of the ID, that was passed in constructor
    std::size_t hash_id_ = 0;
};

}  // namespace lcm

}  // namespace score

#endif  // IDENTIFIER_HASH_H_
