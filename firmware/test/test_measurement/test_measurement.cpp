#include "gravimetra/calibration/calibration.hpp"
#include "gravimetra/calibration/temperature_compensation.hpp"
#include "gravimetra/drivers/ads1262.hpp"
#include "gravimetra/drivers/tmp117.hpp"
#include "gravimetra/measurement/filters.hpp"
#include "gravimetra/measurement/pipeline.hpp"
#include "gravimetra/measurement/stability.hpp"
#include "gravimetra/measurement/tare.hpp"
#include "gravimetra/measurement/units.hpp"

#include <unity.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace {

using gravimetra::Status;

void assert_status(const Status expected, const Status actual) {
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<std::uint8_t>(expected),
        static_cast<std::uint8_t>(actual));
}

class MockClock final : public gravimetra::hal::MonotonicClock {
public:
    [[nodiscard]] std::uint64_t now_us() const noexcept override {
        const std::uint64_t result = now;
        now += increment;
        return result;
    }

    mutable std::uint64_t now{0U};
    std::uint64_t increment{10U};
};

class MockDigitalInput final : public gravimetra::hal::DigitalInput {
public:
    [[nodiscard]] Status read(bool& asserted) noexcept override {
        asserted = ready;
        return result;
    }

    bool ready{true};
    Status result{Status::ok};
};

class MockSpi final : public gravimetra::hal::SpiBus {
public:
    MockSpi() { registers[2] = 0x05U; }

    [[nodiscard]] Status transfer(
        const std::uint8_t* transmit,
        std::uint8_t* receive,
        const std::size_t length,
        std::uint32_t) noexcept override {
        if (forced_result != Status::ok) {
            return forced_result;
        }
        if (transmit == nullptr || receive == nullptr || length == 0U) {
            return Status::invalid_argument;
        }
        ++transfer_count;
        if (fail_on_transfer != 0U && transfer_count == fail_on_transfer) {
            return fail_result;
        }
        for (std::size_t index = 0U; index < length; ++index) {
            receive[index] = 0U;
        }
        last_opcode = transmit[0];
        if (transmit[0] == 0x06U) {
            registers.fill(0U);
            registers[1] = 0x11U;
            registers[2] = 0x05U;
            registers[4] = 0x80U;
            registers[5] = 0x04U;
            registers[6] = 0x01U;
            return Status::ok;
        }
        if (transmit[0] == 0x08U || transmit[0] == 0x0AU) {
            return Status::ok;
        }
        if (transmit[0] == 0x12U) {
            std::size_t index = 1U;
            if ((registers[2] & 0x04U) != 0U) {
                receive[index] = conversion_status;
                ++index;
            }
            const std::array<std::uint8_t, 4U> data{
                static_cast<std::uint8_t>(conversion_code >> 24U),
                static_cast<std::uint8_t>(conversion_code >> 16U),
                static_cast<std::uint8_t>(conversion_code >> 8U),
                static_cast<std::uint8_t>(conversion_code),
            };
            for (const std::uint8_t byte : data) {
                receive[index] = byte;
                ++index;
            }
            if ((registers[2] & 0x03U) != 0x00U) {
                std::uint8_t integrity = 0U;
                if (use_integrity_override) {
                    integrity = integrity_override;
                } else if ((registers[2] & 0x03U) == 0x01U) {
                    integrity = 0x9BU;
                    for (const std::uint8_t byte : data) {
                        integrity = static_cast<std::uint8_t>(integrity + byte);
                    }
                } else {
                    for (const std::uint8_t byte : data) {
                        integrity = static_cast<std::uint8_t>(integrity ^ byte);
                        for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
                            integrity = (integrity & 0x80U) != 0U
                                            ? static_cast<std::uint8_t>(
                                                  (static_cast<std::uint32_t>(
                                                       integrity)
                                                   << 1U) ^
                                                  0x07U)
                                            : static_cast<std::uint8_t>(
                                                  static_cast<std::uint32_t>(
                                                      integrity)
                                                  << 1U);
                        }
                    }
                }
                receive[index] =
                    corrupt_integrity
                        ? static_cast<std::uint8_t>(integrity ^ 0x01U)
                        : integrity;
            }
            return Status::ok;
        }
        if ((transmit[0] & 0xE0U) == 0x20U && length >= 3U) {
            const std::size_t start = transmit[0] & 0x1FU;
            const std::size_t count = static_cast<std::size_t>(transmit[1]) + 1U;
            if (start + count > registers.size() || length != count + 2U) {
                return Status::protocol_error;
            }
            for (std::size_t index = 0U; index < count; ++index) {
                receive[index + 2U] = registers[start + index];
            }
            return Status::ok;
        }
        if ((transmit[0] & 0xE0U) == 0x40U && length >= 3U) {
            const std::size_t start = transmit[0] & 0x1FU;
            const std::size_t count = static_cast<std::size_t>(transmit[1]) + 1U;
            if (start + count > registers.size() || length != count + 2U) {
                return Status::protocol_error;
            }
            for (std::size_t index = 0U; index < count; ++index) {
                registers[start + index] = transmit[index + 2U];
            }
            return Status::ok;
        }
        return Status::protocol_error;
    }

    std::array<std::uint8_t, 21U> registers{};
    std::uint32_t conversion_code{0U};
    std::uint8_t conversion_status{0x40U};
    bool corrupt_integrity{false};
    bool use_integrity_override{false};
    std::uint8_t integrity_override{0U};
    Status forced_result{Status::ok};
    std::size_t fail_on_transfer{0U};
    Status fail_result{Status::io_error};
    std::uint8_t last_opcode{0U};
    std::size_t transfer_count{0U};
};

