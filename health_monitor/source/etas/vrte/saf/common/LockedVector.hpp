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

#ifndef LOCKEDVECTOR_HPP_INCLUDED
#define LOCKEDVECTOR_HPP_INCLUDED

#include "etas/vrte/saf/common/FixedSizeVector.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace common
{

template <class Type>
class LockedVector final : private FixedSizeVector<Type>
{
public:
    // Required guarantee for move constructors to be noexcept(true)
    static_assert(std::is_nothrow_move_constructible<FixedSizeVector<Type>>::value,
                  "FixedSizeVector<Type> should be nothrow move constructible");

    explicit LockedVector(FixedSizeVector<Type>&& f_fixedSizeVector_r) noexcept(true) :
        FixedSizeVector<Type>(std::move(f_fixedSizeVector_r))
    {
    }
    explicit LockedVector(std::vector<Type>&& f_araCoreVector_r) noexcept(true) :
        FixedSizeVector<Type>(std::move(f_araCoreVector_r))
    {
    }
    /* RULECHECKER_comment(0, 70, check_member_function_missing_static, "The underlying FixedSizeVector object could not be made static", true_no_defect) */
    /// @brief Return a reference to the first element of the LockedVector
    /// @details If LockedVector is empty, return value is undefined
    Type& front(void) noexcept(true)
    {
        return FixedSizeVector<Type>::front();
    }

    /// @brief Return a const reference to the first element of the LockedVector
    /// @details Incase LockedVector is empty, return value is undefined
    const Type& front(void) const noexcept(true)
    {
        return FixedSizeVector<Type>::front();
    }

    /// @brief Return a reference to the last element of the LockedVector
    /// @details If LockedVector is empty, return value is undefined
    Type& back(void) noexcept(true)
    {
        return FixedSizeVector<Type>::back();
    }

    /// @brief Return a const reference to the last element of the LockedVector
    /// @details If LockedVector is empty, return value is undefined
    const Type& back(void) const noexcept(true)
    {
        return FixedSizeVector<Type>::back();
    }

    /// @brief  Return the size of the LockedVector
    std::size_t size(void) const noexcept(true)
    {
        return FixedSizeVector<Type>::size();
    }

    using BufferIterator = Type*;
    using ConstBufferIterator = const Type*;

    /// @brief Iterator to the begin of the LockedVector
    /// @return Returns an iterator that points to first element of LockedVector
    BufferIterator begin(void) noexcept(true)
    {
        return FixedSizeVector<Type>::begin();
    }

    /// @brief Iterator to the end of the LockedVector
    /// @return Returns an iterator that points to end of LockedVector
    BufferIterator end(void) noexcept(true)
    {
        return FixedSizeVector<Type>::end();
    }

    /// @brief Iterator to begin of the LockedVector with const access
    /// @return Returns an iterator with const access to first element of LockedVector
    ConstBufferIterator cbegin(void) const noexcept(true)
    {
        return FixedSizeVector<Type>::cbegin();
    }

    /// @brief Iterator to end of the LockedVector with const access
    /// @return Returns an iterator with const access to end of LockedVector
    ConstBufferIterator cend(void) const noexcept(true)
    {
        return FixedSizeVector<Type>::cend();
    }

    ///@brief No default constructor
    LockedVector() = delete;
    /// @brief No copy constructor
    LockedVector(const LockedVector<Type>&) = delete;
    /// @brief No copy assignment
    LockedVector operator=(const LockedVector<Type>&) = delete;
    /// @brief No move constructor
    LockedVector(LockedVector<Type>&& f_lockedVector) = delete;
    /// @brief No move assignment
    LockedVector operator=(LockedVector<Type>&& f_lockedVector) = delete;
    /// @brief Default destructor
    ~LockedVector() override = default;
};

}  // namespace common
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif