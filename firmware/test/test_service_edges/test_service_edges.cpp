#include "gravimetra/comms/service_framing.hpp"
#include "gravimetra/comms/transports.hpp"
#include "gravimetra/hmi/nextion.hpp"
#include "gravimetra/hmi/ui_model.hpp"
#include "gravimetra/system/fault_manager.hpp"
#include "gravimetra/system/health_supervisor.hpp"
#include "gravimetra/system/telemetry.hpp"

#include <unity.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {

class FakeClock final : public gravimetra::hal::MonotonicClock {
public:
    [[nodiscard]] std::uint64_t now_us() const noexcept override { return now; }
    std::uint64_t now{0U};
};

class CopyingUart final : public gravimetra::hal::Uart {
public:
    [[nodiscard]] gravimetra::Status write(
        const std::uint8_t* const data,
        const std::size_t length) noexcept override {
        if (!gravimetra::is_ok(write_status)) {
            return write_status;
        }
        if (data == nullptr || length > transmitted.size()) {
            return gravimetra::Status::io_error;
        }
        std::copy_n(data, length, transmitted.begin());
        transmitted_length = length;
        return gravimetra::Status::ok;
    }

    [[nodiscard]] std::size_t read(
        std::uint8_t*,
        std::size_t) noexcept override {
        return 0U;
    }

    std::array<std::uint8_t, 256U> transmitted{};
    std::size_t transmitted_length{0U};
    gravimetra::Status write_status{gravimetra::Status::ok};
};

class FakeOutput final : public gravimetra::hal::DigitalOutput {
public:
    [[nodiscard]] gravimetra::Status write(
        const bool asserted) noexcept override {
        if (gravimetra::is_ok(status)) {
            state = asserted;
        }
        return status;
    }

    bool state{false};
    gravimetra::Status status{gravimetra::Status::ok};
};

class FakeWatchdog final : public gravimetra::hal::Watchdog {
public:
    void refresh() noexcept override { ++refresh_count; }
    std::uint32_t refresh_count{0U};
};

void assert_status(
    const gravimetra::Status expected,
    const gravimetra::Status actual) {
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(expected), static_cast<int>(actual));
}

void ingest_ok(
    gravimetra::hmi::NextionParser& parser,
    const std::uint8_t* const data,
    const std::size_t length) {
    for (std::size_t index = 0U; index < length; ++index) {
        assert_status(gravimetra::Status::ok, parser.ingest(data[index]));
    }
}

void ingest_service_ok(
    gravimetra::comms::ServiceFrameParser& parser,
    const std::uint8_t* const data,
    const std::size_t length) {
    for (std::size_t index = 0U; index < length; ++index) {
        assert_status(gravimetra::Status::ok, parser.ingest(data[index]));
    }
}

void test_nextion_numeric_payload_may_contain_three_ff_bytes() {
    constexpr std::array<std::uint8_t, 8U> frame{
        0x71U, 0xFFU, 0xFFU, 0xFFU, 0x00U, 0xFFU, 0xFFU, 0xFFU};
    gravimetra::hmi::NextionParser parser;
    ingest_ok(parser, frame.data(), frame.size());

    gravimetra::hmi::NextionEvent event{};
    TEST_ASSERT_TRUE(parser.pop(event));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::hmi::NextionEventType::numeric_value),
        static_cast<int>(event.type));
    TEST_ASSERT_EQUAL_HEX32(0x00FFFFFFU, event.numeric_value);

    constexpr std::array<std::uint8_t, 7U> invalid_touch{
        0x65U, 1U, 2U, 2U, 0xFFU, 0xFFU, 0xFFU};
    for (std::size_t index = 0U; index + 1U < invalid_touch.size(); ++index) {
        assert_status(
            gravimetra::Status::ok, parser.ingest(invalid_touch[index]));
    }
    assert_status(
        gravimetra::Status::protocol_error,
        parser.ingest(invalid_touch.back()));
    TEST_ASSERT_FALSE(parser.pop(event));
}