class MockI2c final : public gravimetra::hal::I2cBus {
public:
    [[nodiscard]] Status write(
        const std::uint8_t address,
        const std::uint8_t* data,
        const std::size_t length,
        std::uint32_t) noexcept override {
        last_address = address;
        if (result != Status::ok) {
            return result;
        }
        if (data == nullptr || length != 3U || data[0] >= registers.size()) {
            return Status::protocol_error;
        }
        registers[data[0]] = static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(data[1]) << 8U) |
            static_cast<std::uint16_t>(data[2]));
        return Status::ok;
    }

    [[nodiscard]] Status write_read(
        const std::uint8_t address,
        const std::uint8_t* transmit,
        const std::size_t transmit_length,
        std::uint8_t* receive,
        const std::size_t receive_length,
        std::uint32_t) noexcept override {
        last_address = address;
        if (result != Status::ok) {
            return result;
        }
        if (transmit == nullptr || receive == nullptr || transmit_length != 1U ||
            receive_length != 2U || transmit[0] >= registers.size()) {
            return Status::protocol_error;
        }
        const std::uint16_t value = registers[transmit[0]];
        receive[0] = static_cast<std::uint8_t>(value >> 8U);
        receive[1] = static_cast<std::uint8_t>(value);
        return Status::ok;
    }

    std::array<std::uint16_t, 16U> registers{};
    std::uint8_t last_address{0U};
    Status result{Status::ok};
};

gravimetra::measurement::StabilityConfiguration stability_configuration() {
    gravimetra::measurement::StabilityConfiguration configuration{};
    configuration.configured = true;
    configuration.window_size = 5U;
    configuration.minimum_valid_samples = 5U;
    configuration.minimum_stable_duration_us = 200'000U;
    configuration.maximum_sample_gap_us = 250'000U;
    configuration.maximum_absolute_slope_mg_per_s = 0.1;
    configuration.maximum_standard_deviation_mg = 0.05;
    configuration.maximum_peak_to_peak_mg = 0.15;
    configuration.require_optical_diagnostic = true;
    configuration.maximum_absolute_optical_error = 0.2;
    configuration.maximum_absolute_coil_current_a = 0.5;
    return configuration;
}

gravimetra::measurement::StabilitySample valid_sample(
    const std::uint64_t timestamp_us,
    const double mass_mg) {
    gravimetra::measurement::StabilitySample sample{};
    sample.timestamp_us = timestamp_us;
    sample.mass_mg = mass_mg;
    sample.measurement_valid = true;
    sample.optical_position_error = 0.01;
    sample.optical_valid = true;
    sample.coil_current_a = 0.1;
    return sample;
}

void test_exact_unit_conversion() {
    TEST_ASSERT_DOUBLE_WITHIN(
        1.0e-12,
        64.79891,
        gravimetra::measurement::grains_to_milligrams(1.0));
    TEST_ASSERT_DOUBLE_WITHIN(
        1.0e-12,
        1.0,
        gravimetra::measurement::milligrams_to_grains(64.79891));
}

