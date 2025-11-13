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
#ifndef TIMESORTINGBUFFER_HPP_INCLUDED
#define TIMESORTINGBUFFER_HPP_INCLUDED

#include <cstdint>
#include <type_traits>

#include "etas/vrte/saf/common/FixedSizeVector.hpp"
#include "etas/vrte/saf/timers/Timers_OsClock.hpp"

namespace etas
{
namespace vrte
{
namespace saf
{
namespace common
{

/// Time Sorting Buffer template class
/// Sorts a given type (TimeSortedElementType) into a buffer with chronological order (oldest first),
/// with the help of a additional provided timestamp value.
/// Elements are retrieved with getNextElement() from the beginning of the buffer (i.e. oldest first).
/// The class uses dynamic memory allocation only during construction, after construction no further
/// memory is allocated.
/// Note: This is also the reason why it was not implemented with std::set or std::list
template <class TimeSortedElementType>
class TimeSortingBuffer
{
    /// @brief To allow for nothrow move constructor and move assignment operator,
    /// the underlying type also needs to be nothrow move constructible and nothrow move assignable
    static_assert(std::is_nothrow_move_constructible<TimeSortedElementType>::value,
                  "Type must be nothrow move constructible");
    static_assert(std::is_nothrow_move_assignable<TimeSortedElementType>::value,
                  "Type must be nothrow move assignable");

public:
    /// Constructor
    /// @param [in] f_bufferSize    Number of available buffer elements
    explicit TimeSortingBuffer(uint16_t f_bufferSize) noexcept(false) : timeSortedBuffer(f_bufferSize)
    {
    }

    /// @brief Deleted Copy constructor
    TimeSortingBuffer(const TimeSortingBuffer&) = delete;
    /// @brief Default move constructor
    TimeSortingBuffer(TimeSortingBuffer&&) noexcept(true) = default;
    /// @brief Deleted Copy assignment operator
    TimeSortingBuffer& operator=(const TimeSortingBuffer&) = delete;
    /// @brief Default move assignment
    TimeSortingBuffer& operator=(TimeSortingBuffer&&) noexcept(true) = default;

    /// Push entry to buffer
    /// Pushes an element of the given type (TimeSortingElement) to the buffer in chronological order (oldest first)
    /// @warning  If the input type (TimeSortedElementType) has a constructor that can throw then this function should
    /// be considered noexcept(false).  std::bad_alloc and std::length_error are not thrown here because it is fixed
    /// size vector. Extension or resizing are not done in fixed size vector.
    /// @param [in] f_element_r     Element which shall be copied into the buffer
    /// @param [in] f_timestamp     Timestamp used for sorting it into the buffer
    /// @return                     Success of push (true) sufficient space in buffer was available
    /* RULECHECKER_comment(0, 3, check_cheap_to_copy_in_parameter, "For template argument f_element_r, it is not \
    possible to classify cheap_to_copy or expensive_to_copy without referring original object.", true_no_defect) */
    bool push(const TimeSortedElementType& f_element_r, const etas::vrte::saf::timers::NanoSecondType f_timestamp)
    {
        bool isSuccess{false};
        SortChainElement newElement{nullptr, nullptr, f_element_r, f_timestamp};
        if (timeSortedBuffer.emplace_back(std::move(newElement)))
        {
            SortChainElement* newChainElement_p{&timeSortedBuffer.back()};
            sort(*newChainElement_p);

            isSuccess = true;
        }
        return isSuccess;
    }

    /// Get next element
    /// Returns the next element in chronological order each time the function is called
    /// First call:  oldest
    /// Second call: oldest + 1
    /// ...
    /// @return TimeSortingElement*     Pointer to time sorted element
    ///                                 Returns nullptr in case end is reached or no entry available
    TimeSortedElementType* getNextElement(void) noexcept(true)
    {
        TimeSortedElementType* element_p{nullptr};
        if (timeSortedBuffer.size() != 0U)
        {
            if (lastReportedElement_p == nullptr)
            {
                element_p = &sortChainStart_p->element;
                lastReportedElement_p = sortChainStart_p;
            }
            else
            {
                if (lastReportedElement_p->next_p != nullptr)
                {
                    element_p = &lastReportedElement_p->next_p->element;
                    lastReportedElement_p = lastReportedElement_p->next_p;
                }
            }
        }
        return element_p;
    }

