#include "gravimetra/motion/auger_manager.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace gravimetra::motion {
namespace {

[[nodiscard]] constexpr std::uint64_t deadline_after(
    const std::uint64_t now,
    const std::uint32_t duration) noexcept {
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    if (duration > maximum - now) {
        return maximum;
    }
    return now + duration;
}

}  // namespace

AugerManager::AugerManager(
    hal::StepPulseTimer& timer,
    const hal::MonotonicClock& clock,
    const std::array<AugerChannelConfig, kMaximumAugers>& channels) noexcept
    : timer_(timer), clock_(clock), channels_(channels) {
    // Motion begins locked. A validated safety monitor must explicitly permit
    // it after observing a healthy, released E-stop contact.
    static_cast<void>(stop_all());
}

bool AugerManager::valid_channel(const std::uint8_t channel) const noexcept {
    return static_cast<std::size_t>(channel) < channels_.size() &&
           channels_[channel].valid();
}

Status AugerManager::configure_motor(
    const std::uint8_t channel,
    const MotorElectricalConfig& config) noexcept {
    const Status service_status = service();
    if (!is_ok(service_status)) {
        return service_status;
    }
    if (shutdown_fault_latched_) {
        return Status::fault_active;
    }
    if (!valid_channel(channel)) {
        return Status::not_configured;
    }
    if (!config.valid()) {
        return Status::not_configured;
    }
    for (const auto& configured_channel : channels_) {
        if ((configured_channel.driver != nullptr &&
             configured_channel.driver->enable_state() !=
                 MotorEnableState::disabled) ||
            (configured_channel.timer_channel != 0xFFU &&
             timer_.active(configured_channel.timer_channel))) {
            return Status::busy;
        }
    }
    return channels_[channel].driver->configure(config);
}

bool AugerManager::another_channel_energized(
    const std::uint8_t requested_channel) const noexcept {
    for (std::size_t index = 0U; index < channels_.size(); ++index) {
        if (index == static_cast<std::size_t>(requested_channel)) {
            continue;
        }
        if ((channels_[index].driver != nullptr &&
             channels_[index].driver->enable_state() !=
                 MotorEnableState::disabled) ||
            (channels_[index].timer_channel != 0xFFU &&
             timer_.active(channels_[index].timer_channel))) {
            return true;
        }
    }
    return false;
}

Status AugerManager::start_pulses(
    const std::uint8_t channel,
    const MotorDirection direction,
    const std::uint32_t frequency_hz,
    const std::uint32_t pulse_count) noexcept {
    if (!valid_motor_direction(direction) || frequency_hz == 0U ||
        pulse_count == 0U) {
        return Status::invalid_argument;
    }
    const Status service_status = service();
    if (!is_ok(service_status)) {
        return service_status;
    }
    if (shutdown_fault_latched_) {
        return Status::fault_active;
    }
    if (!motion_permitted_) {
        return Status::fault_active;
    }
    if (!valid_channel(channel)) {
        return Status::not_configured;
    }
    if (another_channel_energized(channel)) {
        // A contradictory request is treated as a system-level interlock
        // event: de-energize every channel, including the one already active.
        static_cast<void>(stop_all());
        return Status::interlock_violation;
    }

    const auto& configured_channel = channels_[channel];
    auto& runtime_channel = runtime_[channel];
    if (timer_.active(configured_channel.timer_channel)) {
        return Status::busy;
    }

    const bool direction_changed =
        !runtime_channel.direction_known ||
        runtime_channel.direction != direction;
    if (direction_changed) {
        const Status direction_status = configured_channel.direction_output->write(
            direction == MotorDirection::reverse);
        if (!is_ok(direction_status)) {
            static_cast<void>(stop_all());
            return direction_status;
        }
        runtime_channel.direction_known = true;
        runtime_channel.direction = direction;
        runtime_channel.direction_ready_at_us = deadline_after(
            clock_.now_us(), configured_channel.direction_setup_us);
    }

    if (prepared_channel_ != channel ||
        !configured_channel.driver->enabled()) {
        const Status enable_status =
            configured_channel.driver->set_enabled(true);
        if (!is_ok(enable_status)) {
            static_cast<void>(stop_all());
            return enable_status;
        }
        prepared_channel_ = channel;
    }

    if (clock_.now_us() < runtime_channel.direction_ready_at_us) {
        return Status::busy;
    }
    if (another_channel_energized(channel)) {
        static_cast<void>(stop_all());
        return Status::interlock_violation;
    }

    const Status timer_status = timer_.start(
        configured_channel.timer_channel, frequency_hz, pulse_count);
    if (!is_ok(timer_status)) {
        static_cast<void>(stop_all());
        return timer_status;
    }
    active_channel_ = channel;
    saturating_add(runtime_channel.commanded_steps, pulse_count);
    return Status::ok;
}

Status AugerManager::stop(const std::uint8_t channel) noexcept {
    if (!valid_channel(channel)) {
        return Status::not_configured;
    }
    const auto& configured_channel = channels_[channel];
    timer_.stop(configured_channel.timer_channel);
    const Status status = configured_channel.driver->set_enabled(false);
    if (!is_ok(status)) {
        latch_shutdown_failure(
            status, static_cast<std::uint8_t>(1UL << channel));
    }
    if (active_channel_ == channel) {
        active_channel_ = kNoActiveAuger;
    }
    if (prepared_channel_ == channel) {
        prepared_channel_ = kNoActiveAuger;
    }
    return status;
}

