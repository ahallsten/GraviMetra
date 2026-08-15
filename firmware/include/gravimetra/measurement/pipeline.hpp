#pragma once

#include "gravimetra/calibration/calibration.hpp"
#include "gravimetra/calibration/temperature_compensation.hpp"
#include "gravimetra/common/status.hpp"
#include "gravimetra/drivers/ads1262.hpp"
#include "gravimetra/measurement/tare.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::measurement {

inline constexpr std::size_t kAds1262ReferenceCount = 5U;

struct ReferenceCalibration {
    bool configured{false};
    double calibrated_voltage_v{0.0};
};

struct MeasurementConfiguration {
    bool configured{false};
    std::array<ReferenceCalibration, kAds1262ReferenceCount> references{};
    double calibrated_shunt_resistance_ohm{0.0};
    calibration::PolynomialCalibration mass_calibration{};
    calibration::TemperatureCompensationModel temperature_model{};
};

struct MeasurementSnapshot {
    std::int32_t raw_adc_code{0};
    bool adc_saturated{false};
    drivers::Ads1262::ConversionContext adc_context{};
    double calibrated_reference_voltage_v{0.0};
    double effective_pga_gain{0.0};
    double differential_voltage_v{0.0};
    double coil_current_a{0.0};
    double uncorrected_mass_mg{0.0};
    calibration::TemperatureCorrection temperature{};
    double temperature_corrected_mass_mg{0.0};
    bool tare_applied{false};
    double tare_offset_mg{0.0};
    double final_mass_mg{0.0};
    double final_mass_grains{0.0};
};

[[nodiscard]] Status validate_measurement_configuration(
    const MeasurementConfiguration& configuration) noexcept;

[[nodiscard]] Status convert_measurement(
    const MeasurementConfiguration& configuration,
    const drivers::Ads1262::Conversion& conversion,
    const calibration::TemperatureReadings& temperatures,
    const TareState& tare,
    MeasurementSnapshot& snapshot) noexcept;

[[nodiscard]] constexpr double adc_code_to_voltage(
    const std::int32_t raw_adc_code,
    const double reference_voltage_v,
    const drivers::Ads1262::Gain gain,
    const bool pga_bypassed) noexcept {
    constexpr double kSignedCodeDenominator = 2'147'483'648.0;
    const double effective_gain =
        pga_bypassed ? 1.0 : drivers::Ads1262::gain_value(gain);
    return (static_cast<double>(raw_adc_code) / kSignedCodeDenominator) *
           (reference_voltage_v / effective_gain);
}

}  // namespace gravimetra::measurement
