#pragma once

#include "gravimetra/common/status.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace gravimetra::measurement {

class ExponentialFilter {
public:
    [[nodiscard]] Status configure(const double alpha) noexcept {
        if (!std::isfinite(alpha) || alpha <= 0.0 || alpha > 1.0) {
            return Status::invalid_argument;
        }
        alpha_ = alpha;
        configured_ = true;
        reset();
        return Status::ok;
    }

    void reset() noexcept {
        initialized_ = false;
        output_ = 0.0;
    }

    [[nodiscard]] Status update(const double input, double& output) noexcept {
        if (!configured_) {
            return Status::not_configured;
        }
        if (!std::isfinite(input)) {
            return Status::invalid_argument;
        }
        const double candidate =
            initialized_ ? output_ + (alpha_ * (input - output_)) : input;
        if (!std::isfinite(candidate)) {
            return Status::verification_failed;
        }
        output_ = candidate;
        initialized_ = true;
        output = output_;
        return Status::ok;
    }

private:
    bool configured_{false};
    bool initialized_{false};
    double alpha_{0.0};
    double output_{0.0};
};

template <std::size_t Capacity>
class MovingAverageFilter {
    static_assert(Capacity > 0U, "moving average capacity must be nonzero");

public:
    [[nodiscard]] Status configure(const std::size_t window_size) noexcept {
        if (window_size == 0U || window_size > Capacity) {
            return Status::invalid_argument;
        }
        window_size_ = window_size;
        configured_ = true;
        reset();
        return Status::ok;
    }

    void reset() noexcept {
        values_.fill(0.0);
        next_ = 0U;
        count_ = 0U;
        sum_ = 0.0;
    }

    [[nodiscard]] Status update(const double input, double& output) noexcept {
        if (!configured_) {
            return Status::not_configured;
        }
        if (!std::isfinite(input)) {
            return Status::invalid_argument;
        }
        const bool full = count_ == window_size_;
        const double removed = full ? sum_ - values_[next_] : sum_;
        if (!std::isfinite(removed)) {
            return Status::verification_failed;
        }
        const double candidate_sum = removed + input;
        const std::size_t candidate_count = full ? count_ : count_ + 1U;
        const double candidate_output =
            candidate_sum / static_cast<double>(candidate_count);
        if (!std::isfinite(candidate_sum) ||
            !std::isfinite(candidate_output)) {
            return Status::verification_failed;
        }
        values_[next_] = input;
        sum_ = candidate_sum;
        count_ = candidate_count;
        next_ = (next_ + 1U) % window_size_;
        output = candidate_output;
        return Status::ok;
    }

    [[nodiscard]] std::size_t sample_count() const noexcept { return count_; }

private:
    std::array<double, Capacity> values_{};
    std::size_t window_size_{0U};
    std::size_t next_{0U};
    std::size_t count_{0U};
    double sum_{0.0};
    bool configured_{false};
};

}  // namespace gravimetra::measurement
