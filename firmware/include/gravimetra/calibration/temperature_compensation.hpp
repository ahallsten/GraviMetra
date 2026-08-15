#pragma once

#include "gravimetra/common/status.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::calibration {

enum class TemperatureLocation : std::uint8_t {
    magnet_yoke = 0U,
    flexure = 1U,
    analog_front_end = 2U,
};

constexpr std::size_t kTemperatureLocationCount = 3U;

struct TemperatureReadings {
    std::array<double, kTemperatureLocationCount> celsius{};
    std::array<bool, kTemperatureLocationCount> valid{};
};

struct TemperatureCompensationModel {
    bool configured{false};
    std::array<bool, kTemperatureLocationCount> sensor_enabled{};
    std::array<double, kTemperatureLocationCount> reference_celsius{};
    std::array<double, kTemperatureLocationCount> offset_linear_mg_per_c{};
    std::array<double, kTemperatureLocationCount> offset_quadratic_mg_per_c2{};
    std::array<double, kTemperatureLocationCount> span_linear_per_c{};
    std::array<double, kTemperatureLocationCount> span_quadratic_per_c2{};
};

struct TemperatureCorrection {
    double offset_correction_mg{0.0};
    double span_correction_fraction{0.0};
    double total_correction_mg{0.0};
    double corrected_mass_mg{0.0};
};

[[nodiscard]] Status validate_temperature_model(
    const TemperatureCompensationModel& model) noexcept;

[[nodiscard]] Status apply_temperature_compensation(
    const TemperatureCompensationModel& model,
    const TemperatureReadings& readings,
    double uncorrected_mass_mg,
    TemperatureCorrection& correction) noexcept;

}  // namespace gravimetra::calibration