void test_ui_target_value_is_correlated_and_range_checked() {
    gravimetra::hmi::UiRequestModel model;
    assert_status(
        gravimetra::Status::invalid_argument,
        model.bind(gravimetra::hmi::UiBinding{
            1U,
            2U,
            gravimetra::hmi::UiTouchEdge::release,
            gravimetra::hmi::UiRequestType::set_target}));
    assert_status(
        gravimetra::Status::ok,
        model.bind(gravimetra::hmi::UiBinding{
            1U,
            2U,
            gravimetra::hmi::UiTouchEdge::release,
            gravimetra::hmi::UiRequestType::set_target,
            true,
            100U,
            200U}));

    gravimetra::hmi::NextionEvent touch{};
    touch.type = gravimetra::hmi::NextionEventType::touch;
    touch.page_id = 1U;
    touch.component_id = 2U;
    touch.pressed = false;
    assert_status(gravimetra::Status::ok, model.accept(touch));
    TEST_ASSERT_EQUAL_UINT32(0U, model.pending_requests());

    gravimetra::hmi::UiRequest pending{};
    TEST_ASSERT_TRUE(model.pending_numeric_request(pending));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::hmi::UiRequestType::set_target),
        static_cast<int>(pending.type));

    gravimetra::hmi::NextionEvent numeric{};
    numeric.type = gravimetra::hmi::NextionEventType::numeric_value;
    numeric.numeric_value = 150U;
    assert_status(gravimetra::Status::ok, model.accept(numeric));
    gravimetra::hmi::UiRequest completed{};
    TEST_ASSERT_TRUE(model.pop(completed));
    TEST_ASSERT_EQUAL_UINT32(150U, completed.value);

    assert_status(gravimetra::Status::ok, model.accept(touch));
    numeric.numeric_value = 99U;
    assert_status(gravimetra::Status::invalid_argument, model.accept(numeric));
    TEST_ASSERT_FALSE(model.pop(completed));
    TEST_ASSERT_FALSE(model.pending_numeric_request(pending));
}

void test_service_parser_recovers_overlapping_preamble_after_crc_failure() {
    gravimetra::comms::ServiceFrameParser parser;
    constexpr std::array<std::uint8_t, 8U> truncated{
        0xA5U, 0x5AU, 1U, 1U, 1U, 0U, 0U, 0U};
    TEST_ASSERT_NOT_EQUAL(
        0x5AA5U,
        gravimetra::comms::crc16_ccitt(&truncated[2U], 6U));
    ingest_service_ok(parser, truncated.data(), truncated.size());
    assert_status(gravimetra::Status::ok, parser.ingest(0xA5U));
    assert_status(
        gravimetra::Status::verification_failed, parser.ingest(0x5AU));

    gravimetra::comms::ServiceMessage message{};
    message.message_class = gravimetra::comms::ServiceMessageClass::event;
    message.sequence = 9U;
    message.payload[0U] = 0x42U;
    message.payload_length = 1U;
    std::array<std::uint8_t, gravimetra::comms::kServiceEncodedCapacity> frame{};
    std::size_t written = 0U;
    assert_status(
        gravimetra::Status::ok,
        gravimetra::comms::encode_service_message(
            message, frame.data(), frame.size(), written));
    ingest_service_ok(parser, &frame[2U], written - 2U);

    gravimetra::comms::ServiceMessage received{};
    TEST_ASSERT_TRUE(parser.pop(received));
    TEST_ASSERT_EQUAL_UINT16(9U, received.sequence);
    TEST_ASSERT_EQUAL_HEX8(0x42U, received.payload[0U]);
}

