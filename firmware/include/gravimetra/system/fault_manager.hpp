#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/power/power_manager.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::system {

enum class FaultSnapshotField : std::uint64_t {
    monotonic_time = std::uint64_t{1U} << 0U,
    application_state = std::uint64_t{1U} << 1U,
    raw_adc_code = std::uint64_t{1U} << 2U,
    shunt_voltage = std::uint64_t{1U} << 3U,
    coil_current = std::uint64_t{1U} << 4U,
    mass_values = std::uint64_t{1U} << 5U,
    optical_diagnostics = std::uint64_t{1U} << 6U,
    magnet_yoke_temperature = std::uint64_t{1U} << 7U,
    flexure_body_temperature = std::uint64_t{1U} << 8U,
    precision_afe_temperature = std::uint64_t{1U} << 9U,
    battery_voltage = std::uint64_t{1U} << 10U,
    rail_5v_voltage = std::uint64_t{1U} << 11U,
    rail_12v_voltage = std::uint64_t{1U} << 12U,
    power_state = std::uint64_t{1U} << 13U,
    motion_diagnostics = std::uint64_t{1U} << 14U,
    tmc_driver_status = std::uint64_t{1U} << 15U,
    estop_state = std::uint64_t{1U} << 16U,
    adc_profile = std::uint64_t{1U} << 17U,
    stability_diagnostics = std::uint64_t{1U} << 18U,
};

using FaultSnapshotValidity = std::uint64_t;

[[nodiscard]] constexpr FaultSnapshotValidity fault_snapshot_field_mask(
    const FaultSnapshotField field) noexcept {
    return static_cast<FaultSnapshotValidity>(field);
}

enum class FaultCode : std::uint8_t {
    ads1262_timeout = 0,
    ads1262_config_mismatch,
    ads1262_overrange,
    optical_difference_invalid,
    optical_sum_invalid,
    coil_current_overrange,
    coil_current_saturation,
    opa593_current_limit,
    opa593_thermal_warning,
    tmp117_missing,
    tmp117_out_of_range,
    excessive_zero_drift,
    calibration_invalid,
    calibration_crc_failure,
    battery_undervoltage,
    charger_fault,
    rail_5v_fault,
    rail_12v_fault,
    tmc2209_communication_fault,
    tmc2209_overtemperature,
    tmc2209_short,
    tmc2209_open_load,
    motor_motion_timeout,
    motor_interlock_violation,
    stability_timeout,
    check_mass_actuator_timeout,
    check_mass_failure,
    estop_active,
    watchdog_reset_history,
    count,
};

static_assert(
    static_cast<std::uint8_t>(FaultCode::count) <= 64U,
    "fault mask is limited to 64 fault codes");

struct FaultSnapshot {
    FaultSnapshotValidity valid_fields{0U};
    std::uint64_t monotonic_time_us{0U};
    std::uint16_t application_state{0U};
    std::int32_t raw_adc_code{0};
    double shunt_voltage_v{0.0};
    double coil_current_a{0.0};
    double raw_mass_mg{0.0};
    double corrected_mass_mg{0.0};
    double tared_mass_mg{0.0};
    double optical_difference{0.0};
    double optical_sum{0.0};
    double magnet_yoke_temperature_c{0.0};
    double flexure_body_temperature_c{0.0};
    double precision_afe_temperature_c{0.0};
    double battery_voltage_v{0.0};
    double rail_5v_voltage_v{0.0};
    double rail_12v_voltage_v{0.0};
    power::PowerSource power_source{power::PowerSource::unknown};
    power::ChargerState charger_state{power::ChargerState::unavailable};
    std::int8_t active_auger{-1};
    std::uint8_t active_stage{0U};
    std::uint32_t commanded_motor_speed_steps_per_s{0U};
    std::uint64_t commanded_steps{0U};
    std::uint32_t tmc_driver_status{0U};
    bool estop_active{false};
    std::uint8_t adc_profile{0U};
    double stability_standard_deviation_mg{0.0};
    double stability_slope_mg_per_s{0.0};
    double stability_peak_to_peak_mg{0.0};
    std::uint32_t stability_reason_flags{0U};
    std::uint16_t stability_sample_count{0U};
    std::uint16_t stability_valid_sample_count{0U};
    std::uint64_t stability_window_duration_us{0U};
    std::uint64_t stability_continuously_passing_duration_us{0U};
};

struct FaultRecord {
    std::uint32_t sequence{0U};
    FaultCode code{FaultCode::ads1262_timeout};
    bool became_active{false};
    FaultSnapshot snapshot{};
};

constexpr std::size_t kFaultHistoryCapacity = 32U;

class FaultManager {
public:
    [[nodiscard]] Status raise(
        FaultCode code,
        const FaultSnapshot& snapshot) noexcept;
    [[nodiscard]] Status clear(
        FaultCode code,
        const FaultSnapshot& snapshot) noexcept;
    [[nodiscard]] Status clear_all_active(
        const FaultSnapshot& snapshot) noexcept;

    [[nodiscard]] bool active(FaultCode code) const noexcept;
    [[nodiscard]] bool any_active() const noexcept;
    [[nodiscard]] std::uint64_t active_mask() const noexcept;

    [[nodiscard]] std::size_t history_size() const noexcept;
    // Index zero is the oldest retained transition.
    [[nodiscard]] const FaultRecord* history_at(std::size_t index) const noexcept;
    void clear_history() noexcept;

private:
    [[nodiscard]] static bool valid_code(FaultCode code) noexcept;
    [[nodiscard]] static std::uint64_t code_mask(FaultCode code) noexcept;
    void append_record(
        FaultCode code,
        bool became_active,
        const FaultSnapshot& snapshot) noexcept;

    std::uint64_t active_mask_{0U};
    std::array<FaultRecord, kFaultHistoryCapacity> history_{};
    std::size_t history_start_{0U};
    std::size_t history_size_{0U};
    std::uint32_t next_sequence_{1U};
};

}  // namespace gravimetra::system