void test_adc_code_to_voltage_and_full_pipeline() {
    using namespace gravimetra;
    measurement::MeasurementConfiguration configuration{};
    configuration.configured = true;
    const std::size_t reference_index = static_cast<std::size_t>(
        drivers::Ads1262::Reference::external_ain0_ain1);
    configuration.references[reference_index].configured = true;
    configuration.references[reference_index].calibrated_voltage_v = 2.5;
    configuration.calibrated_shunt_resistance_ohm = 10.0;
    configuration.mass_calibration.configured = true;
    configuration.mass_calibration.accepted = true;
    configuration.mass_calibration.degree = 1U;
    configuration.mass_calibration.input_origin = 0.0;
    configuration.mass_calibration.input_scale = 0.125;
    configuration.mass_calibration.minimum_input_signal = -0.125;
    configuration.mass_calibration.maximum_input_signal = 0.125;
    configuration.mass_calibration.coefficients = {0.0, 1000.0, 0.0, 0.0};
    configuration.temperature_model.configured = true;
    configuration.temperature_model.sensor_enabled[0] = true;
    configuration.temperature_model.reference_celsius[0] = 20.0;
    configuration.temperature_model.offset_linear_mg_per_c[0] = 2.0;
    configuration.temperature_model.span_linear_per_c[0] = 0.001;

    calibration::TemperatureReadings temperatures{};
    temperatures.valid[0] = true;
    temperatures.celsius[0] = 22.0;
    measurement::TareState tare{};
    tare.valid = true;
    tare.offset_mg = 6.0;
    drivers::Ads1262::Conversion conversion{};
    conversion.code = 1'073'741'824;
    conversion.context.verified = true;
    conversion.context.profile = drivers::Ads1262::ProfileId::fast_dispense;
    conversion.context.gain = drivers::Ads1262::Gain::x1;
    conversion.context.reference =
        drivers::Ads1262::Reference::external_ain0_ain1;
    measurement::MeasurementSnapshot snapshot{};
    assert_status(
        Status::ok,
        measurement::convert_measurement(
            configuration, conversion, temperatures, tare, snapshot));
    TEST_ASSERT_TRUE(snapshot.adc_context.verified);
    TEST_ASSERT_DOUBLE_WITHIN(
        1.0e-12, 2.5, snapshot.calibrated_reference_voltage_v);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 1.0, snapshot.effective_pga_gain);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 1.25, snapshot.differential_voltage_v);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 0.125, snapshot.coil_current_a);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-9, 1000.0, snapshot.uncorrected_mass_mg);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-9, 1006.0, snapshot.temperature_corrected_mass_mg);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-9, 1000.0, snapshot.final_mass_mg);

    conversion.context.gain = drivers::Ads1262::Gain::x32;
    assert_status(
        Status::ok,
        measurement::convert_measurement(
            configuration, conversion, temperatures, tare, snapshot));
    TEST_ASSERT_DOUBLE_WITHIN(
        1.0e-12, 0.0390625, snapshot.differential_voltage_v);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 32.0, snapshot.effective_pga_gain);

    conversion.context.reference = drivers::Ads1262::Reference::internal_2_5_v;
    assert_status(
        Status::not_configured,
        measurement::convert_measurement(
            configuration, conversion, temperatures, tare, snapshot));

    conversion.context.reference =
        drivers::Ads1262::Reference::external_ain0_ain1;
    conversion.context.gain = drivers::Ads1262::Gain::x1;
    conversion.code = 1'200'000'000;
    assert_status(
        Status::verification_failed,
        measurement::convert_measurement(
            configuration, conversion, temperatures, tare, snapshot));

    conversion.code = 1'073'741'824;
    configuration.mass_calibration.accepted = false;
    assert_status(
        Status::not_configured,
        measurement::convert_measurement(
            configuration, conversion, temperatures, tare, snapshot));

    configuration.mass_calibration.accepted = true;
    conversion.saturated = true;
    assert_status(
        Status::fault_active,
        measurement::convert_measurement(
            configuration, conversion, temperatures, tare, snapshot));
}

