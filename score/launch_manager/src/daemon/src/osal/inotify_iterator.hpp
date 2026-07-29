#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "score/result/result.h"

/// @brief Redefinition of inotify_event
struct INotifyEvent
{
    /// @brief The watch descriptor.
    int wd;

    /// @brief Mask describing event.
    uint32_t mask;

    ///@brief Unique cookie associating related events.
    uint32_t cookie;

    ///@brief Optional name if watching for new files.
    std::string_view name;
};

class INotifyWatcher
{
  private:
    /// @brief Use the `Create()` method to create the `INotifyWatcher`.
    INotifyWatcher(int event_queue_fd, int event_fd, int epoll_fd);

    // forward declaration for iterator class
    class iterator;

  public:
    /// @brief Creates a INotifyWatcher or returns an error.
    [[nodiscard]] static score::Result<INotifyWatcher> Create() noexcept;

    ~INotifyWatcher();

    INotifyWatcher(INotifyWatcher&& other) noexcept;
    INotifyWatcher& operator=(INotifyWatcher&& other) noexcept;

    INotifyWatcher(const INotifyWatcher& other) = delete;
    INotifyWatcher& operator=(const INotifyWatcher& other) = delete;

    /// @brief Adds the given file to that watch with the given event mask.
    [[nodiscard]] int add_watch(const std::string_view path, uint32_t mask) const noexcept;

    /// @brief Interupt the reading of the events.
    void interrupt() const;

    /// @brief Gives an iterator to read new events from the registered
    ///        watches.
    [[nodiscard]] iterator begin();

    /// @brief Returns the sentinal iterator.
    /// @details If iterating over the iterator the `interrupt()` has to be
    ///          called.
    [[nodiscard]] iterator end();

  private:
    /// @brief The event queue taken from `inotify_init`
    int event_queue_fd_;

    /// @brief Event used to interrupt the wait.
    int event_fd_;

    /// @brief 
    int epoll_fd_;

    /// @brief An iterator that reads the event queue, and 
    ///
    /// @details 
    /// ```
    /// auto res = INotifyWatcher::Create();
    /// auto watcher = std::move(res).value();
    /// auto watch = watcher.add_watch("/tmp", IN_CREATE);
    /// 
    /// for (const INotifyEvent& event : watcher)
    /// {
    ///     std::cout << event.wd << std::endl;
    ///     std::cout << event.name << std::endl;
    /// }
    /// ```
    class iterator
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = INotifyEvent;
        using difference_type = std::ptrdiff_t;
        using pointer = const INotifyEvent*;
        using reference = const INotifyEvent&;

        explicit iterator(INotifyWatcher* watch_ptr = nullptr);

        [[nodiscard]] reference operator*() const;

        [[nodiscard]] pointer operator->() const;

        iterator& operator++();

        iterator operator++(int);

        [[nodiscard]] bool operator==(const iterator& other) const;

        [[nodiscard]] bool operator!=(const iterator& other) const;

      private:
        /// @brief Pointer to the parent watcher.
        INotifyWatcher* watcher_ = nullptr;

        /// @brief The current event.
        INotifyEvent current_{};

        /// @brief The size of the buffer for a single event entry.
        static constexpr size_t kEventSize = (sizeof(inotify_event) + NAME_MAX + 1);

        /// @brief Backing data for the event.
        std::array<char, kEventSize> buf_{};

        /// @brief Returns false if interrupted or error.
        [[nodiscard]] bool advance();

        /// @brief Parse the `inotify_event` at the current cursor into internal
        ///        data structure and advance cursor.
        void consume_next();
    };
};
