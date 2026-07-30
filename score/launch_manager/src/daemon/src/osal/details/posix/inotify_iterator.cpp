#include "score/mw/launch_manager/osal/inotify_iterator.hpp"
#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/launch_manager/src/execution_error.h"
#include "score/os/utils/inotify/inotify_instance_impl.h"
#include <score/assert.hpp>
#include <sys/inotify.h>

#include <score/mw/lifecycle/execution_error.h>
#include <cstddef>
#include <memory>

INotifyWatcher::INotifyWatcher(std::unique_ptr<score::os::InotifyInstance> instance) noexcept
    : instance_(std::move(instance))
{
}

score::Result<INotifyWatcher> INotifyWatcher::Create(std::unique_ptr<score::os::InotifyInstance> inotify_instance) noexcept
{
    return INotifyWatcher{std::move(inotify_instance)};
}

score::Result<INotifyWatcher> INotifyWatcher::Create() noexcept
{
    auto instance = std::make_unique<score::os::InotifyInstanceImpl>();
    auto is_valid = instance->IsValid();
    if (!is_valid.has_value())
    {
        return score::MakeUnexpected(score::mw::lifecycle::ExecErrc::kCommunicationError);
    }
    return INotifyWatcher{std::move(instance)};
}


int INotifyWatcher::add_watch(std::string_view path, uint32_t mask) const noexcept
{
    if (!instance_)
    {
        return -1;
    }

    // Convert path to zstring_view (assuming path is null-terminated)
    score::safecpp::zstring_view zpath(path.data(), path.size());

    // Convert mask to EventMask
    auto event_mask = static_cast<score::os::Inotify::EventMask>(mask);

    auto result = instance_->AddWatch(zpath, event_mask);
    if (!result.has_value())
    {
        LM_LOG_ERROR() << "Couldn't add watch" << std::strerror(errno);
        return -1;
    }

    last_watch_descriptor_ = result.value();
    return last_watch_descriptor_.GetUnderlying();
}

void INotifyWatcher::interrupt() const noexcept
{
    if (instance_)
    {
        instance_->Close();
    }
}

INotifyWatcher::iterator INotifyWatcher::begin()
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(instance_ != nullptr, "INotify instance not initialized!");

    iterator out{this};
    ++out;
    return out;
}

INotifyWatcher::iterator INotifyWatcher::end()
{
    return iterator{nullptr};
}

INotifyWatcher::iterator::iterator(INotifyWatcher* watch_ptr) : watcher_(watch_ptr) {}

INotifyWatcher::iterator::reference INotifyWatcher::iterator::operator*() const
{
    return events_[event_index_];
}

INotifyWatcher::iterator::pointer INotifyWatcher::iterator::operator->() const
{
    return &events_[event_index_];
}

INotifyWatcher::iterator& INotifyWatcher::iterator::operator++()
{
    if (watcher_ == nullptr)
    {
        // don't advance over end()
        return *this;
    }
    if (!advance())
    {
        // become end()
        watcher_ = nullptr;
    }
    return *this;
}

INotifyWatcher::iterator INotifyWatcher::iterator::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

bool INotifyWatcher::iterator::operator==(const iterator& other) const
{
    return watcher_ == other.watcher_;
}

bool INotifyWatcher::iterator::operator!=(const iterator& other) const
{
    return !(*this == other);
}

bool INotifyWatcher::iterator::advance()
{
    if (!watcher_ || !watcher_->instance_)
    {
        return false;
    }

    // Check if we have more events in the current buffer
    if (event_index_ + 1 < events_.size())
    {
        ++event_index_;
        return true;
    }

    // Need to read new events from the instance
    auto result = watcher_->instance_->Read();
    if (!result.has_value())
    {
        LM_LOG_ERROR() << "Failed to read inotify events";
        return false;
    }

    events_ = std::move(result.value());
    if (events_.empty())
    {
        // No events or interrupted
        return false;
    }

    event_index_ = 0;
    return true;
}
