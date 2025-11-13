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

#ifndef FIXEDSIZEVECTOR_HPP_INCLUDED
#define FIXEDSIZEVECTOR_HPP_INCLUDED

#include <vector>

namespace etas
{
namespace vrte
{
namespace saf
{
namespace common
{
template <class Type>
class FixedSizeVector
{
public:
    // Required guarantees for move constructors and assignments to be noexcept(true)
    static_assert(std::is_nothrow_move_assignable<std::vector<Type>>::value,
                  "std::vector<Type> should be nothrow move assignable");

    /// @brief Parameterised constructor with length of FixedSizeVector
    /// @param[in] f_length The length of the FixedSizeVector
    /// @throws std::bad_alloc in case no memory on heap can be allocated
    explicit FixedSizeVector(std::size_t f_length) noexcept(false)
    {
        fixedSizedVector.reserve(f_length);
    }

    /// @brief Move constructor for FixedSizeVector object
    /// @param f_inputBuffer_r Reference to FixedSizeVector object to be move constructed
    /// @details Invokes the move assignment operation for FixedSizeVector
    FixedSizeVector(FixedSizeVector<Type>&& f_inputBuffer_r) noexcept(true)
    {
        *this = std::move(f_inputBuffer_r);
    }

    /// @brief Move constructor for std::vector object
    /// @param f_inputVector_r Reference to std::vector object to be move constructed
    /// @details Invokes the move assignment operation for std::vector
    explicit FixedSizeVector(std::vector<Type>&& f_inputVector_r) noexcept(true)
    {
        *this = std::move(f_inputVector_r);
    }

    /// @brief Move Assignment of FixedSizeVector
    /// @param f_inputBuffer_r Reference to FixedSizeVector object to be move assigned
    /// @return Pointer to moved object
    FixedSizeVector& operator=(FixedSizeVector<Type>&& f_inputBuffer_r) noexcept(true)
    {
        if (this != &f_inputBuffer_r)
        {
            this->fixedSizedVector.clear();
            this->fixedSizedVector = std::move(f_inputBuffer_r.fixedSizedVector);
        }
        return *this;
    }

    /// @brief Move Assignment with std::vector
    /// @param f_inputVector_r Reference to std::vector to be move assigned
    /// @return Pointer to moved object
    FixedSizeVector& operator=(std::vector<Type>&& f_inputVector_r) noexcept(true)
    {
        this->fixedSizedVector.clear();
        this->fixedSizedVector = std::move(f_inputVector_r);
        return *this;
    }

    /// @brief Clear the contents of FixedSizeVector
    void clear() noexcept(true)
    {
        fixedSizedVector.clear();
    }

    /// @brief Construct an element in-place within the FixedSizeVector
    /// @warning If the input type (BufferElementType) has a constructor that can throw then this function should be
    /// considered noexcept(false)
    /// @tparam BufferElementType Type of element to be added to the FixedSizeVector
    /// @param f_inputElement_r Input data used to construct the element in the FixedSizeVector
    /// @return True if successful construction of element in FixedSizeVector
    template <typename... BufferElementType>
    bool emplace_back(BufferElementType&&... f_inputElement_r)
    {
        bool result{false};
        if (size() < capacity())
        {
            fixedSizedVector.emplace_back(std::forward<BufferElementType>(f_inputElement_r)...);
            result = true;
        }
        return result;
    }

    /// @brief Copy an element to the FixedSizeVector
    /// @warning If the input type (Type) has a constructor that can throw then this function should be considered
    /// noexcept(false)
    /// @param f_inputElement_r Reference to element that has to be copied to FixedSizeVector
    /// @return True if element is added successfully to the FixedSizeVector
    /* RULECHECKER_comment(0, 10, check_cheap_to_copy_in_parameter, "Copying a value of template type could become expensive", true_no_defect) */
    bool push_back(const Type& f_inputElement_r)
    {
        bool result = false;
        if (size() < capacity())
        {
            fixedSizedVector.push_back(f_inputElement_r);
            result = true;
        }
        return result;
    }

