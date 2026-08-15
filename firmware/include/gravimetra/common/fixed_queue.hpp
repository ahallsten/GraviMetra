#pragma once

#include <array>
#include <cstddef>

namespace gravimetra {

template <typename T, std::size_t Capacity>
class FixedQueue {
    static_assert(Capacity > 0U, "FixedQueue capacity must be nonzero");

public:
    [[nodiscard]] constexpr bool push(const T& value) noexcept {
        if (size_ == Capacity) {
            return false;
        }
        values_[tail_] = value;
        tail_ = (tail_ + 1U) % Capacity;
        ++size_;
        return true;
    }

    [[nodiscard]] constexpr bool pop(T& value) noexcept {
        if (size_ == 0U) {
            return false;
        }
        value = values_[head_];
        head_ = (head_ + 1U) % Capacity;
        --size_;
        return true;
    }

    constexpr void clear() noexcept {
        head_ = 0U;
        tail_ = 0U;
        size_ = 0U;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0U; }
    [[nodiscard]] constexpr bool full() const noexcept { return size_ == Capacity; }

private:
    std::array<T, Capacity> values_{};
    std::size_t head_{0U};
    std::size_t tail_{0U};
    std::size_t size_{0U};
};

}  // namespace gravimetra

