#include "gravimetra/system/telemetry.hpp"

#include <cmath>
#include <cstring>
#include <limits>

namespace gravimetra::system {
namespace {

constexpr char kHeader[] =
    "monotonic_time_us,application_state,raw_ads1262_code,shunt_voltage_v,"
    "coil_current_a,raw_mass_mg,corrected_mass_mg,tared_mass_mg,"
    "optical_difference,optical_sum,magnet_yoke_temperature_c,"
    "flexure_body_temperature_c,precision_afe_temperature_c,active_auger,"
    "active_stage,commanded_motor_speed_steps_per_s,commanded_steps,"
    "tmc_driver_status,battery_voltage_v,power_source,charger_state,"
    "estop_active,adc_profile,stability_standard_deviation_mg,"
    "stability_slope_mg_per_s,stability_peak_to_peak_mg,"
    "stability_reason_flags,stability_sample_count,"
    "stability_valid_sample_count,stability_window_duration_us,"
    "stability_continuously_passing_duration_us,stable,fault_flags,"
    "calibration_version,validity_flags\n";

class CsvWriter {
public:
    CsvWriter(char* const destination, const std::size_t capacity) noexcept
        : destination_(destination), capacity_(capacity) {
        if (destination_ != nullptr && capacity_ != 0U) {
            destination_[0U] = '\0';
        }
    }

    void empty_field() noexcept { begin_field(); }

    void unsigned_field(
        const std::uint64_t value,
        const bool valid = true) noexcept {
        begin_field();
        if (valid) {
            append_unsigned(value);
        }
    }

    void signed_field(const std::int64_t value, const bool valid) noexcept {
        begin_field();
        if (!valid) {
            return;
        }
        if (value < 0) {
            append('-');
            const std::uint64_t magnitude =
                static_cast<std::uint64_t>(-(value + 1)) + 1U;
            append_unsigned(magnitude);
        } else {
            append_unsigned(static_cast<std::uint64_t>(value));
        }
    }

    void double_field(const double value, const bool valid) noexcept {
        begin_field();
        if (valid) {
            append_fixed(value);
        }
    }

    void bool_field(const bool value, const bool valid) noexcept {
        unsigned_field(value ? 1U : 0U, valid);
    }

    [[nodiscard]] Status finish(std::size_t& written) noexcept {
        append('\n');
        if (overflow_) {
            destination_[0U] = '\0';
            written = 0U;
            return Status::queue_full;
        }
        if (invalid_numeric_) {
            destination_[0U] = '\0';
            written = 0U;
            return Status::invalid_argument;
        }
        destination_[position_] = '\0';
        written = position_;
        return Status::ok;
    }

private:
    static constexpr std::uint64_t kFractionScale = 1'000'000'000U;
    static constexpr std::size_t kFractionDigits = 9U;

    void begin_field() noexcept {
        if (!first_field_) {
            append(',');
        }
        first_field_ = false;
    }

    void append(const char character) noexcept {
        if (overflow_) {
            return;
        }
        // Keep one byte reserved for the trailing null terminator.
        if (position_ + 1U >= capacity_) {
            overflow_ = true;
            return;
        }
        destination_[position_] = character;
        ++position_;
    }

    void append_unsigned(std::uint64_t value) noexcept {
        char digits[20U]{};
        std::size_t count = 0U;
        do {
            digits[count] = static_cast<char>('0' + (value % 10U));
            ++count;
            value /= 10U;
        } while (value != 0U);
        while (count != 0U) {
            --count;
            append(digits[count]);
        }
    }

    void append_fixed(const double value) noexcept {
        if (!std::isfinite(value)) {
            invalid_numeric_ = true;
            return;
        }

        const double absolute = std::fabs(value);
        constexpr std::uint64_t maximum_whole =
            (std::numeric_limits<std::uint64_t>::max() / kFractionScale) - 1U;
        if (absolute > static_cast<double>(maximum_whole)) {
            invalid_numeric_ = true;
            return;
        }

        const auto scaled = static_cast<std::uint64_t>(
            (absolute * static_cast<double>(kFractionScale)) + 0.5);
        if (std::signbit(value) && scaled != 0U) {
            append('-');
        }
        append_unsigned(scaled / kFractionScale);

        std::uint64_t fraction = scaled % kFractionScale;
        if (fraction == 0U) {
            return;
        }
        char digits[kFractionDigits]{};
        for (std::size_t index = kFractionDigits; index != 0U; --index) {
            digits[index - 1U] = static_cast<char>('0' + (fraction % 10U));
            fraction /= 10U;
        }
        std::size_t used = kFractionDigits;
        while (used != 0U && digits[used - 1U] == '0') {
            --used;
        }
        append('.');
        for (std::size_t index = 0U; index < used; ++index) {
            append(digits[index]);
        }
    }

