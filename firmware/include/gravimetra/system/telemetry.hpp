#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/power/power_manager.hpp"

#include <cstddef>
#include <cstdint>

namespace gravimetra::system {

enum class TelemetryField : std::uint64_t {
    monotonic_time = std::uint64_t{1U} << 0U,
    application_state = std::uint64_t{1U} << 1U,
    raw_ads1262_code = std::uint64_t{1U} << 2U,
    shunt_voltage = std::uint64_t{1U} << 3U,
    coil_current = std::uint64_t{1U} << 4U,
    raw_mass = std::uint64_t{1U} << 5U,
    corrected_mass = std::uint64_t{1U} << 6U,
    tared_mass = std::uint64_t{1U} << 7U,
    optical_difference = std::uint64_t{1U} << 8U,
    optical_sum = std::uint64_t{1U} << 9U,
    magnet_yoke_temperature = std::uint64_t{1U} << 10U,
    flexure_body_temperature = std::uint64_t{1U} << 11U,
    precision_afe_temperature = std::uint64_t{1U} << 12U,
    motion = std::uint64_t{1U} << 13U,
    tmc_driver_status = std::uint64_t{1U} << 14U,
    battery_voltage = std::uint64_t{1U} << 15U,
    power_state = std::uint64_t{1U} << 16U,
    estop_state = std::uint64_t{1U} << 17U,
    adc_profile = std::uint64_t{1U} << 18U,
    stability_diagnostics = std::uint64_t{1U} << 19U,
    fault_flags = std::uint64_t{1U} << 20U,
    calibration_version = std::uint64_t{1U} << 21U,
};

using TelemetryValidity = std::uint64_t;

[[nodiscard]] constexpr TelemetryValidity telemetry_field_mask(
    const TelemetryField field) noexcept {
    return static_cast<TelemetryValidity>(field);
}

struct TelemetrySample {
    TelemetryValidity valid_fields{0U};
    std::uint64_t monotonic_time_us{0U};
    std::uint16_t application_state{0U};
    std::int32_t raw_ads1262_code{0};
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
    std::int8_t active_auger{-1};
    std::uint8_t active_stage{0U};
    std::uint32_t commanded_motor_speed_steps_per_s{0U};
    std::uint64_t commanded_steps{0U};
    std::uint32_t tmc_driver_status{0U};
    double battery_voltage_v{0.0};
    power::PowerSource power_source{power::PowerSource::unknown};
    power::ChargerState charger_state{power::ChargerState::unavailable};
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
    bool stable{false};
    std::uint64_t fault_flags{0U};
    std::uint32_t calibration_version{0U};
};

class TelemetryCsv {
public:
    [[nodiscard]] static Status serialize_header(
        char* destination,
        std::size_t capacity,
        std::size_t& written) noexcept;
    [[nodiscard]] static Status serialize(
        const TelemetrySample& sample,
        char* destination,
        std::size_t capacity,
        std::size_t& written) noexcept;
};

}  // namespace gravimetra::system
