#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"
#include "gravimetra/motion/motor_driver.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::motion {

struct AugerChannelConfig {
    MotorDriver* driver{nullptr};
    hal::DigitalOutput* direction_output{nullptr};
    std::uint8_t timer_channel{0xFFU};
    std::uint32_t direction_setup_us{0U};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return driver != nullptr && direction_output != nullptr &&
               timer_channel != 0xFFU && direction_setup_us > 0U;
    }
};

class AugerManager {
public:
    static constexpr std::size_t kMaximumAugers = 4U;
    static constexpr std::uint8_t kNoActiveAuger = 0xFFU;

    AugerManager(
        hal::StepPulseTimer& timer,
        const hal::MonotonicClock& clock,
        const std::array<AugerChannelConfig, kMaximumAugers>& channels) noexcept;

    [[nodiscard]] Status configure_motor(
        std::uint8_t channel,
        const MotorElectricalConfig& config) noexcept;

    // The first call after a direction change prepares DIR and enable, then
    // returns busy until direction_setup_us has elapsed. A later call starts
    // the bounded hardware-timer pulse train.
    [[nodiscard]] Status start_pulses(
        std::uint8_t channel,
        MotorDirection direction,
        std::uint32_t frequency_hz,
        std::uint32_t pulse_count) noexcept;

    [[nodiscard]] Status stop(std::uint8_t channel) noexcept;
    [[nodiscard]] Status stop_all() noexcept;
    [[nodiscard]] Status service() noexcept;

    [[nodiscard]] Status read_driver_status(
        std::uint8_t channel,
        MotorDriverStatus& status) noexcept;

    [[nodiscard]] Status set_motion_permitted(bool permitted) noexcept;
    [[nodiscard]] Status clear_shutdown_fault() noexcept;
    [[nodiscard]] bool motion_permitted() const noexcept {
        return motion_permitted_;
    }
    [[nodiscard]] bool shutdown_fault_latched() const noexcept {
        return shutdown_fault_latched_;
    }
    [[nodiscard]] Status last_shutdown_status() const noexcept {
        return last_shutdown_status_;
    }
    // One bit per physical auger slot. Bits accumulate while the shutdown
    // fault is latched so simultaneous or repeated output failures are not
    // collapsed into a single ambiguous status code.
    [[nodiscard]] std::uint8_t shutdown_failure_mask() const noexcept {
        return shutdown_failure_mask_;
    }
    [[nodiscard]] bool any_active() const noexcept;
    [[nodiscard]] std::uint8_t active_channel() const noexcept {
        return active_channel_;
    }
    [[nodiscard]] std::uint64_t commanded_steps(
        std::uint8_t channel) const noexcept;

private:
    struct RuntimeChannel {
        bool direction_known{false};
        MotorDirection direction{MotorDirection::forward};
        std::uint64_t direction_ready_at_us{0U};
        std::uint64_t commanded_steps{0U};
    };

    [[nodiscard]] bool valid_channel(std::uint8_t channel) const noexcept;
    [[nodiscard]] bool another_channel_energized(
        std::uint8_t requested_channel) const noexcept;
    [[nodiscard]] bool all_channels_deenergized() const noexcept;
    void latch_shutdown_failure(
        Status status,
        std::uint8_t channel_failure_mask) noexcept;
    static void saturating_add(
        std::uint64_t& value,
        std::uint32_t increment) noexcept;

    hal::StepPulseTimer& timer_;
    const hal::MonotonicClock& clock_;
    std::array<AugerChannelConfig, kMaximumAugers> channels_{};
    std::array<RuntimeChannel, kMaximumAugers> runtime_{};
    std::uint8_t active_channel_{kNoActiveAuger};
    std::uint8_t prepared_channel_{kNoActiveAuger};
    bool motion_permitted_{false};
    bool shutdown_fault_latched_{false};
    Status last_shutdown_status_{Status::ok};
    std::uint8_t shutdown_failure_mask_{0U};
};

}  // namespace gravimetra::motion
