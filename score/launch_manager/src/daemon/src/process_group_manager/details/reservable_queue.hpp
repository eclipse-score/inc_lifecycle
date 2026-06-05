#ifndef SCORE_LCM_RESERVABLE_QUEUE_HPP
#define SCORE_LCM_RESERVABLE_QUEUE_HPP

#include <cstddef>
#include <vector>

namespace score::lcm
{

/// @brief Queue that can be preallocated. Not thread safe.
/// @details There's no .reserve() method on std::queue, so this class wraps a vector so it can be used like a queue
template <typename T>
class ReservableQueue
{
    static_assert(std::is_default_constructible_v<T>);

    std::vector<T> data;
    std::size_t front{};
    std::size_t back{};
    bool full{false};

  public:
    void reserve(std::size_t size)
    {
        data.resize(size);
    }

    void push(const T& val)
    {
        data[back] = val;
        back = (back + 1) % data.size();
        full = (back == front);
    }

    T pop()
    {
        auto val = data[front];
        front = (front + 1) % data.size();
        full = false;
        return val;
    }

    [[nodiscard]] bool empty() const
    {
        return front == back && !full;
    }
};

}  // namespace score::lcm

#endif  // SCORE_LCM_RESERVABLE_QUEUE_HPP