void test_polynomial_fit_and_quality_rejection() {
    using namespace gravimetra::calibration;
    std::array<CalibrationPoint, 7U> points{};
    for (std::size_t index = 0U; index < points.size(); ++index) {
        const double input = static_cast<double>(index);
        points[index] = CalibrationPoint{
            true,
            input,
            2.0 + (3.0 * input) + (0.5 * input * input) -
                (0.1 * input * input * input),
            0.01};
    }
    PolynomialCalibration model{};
    CalibrationQuality quality{};
    assert_status(
        Status::ok,
        fit_polynomial(points.data(), points.size(), 3U, model, quality));
    TEST_ASSERT_FALSE(model.accepted);
    const double input = 2.5;
    const double expected =
        2.0 + (3.0 * input) + (0.5 * input * input) -
        (0.1 * input * input * input);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-9, expected, model.evaluate(input));
    TEST_ASSERT_TRUE(std::isnan(model.evaluate(7.0)));
    (void)expected;
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-9, 0.0, quality.rms_residual_mg);

    CalibrationAcceptance acceptance{};
    acceptance.configured = true;
    acceptance.minimum_point_count = 7U;
    acceptance.minimum_input_span = 5.0;
    acceptance.maximum_rms_residual_mg = 0.001;
    acceptance.maximum_absolute_residual_mg = 0.001;
    acceptance.maximum_repeatability_stddev_mg = 0.005;
    CalibrationQuality caller_fabricated_quality = quality;
    caller_fabricated_quality.maximum_repeatability_stddev_mg = 0.0;
    CalibrationQuality verified_quality = caller_fabricated_quality;
    assert_status(
        Status::verification_failed,
        validate_calibration(
            model,
            points.data(),
            points.size(),
            acceptance,
            verified_quality));
    TEST_ASSERT_FALSE(model.accepted);
    TEST_ASSERT_DOUBLE_WITHIN(
        1.0e-12,
        0.01,
        verified_quality.maximum_repeatability_stddev_mg);

    acceptance.maximum_repeatability_stddev_mg = 0.02;
    assert_status(
        Status::ok,
        validate_calibration(
            model,
            points.data(),
            points.size(),
            acceptance,
            verified_quality));
    TEST_ASSERT_TRUE(model.accepted);

    std::array<CalibrationPoint, 7U> invalid_points = points;
    invalid_points[0].certified_mass_mg =
        std::numeric_limits<double>::quiet_NaN();
    assert_status(
        Status::invalid_argument,
        validate_calibration(
            model,
            invalid_points.data(),
            invalid_points.size(),
            acceptance,
            verified_quality));
    TEST_ASSERT_FALSE(model.accepted);
    TEST_ASSERT_EQUAL_UINT32(0U, verified_quality.point_count);
}

void test_temperature_model_uses_offset_and_span_terms() {
    using namespace gravimetra::calibration;
    TemperatureCompensationModel model{};
    model.configured = true;
    model.sensor_enabled = {true, true, false};
    model.reference_celsius = {20.0, 25.0, 0.0};
    model.offset_linear_mg_per_c = {1.0, -2.0, 0.0};
    model.span_linear_per_c = {0.001, 0.002, 0.0};
    TemperatureReadings readings{};
    readings.valid = {true, true, false};
    readings.celsius = {22.0, 24.0, 0.0};
    TemperatureCorrection correction{};
    assert_status(
        Status::ok,
        apply_temperature_compensation(model, readings, 1000.0, correction));
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 4.0, correction.offset_correction_mg);
    TEST_ASSERT_DOUBLE_WITHIN(
        1.0e-12, 0.0, correction.span_correction_fraction);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 1004.0, correction.corrected_mass_mg);

    model.sensor_enabled = {true, false, false};
    model.reference_celsius[0] = 20.0;
    model.offset_linear_mg_per_c[0] = 0.0;
    model.span_linear_per_c[0] = -1.0;
    readings.valid = {true, false, false};
    readings.celsius[0] = 21.0;
    assert_status(
        Status::verification_failed,
        apply_temperature_compensation(model, readings, 1000.0, correction));
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 0.0, correction.corrected_mass_mg);
}

void test_fixed_filters() {
    gravimetra::measurement::MovingAverageFilter<4U> average;
    double output = 0.0;
    assert_status(Status::ok, average.configure(3U));
    assert_status(Status::ok, average.update(1.0, output));
    assert_status(Status::ok, average.update(2.0, output));
    assert_status(Status::ok, average.update(6.0, output));
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 3.0, output);
    assert_status(Status::ok, average.update(10.0, output));
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 6.0, output);

    gravimetra::measurement::ExponentialFilter exponential;
    assert_status(Status::ok, exponential.configure(0.25));
    assert_status(Status::ok, exponential.update(4.0, output));
    assert_status(Status::ok, exponential.update(8.0, output));
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 5.0, output);

    gravimetra::measurement::MovingAverageFilter<2U> overflow_average;
    assert_status(Status::ok, overflow_average.configure(2U));
    const double maximum = std::numeric_limits<double>::max();
    assert_status(Status::ok, overflow_average.update(maximum, output));
    assert_status(
        Status::verification_failed,
        overflow_average.update(maximum, output));
    TEST_ASSERT_EQUAL_UINT32(1U, overflow_average.sample_count());
    assert_status(Status::ok, overflow_average.update(0.0, output));
    TEST_ASSERT_TRUE(std::isfinite(output));
    TEST_ASSERT_DOUBLE_WITHIN(maximum * 1.0e-12, maximum / 2.0, output);

    gravimetra::measurement::ExponentialFilter overflow_exponential;
    assert_status(Status::ok, overflow_exponential.configure(0.5));
    assert_status(Status::ok, overflow_exponential.update(maximum, output));
    assert_status(
        Status::verification_failed,
        overflow_exponential.update(-maximum, output));
    assert_status(Status::ok, overflow_exponential.update(maximum, output));
    TEST_ASSERT_EQUAL_DOUBLE(maximum, output);
}

void test_tare_acceptance_and_rejection() {
    gravimetra::measurement::TarePolicy policy{};
    policy.configured = true;
    policy.maximum_absolute_candidate_mg = 10.0;
    policy.maximum_candidate_stddev_mg = 0.05;
    gravimetra::measurement::TareState state{};
    gravimetra::measurement::TareCandidate candidate{};
    candidate.corrected_mass_mg = 4.0;
    candidate.standard_deviation_mg = 0.01;
    candidate.stable = true;
    candidate.fault_free = true;
    candidate.timestamp_us = 100U;
    assert_status(Status::ok, accept_tare(policy, candidate, state));
    TEST_ASSERT_TRUE(state.valid);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 4.0, state.offset_mg);

    candidate.stable = false;
    assert_status(
        Status::verification_failed, accept_tare(policy, candidate, state));
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 4.0, state.offset_mg);

    candidate.stable = true;
    candidate.timestamp_us = 100U;
    candidate.corrected_mass_mg = 3.0;
    assert_status(
        Status::verification_failed, accept_tare(policy, candidate, state));
    TEST_ASSERT_EQUAL_UINT64(100U, state.accepted_at_us);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 4.0, state.offset_mg);
}

void test_ads1262_protocol_signed_data_and_integrity() {
    MockSpi spi;
    MockDigitalInput data_ready;
    MockClock clock;
    gravimetra::drivers::Ads1262 adc(spi, data_ready, clock, 100U);
    assert_status(
        Status::ok,
        adc.configure_interface(
            true,
            true,
            gravimetra::drivers::Ads1262::DataIntegrity::checksum));
    spi.conversion_code = 0xFFFFFE00U;
    gravimetra::drivers::Ads1262::Conversion conversion{};
    assert_status(Status::ok, adc.read_conversion(conversion, 100U));
    TEST_ASSERT_EQUAL_INT32(-512, conversion.code);
    TEST_ASSERT_TRUE(conversion.status.adc1_new);
    TEST_ASSERT_EQUAL_HEX8(0x12U, spi.last_opcode);

    spi.corrupt_integrity = true;
    assert_status(Status::protocol_error, adc.read_conversion(conversion, 100U));
}

void test_ads1262_golden_integrity_vectors_and_clipping() {
    using Ads1262 = gravimetra::drivers::Ads1262;
    MockSpi spi;
    MockDigitalInput data_ready;
    MockClock clock;
    Ads1262 adc(spi, data_ready, clock, 100U);
    spi.conversion_code = 0x12345678U;
    spi.use_integrity_override = true;
    spi.integrity_override = 0xAFU;
    assert_status(
        Status::ok,
        adc.configure_interface(
            true, true, Ads1262::DataIntegrity::checksum));
    Ads1262::Conversion conversion{};
    assert_status(Status::ok, adc.read_conversion(conversion, 100U));
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, conversion.code);

    spi.integrity_override = 0x1CU;
    assert_status(
        Status::ok,
        adc.configure_interface(
            true, true, Ads1262::DataIntegrity::crc8_atm));
    assert_status(Status::ok, adc.read_conversion(conversion, 100U));
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, conversion.code);

    spi.use_integrity_override = false;
    spi.conversion_code = 0x7FFFFFFFU;
    assert_status(Status::fault_active, adc.read_conversion(conversion, 100U));
    TEST_ASSERT_TRUE(conversion.saturated);
    TEST_ASSERT_EQUAL_INT32(
        std::numeric_limits<std::int32_t>::max(), conversion.code);

    spi.conversion_code = 0x80000000U;
    assert_status(Status::fault_active, adc.read_conversion(conversion, 100U));
    TEST_ASSERT_TRUE(conversion.saturated);
    TEST_ASSERT_EQUAL_INT32(
        std::numeric_limits<std::int32_t>::min(), conversion.code);
}

void test_ads1262_timeout_and_status_fault() {
    MockSpi spi;
    MockDigitalInput data_ready;
    MockClock clock;
    gravimetra::drivers::Ads1262 adc(spi, data_ready, clock, 100U);
    data_ready.ready = false;
    assert_status(Status::timeout, adc.wait_data_ready(25U));

    data_ready.ready = true;
    assert_status(
        Status::ok,
        adc.configure_interface(
            true,
            true,
            gravimetra::drivers::Ads1262::DataIntegrity::checksum));
    spi.conversion_status = 0x50U;
    gravimetra::drivers::Ads1262::Conversion conversion{};
    assert_status(Status::fault_active, adc.read_conversion(conversion, 100U));
    TEST_ASSERT_TRUE(conversion.status.reference_alarm);
}

void test_ads1262_profile_register_verification() {
    MockSpi spi;
    MockDigitalInput data_ready;
    MockClock clock;
    gravimetra::drivers::Ads1262 adc(spi, data_ready, clock, 100U);
    spi.registers[0] = 0x03U;
    spi.registers[1] = 0x11U;
    assert_status(Status::ok, adc.verify_device());
    assert_status(Status::ok, adc.clear_reset_indicator());
    TEST_ASSERT_EQUAL_HEX8(0x01U, spi.registers[1]);
    gravimetra::drivers::Ads1262::Profile profile{};
    profile.configured = true;
    profile.id = gravimetra::drivers::Ads1262::ProfileId::fast_dispense;
    profile.positive_input = gravimetra::drivers::Ads1262::Input::ain2;
    profile.negative_input = gravimetra::drivers::Ads1262::Input::ain3;
    profile.filter = gravimetra::drivers::Ads1262::DigitalFilter::sinc1;
    profile.data_rate = gravimetra::drivers::Ads1262::DataRate::sps_400;
    profile.gain = gravimetra::drivers::Ads1262::Gain::x16;
    profile.reference =
        gravimetra::drivers::Ads1262::Reference::external_ain0_ain1;
    profile.settling_time_us = 0U;
    assert_status(Status::invalid_argument, adc.apply_profile(profile));
    profile.settling_time_us = 1000U;
    assert_status(Status::ok, adc.apply_profile(profile));
    TEST_ASSERT_EQUAL_HEX8(0x00U, spi.registers[4]);
    TEST_ASSERT_EQUAL_HEX8(0x48U, spi.registers[5]);
    TEST_ASSERT_EQUAL_HEX8(0x23U, spi.registers[6]);
    TEST_ASSERT_EQUAL_HEX8(0x09U, spi.registers[15]);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<std::uint8_t>(
            gravimetra::drivers::Ads1262::ProfileId::fast_dispense),
        static_cast<std::uint8_t>(adc.active_profile()));
    TEST_ASSERT_FALSE(adc.profile_settled());
    TEST_ASSERT_TRUE(adc.conversion_context().verified);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<std::uint8_t>(gravimetra::drivers::Ads1262::Gain::x16),
        static_cast<std::uint8_t>(adc.conversion_context().gain));

    spi.fail_on_transfer = spi.transfer_count + 1U;
    assert_status(Status::io_error, adc.apply_profile(profile));
    TEST_ASSERT_FALSE(adc.conversion_context().verified);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<std::uint8_t>(
            gravimetra::drivers::Ads1262::ProfileId::none),
        static_cast<std::uint8_t>(adc.active_profile()));
}

