#include <climits>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "score/result/result.h"
#include "score/os/utils/inotify/inotify_instance.h"
#include "score/os/utils/inotify/inotify_instance_impl.h"
#include "score/os/utils/inotify/inotify_event.h"
#include "score/os/utils/inotify/inotify_watch_descriptor.h"
#include "score/mw/launch_manager/osal/inotify_watcher_interface.hpp"

// Use the baselibs inotify event type
using INotifyEvent = score::os::InotifyEvent;

class INotifyWatcher : public INotifyWatcherInterface
{
  private:
    /// @brief Use the `Create()` method to create the `INotifyWatcher`.
    explicit INotifyWatcher(std::unique_ptr<score::os::InotifyInstance> instance) noexcept;

    // forward declaration for iterator class
    class iterator;

  public:
    /// @brief Creates a INotifyWatcher or returns an error. If nullptr is given this will create the Impl.
    [[nodiscard]] static score::Result<INotifyWatcher> Create(std::unique_ptr<score::os::InotifyInstance> inotify_instance) noexcept;

    /// @brief Creates a INotifyWatcher or returns an error.
    [[nodiscard]] static score::Result<INotifyWatcher> Create() noexcept;

    ~INotifyWatcher() = default;

    INotifyWatcher(INotifyWatcher&& other) noexcept = default;
    INotifyWatcher& operator=(INotifyWatcher&& other) noexcept = default;

    INotifyWatcher(const INotifyWatcher& other) = delete;
    INotifyWatcher& operator=(const INotifyWatcher& other) = delete;

    /// @brief Adds the given file to that watch with the given event mask.
    [[nodiscard]] int add_watch(std::string_view path, uint32_t mask) const noexcept override;

    /// @brief Interupt the reading of the events.
    void interrupt() const noexcept override;

    /// @brief Gives an iterator to read new events from the registered
    ///        watches.
    [[nodiscard]] iterator begin();

    /// @brief Returns the sentinal iterator.
    /// @details If iterating over the iterator the `interrupt()` has to be
    ///          called.
    [[nodiscard]] iterator end();

  private:
    /// @brief The underlying inotify instance from baselibs
    std::unique_ptr<score::os::InotifyInstance> instance_;

    /// @brief Watch descriptor stored for the last add_watch call
    mutable score::os::InotifyWatchDescriptor last_watch_descriptor_{-1};

    /// @brief An iterator that reads the event queue
    ///
    /// @details
    /// ```
    /// auto res = INotifyWatcher::Create();
    /// auto watcher = std::move(res).value();
    /// auto watch = watcher.add_watch("/tmp", IN_CREATE);
    ///
    /// for (const auto& event : watcher)
    /// {
    ///     std::cout << event.GetWatchDescriptor().GetUnderlying() << std::endl;
    ///     std::cout << event.GetName() << std::endl;
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

        /// @brief Buffer of events read from the instance.
        score::cpp::static_vector<INotifyEvent, score::os::InotifyInstance::max_events> events_{};

        /// @brief Current position in the events buffer.
        size_t event_index_{0};

        /// @brief Returns false if interrupted or error.
        [[nodiscard]] bool advance();
    };
};