    /// @brief Move an element into the FixedSizeVector
    /// @warning If the input type (Type) has a constructor that can throw then this function should be considered
    /// noexcept(false)
    /// @param f_inputElement_r Reference to element that has to be moved into the FixedSizeVector
    /// @return True if element is added successfully to the FixedSizeVector
    bool push_back(Type&& f_inputElement_r)
    {
        bool result = false;
        if (size() < capacity())
        {
            fixedSizedVector.push_back(std::move(f_inputElement_r));
            result = true;
        }
        return result;
    }

    /// @brief Return a reference to the first element of the FixedSizeVector
    /// @details Incase FixedSizeVector is empty, return value is undefined
    Type& front(void) noexcept(true)
    {
        return fixedSizedVector.front();
    }

    /// @brief Return a const reference to the first element of the FixedSizeVector
    /// @details Incase FixedSizeVector is empty, return value is undefined
    const Type& front(void) const noexcept(true)
    {
        return fixedSizedVector.front();
    }

    /// @brief Return a reference to the last element of the FixedSizeVector
    /// @details Incase FixedSizeVector is empty, return value is undefined
    Type& back(void) noexcept(true)
    {
        return fixedSizedVector.back();
    }

    /// @brief Return a const reference to the last element of the FixedSizeVector
    /// @details Incase FixedSizeVector is empty, return value is undefined
    const Type& back(void) const noexcept(true)
    {
        return fixedSizedVector.back();
    }

    using BufferIterator = Type*;
    using ConstBufferIterator = const Type*;

    /// @brief Iterator to the begin of the FixedSizeVector
    /// @return Returns an iterator that points to first element of FixedSizeVector
    BufferIterator begin(void) noexcept(true)
    {
        return fixedSizedVector.data();
    }

    /// @brief Iterator to begin of the FixedSizeVector with const access
    /// @return Returns an iterator with const access to first element of FixedSizeVector
    ConstBufferIterator cbegin(void) const noexcept(true)
    {
        return fixedSizedVector.data();
    }

    /// @brief Iterator to the end of the FixedSizeVector
    /// @return Returns an iterator that points to end of FixedSizeVector
    BufferIterator end(void) noexcept(true)
    {
        return fixedSizedVector.data() + fixedSizedVector.size();
    }

    /// @brief Iterator to end of the FixedSizeVector with const access
    /// @return Returns an iterator with const access to end of FixedSizeVector
    ConstBufferIterator cend(void) const noexcept(true)
    {
        return fixedSizedVector.data() + fixedSizedVector.size();
    }

    /// @brief Obtain value at given index using square braces
    /// @param f_index Index of the element within the FixedSizeVector
    /// @return FixedSizeVector element at the given index
    Type& operator[](std::size_t f_index) noexcept(true)
    {
        return fixedSizedVector[f_index];
    }

    /// @brief Obtain value at given index as const using square braces
    /// @param f_index Index of the element within the FixedSizeVector
    /// @return FixedSizeVector element as const at the given index
    const Type& operator[](std::size_t f_index) const noexcept(true)
    {
        return fixedSizedVector[f_index];
    }

    /// @brief  Return the size of the FixedSizeVector
    std::size_t size(void) const noexcept(true)
    {
        return fixedSizedVector.size();
    }

    /// @brief  Return the capacity of the FixedSizeVector
    std::size_t capacity(void) const noexcept(true)
    {
        return fixedSizedVector.capacity();
    }

    /// @brief No Default Constructor
    FixedSizeVector() = delete;
    /// @brief No Copy Constructor
    FixedSizeVector(const FixedSizeVector<Type>&) = delete;
    /// @brief No Copy Assignment
    FixedSizeVector& operator=(const FixedSizeVector<Type>&) = delete;
    /// @brief Default destructor
    virtual ~FixedSizeVector() = default;

private:
    std::vector<Type> fixedSizedVector{};
};
}  // namespace common
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif