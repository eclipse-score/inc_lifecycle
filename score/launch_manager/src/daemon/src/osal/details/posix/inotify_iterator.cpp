#include "score/mw/launch_manager/osal/inotify_iterator.hpp"
#include "score/launch_manager/src/daemon/src/common/log.hpp"
#include "score/launch_manager/src/execution_error.h"
#include <score/assert.hpp>
#include <sys/inotify.h>

#include <score/mw/lifecycle/execution_error.h>
#include <cstddef>

INotifyWatcher::INotifyWatcher(int event_queue_fd, int event_fd, int epoll_fd)
    : event_queue_fd_(event_queue_fd), event_fd_(event_fd), epoll_fd_(epoll_fd)
{

    auto add = [&](int fd) {
        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        ::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    };
    add(event_queue_fd_);
    add(event_fd_);
}

score::Result<INotifyWatcher> INotifyWatcher::Create() noexcept
{
    int event_queue_fd = ::inotify_init1(IN_CLOEXEC);
    int event_fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    int epoll_fd = ::epoll_create1(EPOLL_CLOEXEC);

    if (event_queue_fd < 0 || event_fd < 0 || epoll_fd < 0)
    {
        score::Result<INotifyWatcher>{score::MakeUnexpected(score::mw::lifecycle::ExecErrc::kCommunicationError)};
    }

    return INotifyWatcher{event_queue_fd, event_fd, epoll_fd};
}

INotifyWatcher::INotifyWatcher(INotifyWatcher&& other) noexcept
    : event_queue_fd_(other.event_queue_fd_), event_fd_(other.event_fd_), epoll_fd_(other.epoll_fd_)
{
    // invalidate others file descriptors
    other.event_queue_fd_ = -1;
    other.event_fd_ = -1;
    other.epoll_fd_ = -1;
}

INotifyWatcher& INotifyWatcher::operator=(INotifyWatcher&& other) noexcept
{
    event_queue_fd_ = other.event_queue_fd_;
    event_fd_ = other.event_fd_;
    epoll_fd_ = other.epoll_fd_;

    // invalidate others file descriptors
    other.event_queue_fd_ = -1;
    other.event_fd_ = -1;
    other.epoll_fd_ = -1;
    return *this;
}

INotifyWatcher::~INotifyWatcher()
{
    static_cast<void>(::close(epoll_fd_));
    static_cast<void>(::close(event_fd_));
    static_cast<void>(::close(event_queue_fd_));
}

int INotifyWatcher::add_watch(const std::string_view path, uint32_t mask) const noexcept
{
    return ::inotify_add_watch(event_queue_fd_, path.begin(), mask);
}

void INotifyWatcher::interrupt() const
{
    std::uint64_t val = 1;
    ::write(event_fd_, &val, sizeof(val));
}

INotifyWatcher::iterator INotifyWatcher::begin()
{
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(event_fd_ > 0, "Event FD not initlized!");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(epoll_fd_ > 0, "Event FD not initlized!");
    SCORE_LANGUAGE_FUTURECPP_ASSERT_MESSAGE(event_queue_fd_ > 0, "Event FD not initlized!");

    iterator out{this};
    ++out;
    return out;
}

INotifyWatcher::iterator INotifyWatcher::end()
{
    return iterator{nullptr};
}

INotifyWatcher::iterator::iterator(INotifyWatcher* watch_ptr) : watcher_(watch_ptr) {};

INotifyWatcher::iterator::reference INotifyWatcher::iterator::operator*() const
{
    return current_;
}

INotifyWatcher::iterator::pointer INotifyWatcher::iterator::operator->() const
{
    return &current_;
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
    // need 2 fields, one for the inotify fd and one for the event fd
    std::array<epoll_event, 2> events{};
    int events_seen = ::epoll_wait(watcher_->epoll_fd_, events.data(), events.size(), -1);

    if (events_seen < 0)
    {
        LM_LOG_ERROR() << "Failed to epoll_wait" << std::strerror(errno);
        return false;
    }

    for (const auto& event : events)
    {
        if (event.data.fd == watcher_->event_fd_)
        {
            // recived event from event_fd_ so we got interrupted
            std::uint64_t val = 0;
            ::read(watcher_->event_fd_, &val, sizeof(val));
            return false;
        }
    }

    ssize_t bytes_read = ::read(watcher_->event_queue_fd_, buf_.data(), buf_.size());
    if (bytes_read <= 0)
    {
        // this shouldn't happen but we should still continue...
        return false;
    }

    consume_next();
    return true;
};

void INotifyWatcher::iterator::consume_next()
{
    const char* raw = buf_.data();

    // using memcpy so that we don't reintepret_cast
    std::memcpy(&current_.wd, raw, sizeof(inotify_event::wd));
    raw = std::next(raw, sizeof(inotify_event::wd));

    std::memcpy(&current_.mask, raw, sizeof(inotify_event::mask));
    raw = std::next(raw, sizeof(inotify_event::mask));

    std::memcpy(&current_.cookie, raw, sizeof(inotify_event::cookie));
    raw = std::next(raw, sizeof(inotify_event::cookie));

    decltype(inotify_event::len) len{0};
    std::memcpy(&len, raw, sizeof(inotify_event::len));
    raw = std::next(raw, sizeof(inotify_event::len));

    current_.name = std::string_view(raw, len);
};