void test_service_parser_recovers_frame_swallowed_by_corrupted_length() {
    gravimetra::comms::ServiceMessage inner{};
    inner.message_class = gravimetra::comms::ServiceMessageClass::event;
    inner.sequence = 37U;
    inner.payload[0U] = 0x42U;
    inner.payload_length = 1U;
    std::array<std::uint8_t, gravimetra::comms::kServiceEncodedCapacity>
        inner_frame{};
    std::size_t inner_length = 0U;
    assert_status(
        gravimetra::Status::ok,
        gravimetra::comms::encode_service_message(
            inner, inner_frame.data(), inner_frame.size(), inner_length));

    // This valid outer header falsely declares the entire following frame as
    // payload. Its deliberately bad CRC must not make the inner frame vanish.
    std::array<std::uint8_t, gravimetra::comms::kServiceEncodedCapacity> outer{};
    outer[0U] = gravimetra::comms::kServicePreambleFirst;
    outer[1U] = gravimetra::comms::kServicePreambleSecond;
    outer[2U] = gravimetra::comms::kServiceProtocolVersion;
    outer[3U] = static_cast<std::uint8_t>(
        gravimetra::comms::ServiceMessageClass::request);
    outer[4U] = 1U;
    outer[5U] = 0U;
    outer[6U] = static_cast<std::uint8_t>(inner_length & 0xFFU);
    outer[7U] = static_cast<std::uint8_t>((inner_length >> 8U) & 0xFFU);
    std::copy_n(inner_frame.begin(), inner_length, outer.begin() + 8U);
    const std::size_t crc_offset = 8U + inner_length;
    const std::uint16_t outer_crc = gravimetra::comms::crc16_ccitt(
        &outer[2U], 6U + inner_length);
    const std::uint16_t rejected_crc =
        static_cast<std::uint16_t>(outer_crc ^ 0x0001U);
    outer[crc_offset] = static_cast<std::uint8_t>(rejected_crc & 0xFFU);
    outer[crc_offset + 1U] =
        static_cast<std::uint8_t>((rejected_crc >> 8U) & 0xFFU);

    gravimetra::comms::ServiceFrameParser parser;
    ingest_service_ok(parser, outer.data(), crc_offset + 1U);
    assert_status(
        gravimetra::Status::verification_failed,
        parser.ingest(outer[crc_offset + 1U]));

    gravimetra::comms::ServiceMessage recovered{};
    TEST_ASSERT_TRUE(parser.pop(recovered));
    TEST_ASSERT_EQUAL_UINT16(37U, recovered.sequence);
    TEST_ASSERT_EQUAL_HEX8(0x42U, recovered.payload[0U]);
}

void test_service_timeout_preserves_verified_queue() {
    FakeClock clock;
    gravimetra::comms::ServiceFrameParser parser;
    assert_status(
        gravimetra::Status::ok,
        parser.configure_inter_byte_timeout(clock, 100U));

    gravimetra::comms::ServiceMessage message{};
    message.message_class = gravimetra::comms::ServiceMessageClass::response;
    std::array<std::uint8_t, gravimetra::comms::kServiceEncodedCapacity> frame{};
    std::size_t written = 0U;
    assert_status(
        gravimetra::Status::ok,
        gravimetra::comms::encode_service_message(
            message, frame.data(), frame.size(), written));
    ingest_service_ok(parser, frame.data(), written);
    TEST_ASSERT_EQUAL_UINT32(1U, parser.pending());

    assert_status(gravimetra::Status::ok, parser.ingest(0xA5U));
    clock.now = 100U;
    assert_status(gravimetra::Status::timeout, parser.service_timeout());
    TEST_ASSERT_EQUAL_UINT32(1U, parser.pending());
    TEST_ASSERT_EQUAL_UINT32(1U, parser.timeout_resets());

    ingest_service_ok(parser, frame.data(), written);
    TEST_ASSERT_EQUAL_UINT32(2U, parser.pending());
}

void test_rs485_requires_and_enforces_configured_tx_deadline() {
    FakeClock clock;
    CopyingUart uart;
    FakeOutput direction;
    gravimetra::comms::Rs485HalfDuplexTransport transport(uart, direction);
    assert_status(
        gravimetra::Status::ok, transport.initialize_receive_mode());

    std::array<std::uint8_t, 2U> payload{0x12U, 0x34U};
    assert_status(
        gravimetra::Status::not_configured,
        transport.send(payload.data(), payload.size()));
    assert_status(
        gravimetra::Status::invalid_argument,
        transport.configure_transmit_timeout(clock, 0U));
    assert_status(
        gravimetra::Status::ok,
        transport.configure_transmit_timeout(clock, 50U));
    assert_status(
        gravimetra::Status::ok,
        transport.send(payload.data(), payload.size()));
    payload[0U] = 0xFFU;
    TEST_ASSERT_EQUAL_HEX8(0x12U, uart.transmitted[0U]);
    TEST_ASSERT_TRUE(direction.state);

    clock.now = 49U;
    assert_status(gravimetra::Status::busy, transport.service());
    clock.now = 50U;
    assert_status(gravimetra::Status::timeout, transport.service());
    TEST_ASSERT_FALSE(direction.state);
    TEST_ASSERT_FALSE(transport.transmitting());
}