void test_ads1262_ambiguous_interface_and_internal_monitor_selection() {
    using Ads1262 = gravimetra::drivers::Ads1262;
    MockSpi spi;
    MockDigitalInput data_ready;
    MockClock clock;
    Ads1262 adc(spi, data_ready, clock, 100U);
    assert_status(
        Status::ok,
        adc.configure_interface(
            true, true, Ads1262::DataIntegrity::checksum));

    spi.fail_on_transfer = spi.transfer_count + 1U;
    assert_status(
        Status::io_error,
        adc.configure_interface(
            true, true, Ads1262::DataIntegrity::crc8_atm));
    spi.fail_on_transfer = 0U;
    Ads1262::Conversion conversion{};
    assert_status(Status::not_configured, adc.read_conversion(conversion, 100U));

    assert_status(
        Status::ok,
        adc.configure_interface(
            true, true, Ads1262::DataIntegrity::checksum));
    assert_status(
        Status::ok,
        adc.select_channel(
            Ads1262::Input::temperature_monitor,
            Ads1262::Input::temperature_monitor,
            0U));
    TEST_ASSERT_EQUAL_HEX8(0xBBU, spi.registers[6]);
    assert_status(
        Status::invalid_argument,
        adc.select_channel(
            Ads1262::Input::tdac_test, Ads1262::Input::tdac_test, 0U));
}

void test_tmp117_register_protocol_and_negative_temperature() {
    MockI2c bus;
    bus.registers[15] = 0xA117U;
    bus.registers[0] = 0xF600U;
    gravimetra::drivers::Tmp117 sensor(bus, 0x49U, 100U);
    assert_status(Status::ok, sensor.verify_device());
    gravimetra::drivers::Tmp117::Reading reading{};
    assert_status(Status::ok, sensor.read_temperature(reading));
    TEST_ASSERT_EQUAL_INT16(-2560, reading.raw_code);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, -20.0, reading.celsius);
    TEST_ASSERT_EQUAL_HEX8(0x49U, bus.last_address);
    bus.registers[0] = 0x8000U;
    assert_status(Status::busy, sensor.read_temperature(reading));
}