    /// Reset the time sorting history buffer
    void clear(void) noexcept(true)
    {
        sortChainStart_p = nullptr;
        sortChainEnd_p = nullptr;
        lastReportedElement_p = nullptr;
        timeSortedBuffer.clear();
    }

private:
    /// Sort Chain Element
    /// Structure used for sorting the elements
    /* RULECHECKER_comment(0, 11, check_non_private_non_pod_field, "Struct is only used internally within this class", true_no_defect) */
    /* RULECHECKER_comment(0, 10, check_non_pod_struct, "Struct is only used internally within this class", true_no_defect) */
    struct SortChainElement
    {
        SortChainElement* previous_p{
            nullptr};                       // Pointer to previous element, null pointer means first element (oldest)
        SortChainElement* next_p{nullptr};  // Pointer to next element, null pointer means last element (latest)
        TimeSortedElementType element{};    // Element to be sorted
        etas::vrte::saf::timers::NanoSecondType timestamp{0U};  // Timestamp used for sorting the elements
    };

    /// Sort elements
    /// Sort elements into an chronological order (oldest first)
    /// @param [in] f_newChainElement_r     Reference to new added element which needs to be sorted into the chain
    void sort(SortChainElement& f_newChainElement_r) noexcept(true)
    {
        if (sortChainStart_p == nullptr)
        {
            // First element added to the chain
            // start and end of the list are the same here
            pushFront(f_newChainElement_r, nullptr);
            sortChainEnd_p = &f_newChainElement_r;
        }
        else
        {
            bool entryAdded{false};
            // Start searching for the insertion spot from the end (where the newest entries are)
            SortChainElement* currentChainElement_p{sortChainEnd_p};
            while (!entryAdded)
            {
                if (f_newChainElement_r.timestamp >= currentChainElement_p->timestamp)
                {
                    pushAfter(f_newChainElement_r, *currentChainElement_p);
                    entryAdded = true;
                }
                else if (currentChainElement_p->previous_p == nullptr)
                {
                    pushFront(f_newChainElement_r, currentChainElement_p);
                    entryAdded = true;
                }
                else
                {
                    currentChainElement_p = currentChainElement_p->previous_p;
                }
            }
        }
    }

    /// Push element to front of chain
    /// Push element to front of chain chronological oldest element.
    /// @param [in] f_newChainElement_r     Reference to new added element which needs to be sorted into the chain
    /// @param [in] f_firstChainElement_p   Pointer to first element of the sort chain (will be second element after the
    /// call)
    void pushFront(SortChainElement& f_newChainElement_r, SortChainElement* const f_firstChainElement_p) noexcept(true)
    {
        f_newChainElement_r.previous_p = nullptr;
        f_newChainElement_r.next_p = f_firstChainElement_p;

        if (f_firstChainElement_p != nullptr)
        {
            f_firstChainElement_p->previous_p = &f_newChainElement_r;
        }

        sortChainStart_p = &f_newChainElement_r;
    }

    /// Push element after a given element
    /// @param [in] f_newChainElement_r  Reference to new added element which needs to be sorted into the chain
    /// @param [in] f_ChainElement_r     Reference to element after which the new element will be inserted
    void pushAfter(SortChainElement& f_newChainElement_r, SortChainElement& f_ChainElement_r) noexcept(true)
    {
        f_newChainElement_r.next_p = f_ChainElement_r.next_p;
        if (f_ChainElement_r.next_p != nullptr)
        {
            f_ChainElement_r.next_p->previous_p = &f_newChainElement_r;
        }
        f_ChainElement_r.next_p = &f_newChainElement_r;
        f_newChainElement_r.previous_p = &f_ChainElement_r;
        if (&f_ChainElement_r == sortChainEnd_p)
        {
            sortChainEnd_p = &f_newChainElement_r;
        }
    }

    /// Vector of chain elements sorted based on timestamp
    FixedSizeVector<SortChainElement> timeSortedBuffer{};

    /// Current sort chain start element (oldest)
    SortChainElement* sortChainStart_p = nullptr;

    /// Current sort chain end element (newest)
    SortChainElement* sortChainEnd_p = nullptr;

    /// Last reported element, used by getNextElement()
    SortChainElement* lastReportedElement_p = nullptr;
};

}  // namespace common
}  // namespace saf
}  // namespace vrte
}  // namespace etas

#endif