void test_watchdog_requires_fresh_progress_after_reenable() {
    FakeWatchdog watchdog;
    const gravimetra::system::SubsystemMask required =
        gravimetra::system::subsystem_mask(
            gravimetra::system::Subsystem::measurement) |
        gravimetra::system::subsystem_mask(
            gravimetra::system::Subsystem::safety);
    gravimetra::system::HealthSupervisor supervisor(watchdog, required);
    assert_status(
        gravimetra::Status::ok,
        supervisor.note_progress(gravimetra::system::Subsystem::measurement));
    assert_status(
        gravimetra::Status::ok,
        supervisor.note_progress(gravimetra::system::Subsystem::safety));
    assert_status(gravimetra::Status::ok, supervisor.try_refresh());

    supervisor.set_refresh_permitted(false);
    assert_status(
        gravimetra::Status::ok,
        supervisor.note_progress(gravimetra::system::Subsystem::measurement));
    assert_status(
        gravimetra::Status::ok,
        supervisor.note_progress(gravimetra::system::Subsystem::safety));
    supervisor.set_refresh_permitted(true);
    assert_status(gravimetra::Status::busy, supervisor.try_refresh());
    assert_status(
        gravimetra::Status::ok,
        supervisor.note_progress(gravimetra::system::Subsystem::measurement));
    assert_status(gravimetra::Status::busy, supervisor.try_refresh());
    assert_status(
        gravimetra::Status::ok,
        supervisor.note_progress(gravimetra::system::Subsystem::safety));
    assert_status(gravimetra::Status::ok, supervisor.try_refresh());
    TEST_ASSERT_EQUAL_UINT32(2U, watchdog.refresh_count);
}

void test_clear_all_faults_records_each_transition() {
    gravimetra::system::FaultManager faults;
    gravimetra::system::FaultSnapshot raised{};
    raised.valid_fields = gravimetra::system::fault_snapshot_field_mask(
        gravimetra::system::FaultSnapshotField::monotonic_time);
    raised.monotonic_time_us = 10U;
    assert_status(
        gravimetra::Status::ok,
        faults.raise(gravimetra::system::FaultCode::estop_active, raised));
    assert_status(
        gravimetra::Status::ok,
        faults.raise(gravimetra::system::FaultCode::charger_fault, raised));

    gravimetra::system::FaultSnapshot cleared = raised;
    cleared.monotonic_time_us = 20U;
    cleared.valid_fields |= gravimetra::system::fault_snapshot_field_mask(
        gravimetra::system::FaultSnapshotField::motion_diagnostics);
    cleared.valid_fields |= gravimetra::system::fault_snapshot_field_mask(
        gravimetra::system::FaultSnapshotField::stability_diagnostics);
    cleared.active_auger = 2;
    cleared.active_stage = 3U;
    cleared.commanded_motor_speed_steps_per_s = 4'000U;
    cleared.commanded_steps = 0x1'0000'0005ULL;
    cleared.stability_standard_deviation_mg = 0.01;
    cleared.stability_slope_mg_per_s = -0.02;
    cleared.stability_peak_to_peak_mg = 0.03;
    cleared.stability_sample_count = 8U;
    cleared.stability_valid_sample_count = 7U;
    cleared.stability_window_duration_us = 500U;
    cleared.stability_continuously_passing_duration_us = 200U;
    assert_status(
        gravimetra::Status::ok, faults.clear_all_active(cleared));
    TEST_ASSERT_FALSE(faults.any_active());
    TEST_ASSERT_EQUAL_UINT32(4U, faults.history_size());
    const auto* first_clear = faults.history_at(2U);
    const auto* second_clear = faults.history_at(3U);
    TEST_ASSERT_NOT_NULL(first_clear);
    TEST_ASSERT_NOT_NULL(second_clear);
    TEST_ASSERT_FALSE(first_clear->became_active);
    TEST_ASSERT_FALSE(second_clear->became_active);
    TEST_ASSERT_EQUAL_UINT64(20U, first_clear->snapshot.monotonic_time_us);
    TEST_ASSERT_EQUAL_UINT64(20U, second_clear->snapshot.monotonic_time_us);
    TEST_ASSERT_EQUAL_UINT64(
        0x1'0000'0005ULL, second_clear->snapshot.commanded_steps);
    TEST_ASSERT_EQUAL_UINT64(
        200U,
        second_clear->snapshot.stability_continuously_passing_duration_us);
}

