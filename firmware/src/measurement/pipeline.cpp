#include "gravimetra/measurement/pipeline.hpp"

#include "gravimetra/measurement/units.hpp"

#include <cmath>
#include <cstddef>
#include <limits>

namespace gravimetra::measurement {
namespace {

[[nodiscard]] constexpr std::size_t reference_index(
    const drivers::Ads1262::Reference reference) noexcept {
    return static_cast<std::uint8_t>(reference);
}

[[nodiscard]] bool conversion_context_valid(
    const drivers::Ads1262::ConversionContext& context) noexcept {
    return context.verified && context.profile != drivers::Ads1262::ProfileId::none &&
           static_cast<std::uint8_t>(context.profile) <=
               static_cast<std::uint8_t>(
                   drivers::Ads1262::ProfileId::precision_settle) &&
           static_cast<std::uint8_t>(context.gain) <=
               static_cast<std::uint8_t>(drivers::Ads1262::Gain::x32) &&
           static_cast<std::uint8_t>(context.reference) <
               kAds1262ReferenceCount &&
           (!context.pga_bypassed || context.gain == drivers::Ads1262::Gain::x1);
}

}  // namespace

Status validate_measurement_configuration(
    const MeasurementConfiguration& configuration) noexcept {
    if (!configuration.configured ||
        !configuration.mass_calibration.configured ||
        !configuration.mass_calibration.accepted ||
        !configuration.temperature_model.configured) {
        return Status::not_configured;
    }
    if (!std::isfinite(configuration.calibrated_shunt_resistance_ohm) ||
        configuration.calibrated_shunt_resistance_ohm <= 0.0) {
        return Status::invalid_argument;
    }
    bool any_reference_configured = false;
    for (const ReferenceCalibration& reference : configuration.references) {
        if (!reference.configured) {
            continue;
        }
        any_reference_configured = true;
        if (!std::isfinite(reference.calibrated_voltage_v) ||
            reference.calibrated_voltage_v <= 0.0) {
            return Status::invalid_argument;
        }
    }
    if (!any_reference_configured) {
        return Status::not_configured;
    }
    if (configuration.mass_calibration.degree > 3U ||
        !std::isfinite(configuration.mass_calibration.input_origin) ||
        !std::isfinite(configuration.mass_calibration.input_scale) ||
        configuration.mass_calibration.input_scale <= 0.0 ||
        !std::isfinite(configuration.mass_calibration.minimum_input_signal) ||
        !std::isfinite(configuration.mass_calibration.maximum_input_signal) ||
        configuration.mass_calibration.minimum_input_signal >=
            configuration.mass_calibration.maximum_input_signal) {
        return Status::invalid_argument;
    }
    for (const double coefficient :
         configuration.mass_calibration.coefficients) {
        if (!std::isfinite(coefficient)) {
            return Status::invalid_argument;
        }
    }
    return calibration::validate_temperature_model(
        configuration.temperature_model);
}

Status convert_measurement(
    const MeasurementConfiguration& configuration,
    const drivers::Ads1262::Conversion& conversion,
    const calibration::TemperatureReadings& temperatures,
    const TareState& tare,
    MeasurementSnapshot& snapshot) noexcept {
    snapshot = MeasurementSnapshot{};
    const Status validation = validate_measurement_configuration(configuration);
    if (!is_ok(validation)) {
        return validation;
    }
    if (!conversion_context_valid(conversion.context)) {
        return Status::not_configured;
    }
    if (conversion.saturated ||
        conversion.code == std::numeric_limits<std::int32_t>::max() ||
        conversion.code == std::numeric_limits<std::int32_t>::min() ||
        conversion.status.faulted()) {
        return Status::fault_active;
    }
    if (tare.valid && !std::isfinite(tare.offset_mg)) {
        return Status::invalid_argument;
    }

    const std::size_t selected_reference =
        reference_index(conversion.context.reference);
    const ReferenceCalibration& reference =
        configuration.references[selected_reference];
    if (!reference.configured) {
        return Status::not_configured;
    }

    snapshot.raw_adc_code = conversion.code;
    snapshot.adc_saturated = conversion.saturated;
    snapshot.adc_context = conversion.context;
    snapshot.calibrated_reference_voltage_v = reference.calibrated_voltage_v;
    snapshot.effective_pga_gain = conversion.context.pga_bypassed
                                      ? 1.0
                                      : drivers::Ads1262::gain_value(
                                            conversion.context.gain);
    snapshot.differential_voltage_v = adc_code_to_voltage(
        conversion.code,
        reference.calibrated_voltage_v,
        conversion.context.gain,
        conversion.context.pga_bypassed);
    snapshot.coil_current_a = snapshot.differential_voltage_v /
                               configuration.calibrated_shunt_resistance_ohm;
    if (!std::isfinite(snapshot.differential_voltage_v) ||
        !std::isfinite(snapshot.coil_current_a)) {
        return Status::verification_failed;
    }
    snapshot.uncorrected_mass_mg =
        configuration.mass_calibration.evaluate(snapshot.coil_current_a);
    if (!std::isfinite(snapshot.uncorrected_mass_mg)) {
        return Status::verification_failed;
    }

    const Status temperature_status = calibration::apply_temperature_compensation(
        configuration.temperature_model,
        temperatures,
        snapshot.uncorrected_mass_mg,
        snapshot.temperature);
    if (!is_ok(temperature_status)) {
        return temperature_status;
    }
    snapshot.temperature_corrected_mass_mg =
        snapshot.temperature.corrected_mass_mg;
    snapshot.tare_applied = tare.valid;
    snapshot.tare_offset_mg = tare.valid ? tare.offset_mg : 0.0;
    snapshot.final_mass_mg =
        snapshot.temperature_corrected_mass_mg - snapshot.tare_offset_mg;
    snapshot.final_mass_grains = milligrams_to_grains(snapshot.final_mass_mg);
    return std::isfinite(snapshot.final_mass_grains) ? Status::ok
                                                     : Status::verification_failed;
}

}  // namespace gravimetra::measurement
