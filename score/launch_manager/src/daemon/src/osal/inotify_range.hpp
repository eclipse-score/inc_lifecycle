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

#ifndef SCORE_MW_LIFECYCLE_OSAL_INOTIFY_RANGE_HPP_
#define SCORE_MW_LIFECYCLE_OSAL_INOTIFY_RANGE_HPP_

#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

#include "score/assert.hpp"
#include "score/os/utils/inotify/inotify_event.h"
#include "score/os/utils/inotify/inotify_instance.h"
#include "score/static_vector.hpp"

namespace score::mw::lifecycle
{

/// @brief A wrapper around score::os::InotifyInstance  to get iterator syntax.
class INotifyRange
{
  public:
    explicit INotifyRange(std::unique_ptr<score::os::InotifyInstance> instance) noexcept
        : instance_(std::move(instance))
    {
    }

    ~INotifyRange() = default;

    INotifyRange(INotifyRange&& other) noexcept = default;
    INotifyRange& operator=(INotifyRange&& other) noexcept = default;

    INotifyRange(const INotifyRange& other) = delete;
    INotifyRange& operator=(const INotifyRange& other) = delete;

    /// @brief The iterator giving InotifyEvents.
    /// @warning The iterator's lifetime must not exceed the `INotifyRange` it
    ///          was obtained from.
    class iterator
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = score::os::InotifyEvent;
        using difference_type = std::ptrdiff_t;
        using pointer = const score::os::InotifyEvent*;
        using reference = const score::os::InotifyEvent&;

        explicit iterator(score::os::InotifyInstance* instance = nullptr) noexcept : instance_(instance)
        {
            if ((instance_ != nullptr) && !advance())
            {
                instance_ = nullptr;
            }
        }

        [[nodiscard]] reference operator*() const
        {
            return events_[event_index_];
        }

        [[nodiscard]] pointer operator->() const
        {
            return &events_[event_index_];
        }

        iterator& operator++()
        {
            // we are end iterator
            if (instance_ == nullptr)
            {
                return *this;
            }

            // become end iterator
            if (!advance())
            {
                instance_ = nullptr;
            }

            return *this;
        }

        iterator operator++(int)
        {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        [[nodiscard]] bool operator==(const iterator& other) const
        {
            return instance_ == other.instance_;
        }

        [[nodiscard]] bool operator!=(const iterator& other) const
        {
            return !(*this == other);
        }

      private:
        [[nodiscard]] bool advance()
        {

            SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(
                instance_ != nullptr, "The parent INotifyRange must have been deleted, documented as UB!");

            const bool events_in_internal_buffer = event_index_ + 1U < events_.size();
            if (events_in_internal_buffer)
            {
                ++event_index_;
                return true;
            }

            auto result = instance_->Read();
            if (!result.has_value())
            {
                return false;
            }

            events_ = std::move(result.value());

            if (events_.empty())
            {
                return false;
            }

            event_index_ = 0U;
            return true;
        }

        /// @brief The underlying `InotifyInstance` instance.
        score::os::InotifyInstance* instance_{nullptr};

        /// @brief Internal buffer of events.
        score::cpp::static_vector<score::os::InotifyEvent, score::os::InotifyInstance::max_events> events_{};

        /// @brief Index to the next event to give from the internal buffer.
        std::size_t event_index_{0U};
    };

    [[nodiscard]] iterator begin() noexcept
    {
        return iterator{instance_.get()};
    }

    [[nodiscard]] iterator end() noexcept
    {
        return iterator{nullptr};
    }

    /// @brief Abort the inotify instance by closing it.
    void stop() noexcept
    {
        if (instance_)
        {
            instance_->Close();
        }
    }

  private:
    std::unique_ptr<score::os::InotifyInstance> instance_;
};

}  // namespace score::mw::lifecycle

#endif  // SCORE_MW_LIFECYCLE_OSAL_INOTIFY_RANGE_HPP_