void test_stability_requires_continuous_duration() {
    gravimetra::measurement::StabilityDetector detector;
    assert_status(Status::ok, detector.configure(stability_configuration()));
    for (std::uint64_t index = 0U; index < 7U; ++index) {
        assert_status(
            Status::ok,
            detector.add_sample(valid_sample(index * 100'000U, 10.0)));
    }
    TEST_ASSERT_TRUE(detector.diagnostics().stable);
    TEST_ASSERT_EQUAL_UINT64(
        200'000U, detector.diagnostics().continuously_passing_duration_us);
    TEST_ASSERT_DOUBLE_WITHIN(
        1.0e-12, 0.0, detector.diagnostics().slope_mg_per_s);
}

void test_noisy_trace_eventually_settles() {
    auto configuration = stability_configuration();
    configuration.window_size = 4U;
    configuration.minimum_valid_samples = 4U;
    configuration.minimum_stable_duration_us = 100'000U;
    gravimetra::measurement::StabilityDetector detector;
    assert_status(Status::ok, detector.configure(configuration));
    const std::array<double, 4U> noisy{9.0, 11.0, 8.0, 12.0};
    std::uint64_t time = 0U;
    for (const double value : noisy) {
        assert_status(Status::ok, detector.add_sample(valid_sample(time, value)));
        time += 100'000U;
    }
    TEST_ASSERT_FALSE(detector.diagnostics().stable);
    TEST_ASSERT_TRUE(detector.diagnostics().has_reason(
        gravimetra::measurement::StabilityReason::excessive_peak_to_peak));
    for (std::size_t index = 0U; index < 5U; ++index) {
        assert_status(Status::ok, detector.add_sample(valid_sample(time, 10.0)));
        time += 100'000U;
    }
    TEST_ASSERT_TRUE(detector.diagnostics().stable);
}

void test_slow_drift_and_auxiliary_fault_diagnostics() {
    auto configuration = stability_configuration();
    configuration.maximum_absolute_slope_mg_per_s = 0.05;
    configuration.minimum_stable_duration_us = 0U;
    gravimetra::measurement::StabilityDetector detector;
    assert_status(Status::ok, detector.configure(configuration));
    for (std::uint64_t index = 0U; index < 5U; ++index) {
        assert_status(
            Status::ok,
            detector.add_sample(
                valid_sample(index * 100'000U, static_cast<double>(index) * 0.02)));
    }
    TEST_ASSERT_TRUE(detector.diagnostics().has_reason(
        gravimetra::measurement::StabilityReason::excessive_slope));

    detector.reset();
    for (std::uint64_t index = 0U; index < 5U; ++index) {
        auto sample = valid_sample(index * 100'000U, 1.0);
        if (index == 4U) {
            sample.optical_position_error = 0.3;
            sample.coil_current_a = 0.5;
            sample.power_fault = true;
            sample.temperature_fault = true;
        }
        assert_status(Status::ok, detector.add_sample(sample));
    }
    TEST_ASSERT_TRUE(detector.diagnostics().has_reason(
        gravimetra::measurement::StabilityReason::optical_out_of_bounds));
    TEST_ASSERT_TRUE(detector.diagnostics().has_reason(
        gravimetra::measurement::StabilityReason::current_saturated));
    TEST_ASSERT_TRUE(detector.diagnostics().has_reason(
        gravimetra::measurement::StabilityReason::power_fault));
    TEST_ASSERT_TRUE(detector.diagnostics().has_reason(
        gravimetra::measurement::StabilityReason::temperature_fault));
}

void test_stability_rejects_nonfinite_sample_and_large_gap() {
    using gravimetra::measurement::StabilityReason;
    auto configuration = stability_configuration();
    configuration.maximum_sample_gap_us = 150'000U;
    gravimetra::measurement::StabilityDetector detector;
    assert_status(Status::ok, detector.configure(configuration));
    for (std::uint64_t index = 0U; index < 7U; ++index) {
        assert_status(
            Status::ok,
            detector.add_sample(valid_sample(index * 100'000U, 10.0)));
    }
    TEST_ASSERT_TRUE(detector.diagnostics().stable);

    auto invalid = valid_sample(700'000U, 10.0);
    invalid.mass_mg = std::numeric_limits<double>::quiet_NaN();
    assert_status(Status::invalid_argument, detector.add_sample(invalid));
    TEST_ASSERT_FALSE(detector.diagnostics().stable);
    TEST_ASSERT_TRUE(
        detector.diagnostics().has_reason(StabilityReason::invalid_measurement));
    TEST_ASSERT_EQUAL_UINT64(
        0U, detector.diagnostics().continuously_passing_duration_us);

    assert_status(
        Status::ok, detector.add_sample(valid_sample(1'000'000U, 10.0)));
    TEST_ASSERT_FALSE(detector.diagnostics().stable);
    TEST_ASSERT_EQUAL_UINT32(1U, detector.diagnostics().sample_count);
    TEST_ASSERT_TRUE(detector.diagnostics().has_reason(StabilityReason::sample_gap));
    TEST_ASSERT_EQUAL_UINT64(
        0U, detector.diagnostics().continuously_passing_duration_us);
}

}  // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_exact_unit_conversion);
    RUN_TEST(test_adc_code_to_voltage_and_full_pipeline);
    RUN_TEST(test_polynomial_fit_and_quality_rejection);
    RUN_TEST(test_temperature_model_uses_offset_and_span_terms);
    RUN_TEST(test_fixed_filters);
    RUN_TEST(test_tare_acceptance_and_rejection);
    RUN_TEST(test_ads1262_protocol_signed_data_and_integrity);
    RUN_TEST(test_ads1262_golden_integrity_vectors_and_clipping);
    RUN_TEST(test_ads1262_timeout_and_status_fault);
    RUN_TEST(test_ads1262_profile_register_verification);
    RUN_TEST(test_ads1262_ambiguous_interface_and_internal_monitor_selection);
    RUN_TEST(test_tmp117_register_protocol_and_negative_temperature);
    RUN_TEST(test_stability_requires_continuous_duration);
    RUN_TEST(test_noisy_trace_eventually_settles);
    RUN_TEST(test_slow_drift_and_auxiliary_fault_diagnostics);
    RUN_TEST(test_stability_rejects_nonfinite_sample_and_large_gap);
    return UNITY_END();
}
