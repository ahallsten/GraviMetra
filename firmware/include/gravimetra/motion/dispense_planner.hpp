#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"
#include "gravimetra/motion/auger_manager.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::motion {

inline constexpr double kFrozenLiveCapacityMg = 250'000.0;

enum class DispenseState : std::uint8_t {
    idle = 0,
    verify_empty,
    dispense_stage_1,
    dispense_stage_2,
    dispense_stage_3,
    dispense_stage_4,
    settle,
    validate,
    complete,
    underfill,
    overfill,
    fault,
    estop,
};

enum class DispenseFault : std::uint8_t {
    none = 0,
    invalid_configuration,
    tare_timeout,
    motion_timeout,
    stability_timeout,
    motor_driver_fault,
    interlock_violation,
    measurement_invalid,
    estop_active,
    capacity_exceeded,
};

enum class MeasurementProfileRequest : std::uint8_t {
    precision_settle = 0,
    active_dispense,
};

struct DispenseSample {
    bool valid{false};
    bool stable{false};
    // Responsive mass is supplied by the active-dispense measurement path;
    // settled mass is supplied after the requested precision profile settles.
    double responsive_mass_mg{0.0};
    double settled_mass_mg{0.0};
    // The acquisition coordinator stamps samples with the exact profile
    // request generation it has applied. This prevents stale samples from a
    // prior profile transition from authorizing motion or final validation.
    MeasurementProfileRequest profile{MeasurementProfileRequest::precision_settle};
    std::uint64_t profile_generation{0U};
    bool profile_settled{false};
};

struct DispenseStageConfig {
    bool enabled{false};
    std::uint8_t motor_channel{0xFFU};
    MotorDirection direction{MotorDirection::forward};
    MotorElectricalConfig motor{};
    std::uint32_t start_speed_hz{0U};
    std::uint32_t maximum_speed_hz{0U};
    double acceleration_hz_per_second{0.0};
    std::uint32_t minimum_pulse_count{0U};
    double transition_error_mg{0.0};
    double predictive_margin_mg{0.0};
    std::uint64_t timeout_us{0U};
    double maximum_overshoot_mg{0.0};

    [[nodiscard]] bool valid() const noexcept;
};

struct DispensePlannerConfig {
    // Array order is dispense order. Disabled entries are skipped, and each
    // enabled entry can select any configured physical motor channel.
    std::array<DispenseStageConfig, AugerManager::kMaximumAugers> stages{};
    // Required application ceiling. It may be lower, but never higher, than
    // the frozen 250 g rated live capacity.
    double maximum_allowed_mass_mg{0.0};
    double empty_tolerance_mg{0.0};
    double final_underfill_tolerance_mg{0.0};
    double final_overfill_tolerance_mg{0.0};
    std::uint64_t empty_verification_timeout_us{0U};
    std::uint64_t settling_timeout_us{0U};
    std::uint64_t validation_timeout_us{0U};

    [[nodiscard]] bool valid() const noexcept;
};

class DispensePlanner {
public:
    static constexpr std::uint8_t kNoStage = 0xFFU;

    DispensePlanner(
        AugerManager& augers,
        const hal::MonotonicClock& clock) noexcept;

    [[nodiscard]] Status configure(
        const DispensePlannerConfig& config) noexcept;
    [[nodiscard]] Status start(double target_mass_mg) noexcept;
    [[nodiscard]] Status update(const DispenseSample& sample) noexcept;
    [[nodiscard]] Status abort() noexcept;

    [[nodiscard]] DispenseState state() const noexcept {
        return state_;
    }
    [[nodiscard]] DispenseFault fault() const noexcept {
        return fault_;
    }
    [[nodiscard]] MeasurementProfileRequest requested_profile() const noexcept {
        return requested_profile_;
    }
    [[nodiscard]] std::uint64_t requested_profile_generation() const noexcept {
        return requested_profile_generation_;
    }
    [[nodiscard]] std::uint8_t active_stage() const noexcept {
        return active_stage_;
    }
    [[nodiscard]] double target_mass_mg() const noexcept {
        return target_mass_mg_;
    }
    [[nodiscard]] double final_mass_mg() const noexcept {
        return final_mass_mg_;
    }
    [[nodiscard]] double final_error_mg() const noexcept {
        return final_error_mg_;
    }
    [[nodiscard]] bool running() const noexcept;

private:
    [[nodiscard]] Status update_verify_empty(
        const DispenseSample& sample) noexcept;
    [[nodiscard]] Status update_stage(const DispenseSample& sample) noexcept;
    [[nodiscard]] Status update_settle(const DispenseSample& sample) noexcept;
    [[nodiscard]] Status update_validate(const DispenseSample& sample) noexcept;
    [[nodiscard]] Status enter_stage(std::uint8_t stage) noexcept;
    [[nodiscard]] Status enter_settle() noexcept;
    [[nodiscard]] Status enter_validate() noexcept;
    void finish_underfill(double mass_mg) noexcept;
    void finish_overfill(double mass_mg) noexcept;
    void finish_capacity_exceeded(double mass_mg) noexcept;
    void fail(DispenseFault fault) noexcept;
    [[nodiscard]] bool accept_requested_profile(
        const DispenseSample& sample) noexcept;
    void request_profile(
        MeasurementProfileRequest profile,
        bool force_new_generation = false) noexcept;
    [[nodiscard]] std::uint8_t first_enabled_stage() const noexcept;
    [[nodiscard]] std::uint8_t next_enabled_stage(
        std::uint8_t after_stage) const noexcept;
    [[nodiscard]] static DispenseState state_for_stage(
        std::uint8_t stage) noexcept;
    [[nodiscard]] static std::uint64_t deadline_after(
        std::uint64_t now,
        std::uint64_t duration) noexcept;
    [[nodiscard]] bool deadline_expired() const noexcept;

    AugerManager& augers_;
    const hal::MonotonicClock& clock_;
    DispensePlannerConfig config_{};
    DispenseState state_{DispenseState::idle};
    DispenseFault fault_{DispenseFault::none};
    MeasurementProfileRequest requested_profile_{
        MeasurementProfileRequest::precision_settle};
    std::uint64_t requested_profile_generation_{0U};
    bool profile_acknowledged_{false};
    std::uint8_t active_stage_{kNoStage};
    std::uint8_t settled_stage_{kNoStage};
    std::uint64_t deadline_us_{0U};
    std::uint64_t last_speed_update_us_{0U};
    double current_speed_hz_{0.0};
    double target_mass_mg_{0.0};
    double final_mass_mg_{0.0};
    double final_error_mg_{0.0};
    bool configured_{false};
};

}  // namespace gravimetra::motion