    char* destination_{nullptr};
    std::size_t capacity_{0U};
    std::size_t position_{0U};
    bool first_field_{true};
    bool overflow_{false};
    bool invalid_numeric_{false};
};

[[nodiscard]] bool valid(
    const TelemetrySample& sample,
    const TelemetryField field) noexcept {
    return (sample.valid_fields & telemetry_field_mask(field)) != 0U;
}

}  // namespace

Status TelemetryCsv::serialize_header(
    char* const destination,
    const std::size_t capacity,
    std::size_t& written) noexcept {
    written = 0U;
    if (destination == nullptr || capacity == 0U) {
        return Status::invalid_argument;
    }
    constexpr std::size_t length = sizeof(kHeader) - 1U;
    if (capacity <= length) {
        destination[0U] = '\0';
        return Status::queue_full;
    }
    std::memcpy(destination, kHeader, length + 1U);
    written = length;
    return Status::ok;
}

Status TelemetryCsv::serialize(
    const TelemetrySample& sample,
    char* const destination,
    const std::size_t capacity,
    std::size_t& written) noexcept {
    written = 0U;
    if (destination == nullptr || capacity == 0U) {
        return Status::invalid_argument;
    }

    CsvWriter writer(destination, capacity);
    writer.unsigned_field(
        sample.monotonic_time_us,
        valid(sample, TelemetryField::monotonic_time));
    writer.unsigned_field(
        sample.application_state,
        valid(sample, TelemetryField::application_state));
    writer.signed_field(
        sample.raw_ads1262_code,
        valid(sample, TelemetryField::raw_ads1262_code));
    writer.double_field(
        sample.shunt_voltage_v,
        valid(sample, TelemetryField::shunt_voltage));
    writer.double_field(
        sample.coil_current_a,
        valid(sample, TelemetryField::coil_current));
    writer.double_field(sample.raw_mass_mg, valid(sample, TelemetryField::raw_mass));
    writer.double_field(
        sample.corrected_mass_mg,
        valid(sample, TelemetryField::corrected_mass));
    writer.double_field(
        sample.tared_mass_mg,
        valid(sample, TelemetryField::tared_mass));
    writer.double_field(
        sample.optical_difference,
        valid(sample, TelemetryField::optical_difference));
    writer.double_field(
        sample.optical_sum,
        valid(sample, TelemetryField::optical_sum));
    writer.double_field(
        sample.magnet_yoke_temperature_c,
        valid(sample, TelemetryField::magnet_yoke_temperature));
    writer.double_field(
        sample.flexure_body_temperature_c,
        valid(sample, TelemetryField::flexure_body_temperature));
    writer.double_field(
        sample.precision_afe_temperature_c,
        valid(sample, TelemetryField::precision_afe_temperature));

    const bool motion_valid = valid(sample, TelemetryField::motion);
    writer.signed_field(sample.active_auger, motion_valid);
    writer.unsigned_field(sample.active_stage, motion_valid);
    writer.unsigned_field(sample.commanded_motor_speed_steps_per_s, motion_valid);
    writer.unsigned_field(sample.commanded_steps, motion_valid);
    writer.unsigned_field(
        sample.tmc_driver_status,
        valid(sample, TelemetryField::tmc_driver_status));
    writer.double_field(
        sample.battery_voltage_v,
        valid(sample, TelemetryField::battery_voltage));

    const bool power_valid = valid(sample, TelemetryField::power_state);
    writer.unsigned_field(static_cast<std::uint8_t>(sample.power_source), power_valid);
    writer.unsigned_field(static_cast<std::uint8_t>(sample.charger_state), power_valid);
    writer.bool_field(sample.estop_active, valid(sample, TelemetryField::estop_state));
    writer.unsigned_field(sample.adc_profile, valid(sample, TelemetryField::adc_profile));

    const bool stability_valid = valid(
        sample, TelemetryField::stability_diagnostics);
    writer.double_field(sample.stability_standard_deviation_mg, stability_valid);
    writer.double_field(sample.stability_slope_mg_per_s, stability_valid);
    writer.double_field(sample.stability_peak_to_peak_mg, stability_valid);
    writer.unsigned_field(sample.stability_reason_flags, stability_valid);
    writer.unsigned_field(sample.stability_sample_count, stability_valid);
    writer.unsigned_field(sample.stability_valid_sample_count, stability_valid);
    writer.unsigned_field(sample.stability_window_duration_us, stability_valid);
    writer.unsigned_field(
        sample.stability_continuously_passing_duration_us,
        stability_valid);
    writer.bool_field(sample.stable, stability_valid);
    writer.unsigned_field(
        sample.fault_flags,
        valid(sample, TelemetryField::fault_flags));
    writer.unsigned_field(
        sample.calibration_version,
        valid(sample, TelemetryField::calibration_version));
    writer.unsigned_field(sample.valid_fields);
    return writer.finish(written);
}

}  // namespace gravimetra::system