Status AugerManager::stop_all() noexcept {
    Status aggregate = Status::ok;
    std::uint8_t failure_mask = 0U;
    for (std::size_t index = 0U; index < channels_.size(); ++index) {
        const auto& channel = channels_[index];
        if (channel.timer_channel != 0xFFU) {
            timer_.stop(channel.timer_channel);
        }
        if (channel.driver != nullptr) {
            const Status status = channel.driver->set_enabled(false);
            if (!is_ok(status)) {
                failure_mask = static_cast<std::uint8_t>(
                    failure_mask | static_cast<std::uint8_t>(1UL << index));
                if (is_ok(aggregate)) {
                    aggregate = status;
                }
            }
        }
    }
    active_channel_ = kNoActiveAuger;
    prepared_channel_ = kNoActiveAuger;
    if (!is_ok(aggregate)) {
        latch_shutdown_failure(aggregate, failure_mask);
    }
    return aggregate;
}

Status AugerManager::service() noexcept {
    if (active_channel_ == kNoActiveAuger ||
        !valid_channel(active_channel_)) {
        return shutdown_fault_latched_ ? Status::fault_active : Status::ok;
    }
    const std::uint8_t completed_channel = active_channel_;
    const auto& channel = channels_[completed_channel];
    if (timer_.active(channel.timer_channel)) {
        return Status::ok;
    }
    const Status status = channel.driver->set_enabled(false);
    active_channel_ = kNoActiveAuger;
    prepared_channel_ = kNoActiveAuger;
    if (!is_ok(status)) {
        latch_shutdown_failure(
            status,
            static_cast<std::uint8_t>(1UL << completed_channel));
        return status;
    }
    return Status::ok;
}

Status AugerManager::read_driver_status(
    const std::uint8_t channel,
    MotorDriverStatus& status) noexcept {
    if (!valid_channel(channel)) {
        status = {};
        return Status::not_configured;
    }
    return channels_[channel].driver->read_status(status);
}

Status AugerManager::set_motion_permitted(const bool permitted) noexcept {
    if (!permitted) {
        motion_permitted_ = false;
        return stop_all();
    }
    if (shutdown_fault_latched_ || !all_channels_deenergized()) {
        motion_permitted_ = false;
        return Status::fault_active;
    }
    motion_permitted_ = true;
    return Status::ok;
}

Status AugerManager::clear_shutdown_fault() noexcept {
    motion_permitted_ = false;
    const Status status = stop_all();
    if (!is_ok(status) || !all_channels_deenergized()) {
        if (is_ok(status)) {
            std::uint8_t failure_mask = 0U;
            for (std::size_t index = 0U; index < channels_.size(); ++index) {
                const auto& channel = channels_[index];
                if ((channel.driver != nullptr &&
                     channel.driver->enable_state() !=
                         MotorEnableState::disabled) ||
                    (channel.timer_channel != 0xFFU &&
                     timer_.active(channel.timer_channel))) {
                    failure_mask = static_cast<std::uint8_t>(
                        failure_mask |
                        static_cast<std::uint8_t>(1UL << index));
                }
            }
            latch_shutdown_failure(
                Status::verification_failed, failure_mask);
        }
        return is_ok(status) ? Status::verification_failed : status;
    }
    shutdown_fault_latched_ = false;
    last_shutdown_status_ = Status::ok;
    shutdown_failure_mask_ = 0U;
    return Status::ok;
}

bool AugerManager::any_active() const noexcept {
    for (const auto& channel : channels_) {
        if (channel.timer_channel != 0xFFU &&
            timer_.active(channel.timer_channel)) {
            return true;
        }
    }
    return false;
}

bool AugerManager::all_channels_deenergized() const noexcept {
    for (const auto& channel : channels_) {
        if ((channel.driver != nullptr &&
             channel.driver->enable_state() != MotorEnableState::disabled) ||
            (channel.timer_channel != 0xFFU &&
             timer_.active(channel.timer_channel))) {
            return false;
        }
    }
    return true;
}

void AugerManager::latch_shutdown_failure(
    const Status status,
    const std::uint8_t channel_failure_mask) noexcept {
    if (!shutdown_fault_latched_) {
        last_shutdown_status_ = status;
    }
    shutdown_fault_latched_ = true;
    shutdown_failure_mask_ = static_cast<std::uint8_t>(
        shutdown_failure_mask_ | channel_failure_mask);
    motion_permitted_ = false;
}

std::uint64_t AugerManager::commanded_steps(
    const std::uint8_t channel) const noexcept {
    if (!valid_channel(channel)) {
        return 0U;
    }
    return runtime_[channel].commanded_steps;
}

void AugerManager::saturating_add(
    std::uint64_t& value,
    const std::uint32_t increment) noexcept {
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    if (increment > maximum - value) {
        value = maximum;
        return;
    }
    value += increment;
}

}  // namespace gravimetra::motion
