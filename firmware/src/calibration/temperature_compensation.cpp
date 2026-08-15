#include "gravimetra/calibration/temperature_compensation.hpp"

#include <cmath>
#include <cstddef>

namespace gravimetra::calibration {

Status validate_temperature_model(
    const TemperatureCompensationModel& model) noexcept {
    if (!model.configured) {
        return Status::not_configured;
    }
    bool any_enabled = false;
    for (std::size_t index = 0U; index < kTemperatureLocationCount; ++index) {
        if (!model.sensor_enabled[index]) {
            continue;
        }
        any_enabled = true;
        if (!std::isfinite(model.reference_celsius[index]) ||
            model.reference_celsius[index] < -55.0 ||
            model.reference_celsius[index] > 150.0 ||
            !std::isfinite(model.offset_linear_mg_per_c[index]) ||
            !std::isfinite(model.offset_quadratic_mg_per_c2[index]) ||
            !std::isfinite(model.span_linear_per_c[index]) ||
            !std::isfinite(model.span_quadratic_per_c2[index])) {
            return Status::invalid_argument;
        }
    }
    return any_enabled ? Status::ok : Status::not_configured;
}

Status apply_temperature_compensation(
    const TemperatureCompensationModel& model,
    const TemperatureReadings& readings,
    const double uncorrected_mass_mg,
    TemperatureCorrection& correction) noexcept {
    correction = TemperatureCorrection{};
    const Status validation = validate_temperature_model(model);
    if (!is_ok(validation)) {
        return validation;
    }
    if (!std::isfinite(uncorrected_mass_mg)) {
        return Status::invalid_argument;
    }

    for (std::size_t index = 0U; index < kTemperatureLocationCount; ++index) {
        if (!model.sensor_enabled[index]) {
            continue;
        }
        if (!readings.valid[index]) {
            return Status::not_configured;
        }
        if (!std::isfinite(readings.celsius[index]) ||
            readings.celsius[index] < -55.0 || readings.celsius[index] > 150.0) {
            return Status::fault_active;
        }
        const double delta =
            readings.celsius[index] - model.reference_celsius[index];
        correction.offset_correction_mg +=
            (model.offset_linear_mg_per_c[index] * delta) +
            (model.offset_quadratic_mg_per_c2[index] * delta * delta);
        correction.span_correction_fraction +=
            (model.span_linear_per_c[index] * delta) +
            (model.span_quadratic_per_c2[index] * delta * delta);
    }
    const double effective_span = 1.0 + correction.span_correction_fraction;
    if (!std::isfinite(correction.offset_correction_mg) ||
        !std::isfinite(correction.span_correction_fraction) ||
        !std::isfinite(effective_span) || effective_span <= 0.0) {
        correction = TemperatureCorrection{};
        return Status::verification_failed;
    }
    correction.corrected_mass_mg =
        (uncorrected_mass_mg * effective_span) +
        correction.offset_correction_mg;
    correction.total_correction_mg =
        correction.corrected_mass_mg - uncorrected_mass_mg;
    if (!std::isfinite(correction.corrected_mass_mg) ||
        !std::isfinite(correction.total_correction_mg)) {
        correction = TemperatureCorrection{};
        return Status::verification_failed;
    }
    return Status::ok;
}

}  // namespace gravimetra::calibration