void test_telemetry_uses_validity_and_target_safe_fixed_formatting() {
    using gravimetra::system::TelemetryField;
    gravimetra::system::TelemetrySample sample{};
    sample.valid_fields =
        gravimetra::system::telemetry_field_mask(TelemetryField::monotonic_time) |
        gravimetra::system::telemetry_field_mask(TelemetryField::application_state) |
        gravimetra::system::telemetry_field_mask(TelemetryField::raw_ads1262_code) |
        gravimetra::system::telemetry_field_mask(TelemetryField::tared_mass) |
        gravimetra::system::telemetry_field_mask(TelemetryField::motion) |
        gravimetra::system::telemetry_field_mask(
            TelemetryField::stability_diagnostics) |
        gravimetra::system::telemetry_field_mask(TelemetryField::fault_flags);
    sample.monotonic_time_us = 99U;
    sample.raw_ads1262_code = -10;
    sample.shunt_voltage_v = 42.0;  // Deliberately invalid/unavailable.
    sample.tared_mass_mg = 123.5;
    sample.commanded_steps = 0x1'0000'0005ULL;
    sample.stability_standard_deviation_mg = 0.00125;
    sample.stability_slope_mg_per_s = -0.125;
    sample.stability_peak_to_peak_mg = 0.004;
    sample.stability_reason_flags = 4U;
    sample.stability_sample_count = 8U;
    sample.stability_valid_sample_count = 7U;
    sample.stability_window_duration_us = 500U;
    sample.stability_continuously_passing_duration_us = 200U;
    sample.fault_flags = 0x10U;

    std::array<char, 1536U> output{};
    std::size_t written = 0U;
    assert_status(
        gravimetra::Status::ok,
        gravimetra::system::TelemetryCsv::serialize(
            sample, output.data(), output.size(), written));
    TEST_ASSERT_NOT_NULL(std::strstr(output.data(), "99,0,-10,,,,,123.5,"));
    TEST_ASSERT_NOT_NULL(std::strstr(output.data(), "4294967301"));
    TEST_ASSERT_NOT_NULL(std::strstr(output.data(), "0.00125,-0.125,0.004,4,8,7"));
    TEST_ASSERT_EQUAL_CHAR('\n', output[written - 1U]);

    sample.valid_fields |= gravimetra::system::telemetry_field_mask(
        TelemetryField::shunt_voltage);
    sample.shunt_voltage_v = std::numeric_limits<double>::quiet_NaN();
    assert_status(
        gravimetra::Status::invalid_argument,
        gravimetra::system::TelemetryCsv::serialize(
            sample, output.data(), output.size(), written));
    TEST_ASSERT_EQUAL_CHAR('\0', output[0U]);

    std::array<char, 1024U> header{};
    assert_status(
        gravimetra::Status::ok,
        gravimetra::system::TelemetryCsv::serialize_header(
            header.data(), header.size(), written));
    TEST_ASSERT_NOT_NULL(std::strstr(header.data(), "stability_reason_flags"));
    TEST_ASSERT_NOT_NULL(
        std::strstr(header.data(), "magnet_yoke_temperature_c"));
    TEST_ASSERT_NOT_NULL(
        std::strstr(header.data(), "flexure_body_temperature_c"));
    TEST_ASSERT_NOT_NULL(
        std::strstr(header.data(), "precision_afe_temperature_c"));
    TEST_ASSERT_NULL(std::strstr(header.data(), "ambient_temperature_c"));
    TEST_ASSERT_NOT_NULL(std::strstr(header.data(), "validity_flags"));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_nextion_numeric_payload_may_contain_three_ff_bytes);
    RUN_TEST(test_ui_target_value_is_correlated_and_range_checked);
    RUN_TEST(test_service_parser_recovers_overlapping_preamble_after_crc_failure);
    RUN_TEST(test_service_parser_recovers_frame_swallowed_by_corrupted_length);
    RUN_TEST(test_service_timeout_preserves_verified_queue);
    RUN_TEST(test_rs485_requires_and_enforces_configured_tx_deadline);
    RUN_TEST(test_watchdog_requires_fresh_progress_after_reenable);
    RUN_TEST(test_clear_all_faults_records_each_transition);
    RUN_TEST(test_telemetry_uses_validity_and_target_safe_fixed_formatting);
    return UNITY_END();
}
