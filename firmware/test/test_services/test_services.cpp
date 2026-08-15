#include "gravimetra/comms/service_framing.hpp"
#include "gravimetra/comms/transports.hpp"
#include "gravimetra/hmi/nextion.hpp"
#include "gravimetra/hmi/ui_model.hpp"
#include "gravimetra/power/power_manager.hpp"
#include "gravimetra/system/config_store.hpp"
#include "gravimetra/system/fault_manager.hpp"
#include "gravimetra/system/health_supervisor.hpp"
#include "gravimetra/system/telemetry.hpp"

#include <unity.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

class MockUart final : public gravimetra::hal::Uart {
public:
    [[nodiscard]] gravimetra::Status write(
        const std::uint8_t* const data,
        const std::size_t length) noexcept override {
        if (!gravimetra::is_ok(write_status)) {
            return write_status;
        }
        if (data == nullptr || tx_length + length > tx.size()) {
            return gravimetra::Status::io_error;
        }
        std::copy_n(data, length, tx.begin() + static_cast<std::ptrdiff_t>(tx_length));
        tx_length += length;
        return gravimetra::Status::ok;
    }

    [[nodiscard]] std::size_t read(
        std::uint8_t* const data,
        const std::size_t capacity) noexcept override {
        if (data == nullptr) {
            return 0U;
        }
        const std::size_t remaining = rx_length - rx_position;
        const std::size_t count = std::min(remaining, capacity);
        std::copy_n(
            rx.begin() + static_cast<std::ptrdiff_t>(rx_position),
            count,
            data);
        rx_position += count;
        return count;
    }

    void set_rx(const std::uint8_t* const data, const std::size_t length) noexcept {
        rx_length = std::min(length, rx.size());
        rx_position = 0U;
        std::copy_n(data, rx_length, rx.begin());
    }

    std::array<std::uint8_t, 256U> tx{};
    std::array<std::uint8_t, 256U> rx{};
    std::size_t tx_length{0U};
    std::size_t rx_length{0U};
    std::size_t rx_position{0U};
    gravimetra::Status write_status{gravimetra::Status::ok};
};

class MockOutput final : public gravimetra::hal::DigitalOutput {
public:
    [[nodiscard]] gravimetra::Status write(const bool asserted) noexcept override {
        ++write_count;
        const gravimetra::Status result =
            scripted_index < scripted_count
                ? scripted_statuses[scripted_index++]
                : status;
        if (gravimetra::is_ok(result)) {
            state = asserted;
        }
        return result;
    }

    bool state{false};
    std::uint32_t write_count{0U};
    gravimetra::Status status{gravimetra::Status::ok};
    std::array<gravimetra::Status, 4U> scripted_statuses{};
    std::size_t scripted_count{0U};
    std::size_t scripted_index{0U};
};

class MemoryStorage final : public gravimetra::hal::NonvolatileStorage {
public:
    MemoryStorage() noexcept { bytes.fill(0xFFU); }

    [[nodiscard]] std::size_t size() const noexcept override {
        return bytes.size();
    }

    [[nodiscard]] gravimetra::Status read(
        const std::size_t offset,
        std::uint8_t* const destination,
        const std::size_t length) noexcept override {
        if (destination == nullptr || offset > bytes.size() ||
            length > bytes.size() - offset) {
            return gravimetra::Status::io_error;
        }
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), length, destination);
        return gravimetra::Status::ok;
    }

    [[nodiscard]] gravimetra::Status write(
        const std::size_t offset,
        const std::uint8_t* const source,
        const std::size_t length) noexcept override {
        ++write_call_count;
        if (fail_write_call != 0U && write_call_count == fail_write_call) {
            return gravimetra::Status::io_error;
        }
        if (source == nullptr || offset > bytes.size() ||
            length > bytes.size() - offset ||
            (offset % required_program_alignment) != 0U ||
            (length % required_program_alignment) != 0U) {
            return gravimetra::Status::io_error;
        }
        std::copy_n(source, length, bytes.begin() + static_cast<std::ptrdiff_t>(offset));
        return gravimetra::Status::ok;
    }

    [[nodiscard]] gravimetra::Status erase(
        const std::size_t offset,
        const std::size_t length) noexcept override {
        if (!gravimetra::is_ok(erase_status)) {
            return erase_status;
        }
        if (offset > bytes.size() || length > bytes.size() - offset ||
            (offset % required_erase_alignment) != 0U ||
            (length % required_erase_alignment) != 0U) {
            return gravimetra::Status::io_error;
        }
        std::fill_n(
            bytes.begin() + static_cast<std::ptrdiff_t>(offset), length, 0xFFU);
        return gravimetra::Status::ok;
    }

    std::array<std::uint8_t, 512U> bytes{};
    std::size_t required_program_alignment{8U};
    std::size_t required_erase_alignment{128U};
    std::size_t write_call_count{0U};
    std::size_t fail_write_call{0U};
    gravimetra::Status erase_status{gravimetra::Status::ok};
};

class MockWatchdog final : public gravimetra::hal::Watchdog {
public:
    void refresh() noexcept override { ++refreshes; }
    std::uint32_t refreshes{0U};
};

class MockClock final : public gravimetra::hal::MonotonicClock {
public:
    [[nodiscard]] std::uint64_t now_us() const noexcept override { return now; }
    std::uint64_t now{0U};
};

void assert_status(
    const gravimetra::Status expected,
    const gravimetra::Status actual) {
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(expected), static_cast<int>(actual));
}

void test_nextion_parser_is_incremental_and_ui_only_queues_requests() {
    constexpr std::array<std::uint8_t, 7U> touch{
        0x65U, 0x02U, 0x07U, 0x00U, 0xFFU, 0xFFU, 0xFFU};
    MockUart uart;
    uart.set_rx(touch.data(), touch.size());
    gravimetra::hmi::NextionParser parser;

    std::size_t read = 0U;
    assert_status(gravimetra::Status::ok, parser.poll(uart, 4U, read));
    TEST_ASSERT_EQUAL_UINT32(4U, read);
    TEST_ASSERT_EQUAL_UINT32(0U, parser.pending_events());
    assert_status(gravimetra::Status::ok, parser.poll(uart, 3U, read));
    TEST_ASSERT_EQUAL_UINT32(3U, read);

    gravimetra::hmi::NextionEvent event{};
    TEST_ASSERT_TRUE(parser.pop(event));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::hmi::NextionEventType::touch),
        static_cast<int>(event.type));
    TEST_ASSERT_EQUAL_UINT8(2U, event.page_id);
    TEST_ASSERT_EQUAL_UINT8(7U, event.component_id);
    TEST_ASSERT_FALSE(event.pressed);

    gravimetra::hmi::UiRequestModel model;
    assert_status(
        gravimetra::Status::ok,
        model.bind(gravimetra::hmi::UiBinding{
            2U,
            7U,
            gravimetra::hmi::UiTouchEdge::release,
            gravimetra::hmi::UiRequestType::tare}));
    assert_status(gravimetra::Status::ok, model.accept(event));
    gravimetra::hmi::UiRequest request{};
    TEST_ASSERT_TRUE(model.pop(request));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::hmi::UiRequestType::tare),
        static_cast<int>(request.type));
}

void test_nextion_command_and_event_queues_are_bounded() {
    gravimetra::hmi::NextionCommandQueue commands;
    for (std::size_t index = 0U;
         index < gravimetra::hmi::kNextionCommandQueueCapacity;
         ++index) {
        assert_status(gravimetra::Status::ok, commands.enqueue("sendme", 6U));
    }
    assert_status(
        gravimetra::Status::queue_full, commands.enqueue("sendme", 6U));

    MockUart uart;
    assert_status(gravimetra::Status::ok, commands.send_next(uart));
    TEST_ASSERT_EQUAL_UINT32(9U, uart.tx_length);
    TEST_ASSERT_EQUAL_HEX8(0xFFU, uart.tx[6U]);
    TEST_ASSERT_EQUAL_HEX8(0xFFU, uart.tx[8U]);

    gravimetra::hmi::NextionParser parser;
    for (std::size_t index = 0U;
         index < gravimetra::hmi::kNextionEventQueueCapacity;
         ++index) {
        TEST_ASSERT_TRUE(gravimetra::is_ok(parser.ingest(0x01U)));
        TEST_ASSERT_TRUE(gravimetra::is_ok(parser.ingest(0xFFU)));
        TEST_ASSERT_TRUE(gravimetra::is_ok(parser.ingest(0xFFU)));
        TEST_ASSERT_TRUE(gravimetra::is_ok(parser.ingest(0xFFU)));
    }
    TEST_ASSERT_TRUE(gravimetra::is_ok(parser.ingest(0x01U)));
    TEST_ASSERT_TRUE(gravimetra::is_ok(parser.ingest(0xFFU)));
    TEST_ASSERT_TRUE(gravimetra::is_ok(parser.ingest(0xFFU)));
    assert_status(gravimetra::Status::queue_full, parser.ingest(0xFFU));
    TEST_ASSERT_EQUAL_UINT32(1U, parser.dropped_events());
}

void test_service_frame_round_trip_and_crc_rejection() {
    const std::array<std::uint8_t, 9U> check{
        '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    TEST_ASSERT_EQUAL_HEX16(
        0x29B1U,
        gravimetra::comms::crc16_ccitt(check.data(), check.size()));

    gravimetra::comms::ServiceMessage sent{};
    sent.message_class = gravimetra::comms::ServiceMessageClass::telemetry;
    sent.sequence = 0x1234U;
    sent.payload[0U] = 0x10U;
    sent.payload[1U] = 0x20U;
    sent.payload[2U] = 0x30U;
    sent.payload_length = 3U;
    std::array<std::uint8_t, gravimetra::comms::kServiceEncodedCapacity> frame{};
    std::size_t written = 0U;
    assert_status(
        gravimetra::Status::ok,
        gravimetra::comms::encode_service_message(
            sent, frame.data(), frame.size(), written));

    gravimetra::comms::ServiceFrameParser parser;
    for (std::size_t index = 0U; index < written; ++index) {
        assert_status(gravimetra::Status::ok, parser.ingest(frame[index]));
    }
    gravimetra::comms::ServiceMessage received{};
    TEST_ASSERT_TRUE(parser.pop(received));
    TEST_ASSERT_EQUAL_HEX16(sent.sequence, received.sequence);
    TEST_ASSERT_EQUAL_UINT32(3U, received.payload_length);
    TEST_ASSERT_EQUAL_HEX8(0x20U, received.payload[1U]);

    frame[8U] ^= 0x01U;
    gravimetra::Status final_status = gravimetra::Status::ok;
    for (std::size_t index = 0U; index < written; ++index) {
        final_status = parser.ingest(frame[index]);
    }
    assert_status(gravimetra::Status::verification_failed, final_status);
    TEST_ASSERT_EQUAL_UINT32(1U, parser.crc_failures());
}

void test_rs485_keeps_driver_enabled_until_tx_complete() {
    MockClock clock;
    MockUart uart;
    MockOutput driver_enable;
    gravimetra::comms::Rs485HalfDuplexTransport transport(
        uart, driver_enable);
    assert_status(
        gravimetra::Status::ok, transport.initialize_receive_mode());
    assert_status(
        gravimetra::Status::ok,
        transport.configure_transmit_timeout(clock, 1000U));
    TEST_ASSERT_FALSE(driver_enable.state);

    const std::array<std::uint8_t, 2U> payload{0xAAU, 0x55U};
    assert_status(
        gravimetra::Status::ok,
        transport.send(payload.data(), payload.size()));
    TEST_ASSERT_TRUE(driver_enable.state);
    TEST_ASSERT_TRUE(transport.transmitting());
    std::array<std::uint8_t, 1U> receive{};
    TEST_ASSERT_EQUAL_UINT32(0U, transport.receive(receive.data(), receive.size()));

    assert_status(
        gravimetra::Status::ok, transport.on_transmit_complete());
    TEST_ASSERT_FALSE(driver_enable.state);
    TEST_ASSERT_FALSE(transport.transmitting());
}

void test_rs485_direction_failure_stays_fail_closed() {
    MockClock clock;
    MockUart uart;
    MockOutput direction;
    gravimetra::comms::Rs485HalfDuplexTransport transport(uart, direction);
    const std::array<std::uint8_t, 2U> payload{1U, 2U};

    assert_status(
        gravimetra::Status::not_configured,
        transport.send(payload.data(), payload.size()));
    assert_status(
        gravimetra::Status::ok, transport.initialize_receive_mode());
    assert_status(
        gravimetra::Status::ok,
        transport.configure_transmit_timeout(clock, 1000U));

    uart.write_status = gravimetra::Status::io_error;
    direction.scripted_statuses[0U] = gravimetra::Status::ok;
    direction.scripted_statuses[1U] = gravimetra::Status::io_error;
    direction.scripted_count = 2U;
    direction.scripted_index = 0U;
    assert_status(
        gravimetra::Status::io_error,
        transport.send(payload.data(), payload.size()));
    TEST_ASSERT_TRUE(transport.transmitting());
    TEST_ASSERT_FALSE(transport.direction_state_known());
    assert_status(
        gravimetra::Status::not_configured,
        transport.send(payload.data(), payload.size()));

    direction.status = gravimetra::Status::ok;
    direction.scripted_count = 0U;
    assert_status(gravimetra::Status::ok, transport.abort_transmit());
    TEST_ASSERT_FALSE(transport.transmitting());
    TEST_ASSERT_TRUE(transport.direction_state_known());
}

void test_config_store_recovers_older_copy_after_newest_crc_failure() {
    MemoryStorage storage;
    gravimetra::system::AtomicConfigStore store(
        storage, gravimetra::system::ConfigStoreLayout{0U, 128U, 8U, 128U});
    const std::array<std::uint8_t, 3U> old_payload{1U, 2U, 3U};
    const std::array<std::uint8_t, 2U> new_payload{4U, 5U};
    assert_status(
        gravimetra::Status::ok,
        store.save(7U, old_payload.data(), old_payload.size()));
    assert_status(
        gravimetra::Status::ok,
        store.save(7U, new_payload.data(), new_payload.size()));

    constexpr std::size_t kSecondSlotPayload = 128U + 32U;
    storage.bytes[kSecondSlotPayload] ^= 0x80U;
    std::array<std::uint8_t, 16U> loaded{};
    std::size_t length = 0U;
    gravimetra::system::ConfigLoadInfo info{};
    assert_status(
        gravimetra::Status::ok,
        store.load(7U, loaded.data(), loaded.size(), length, &info));
    TEST_ASSERT_EQUAL_UINT32(old_payload.size(), length);
    TEST_ASSERT_EQUAL_UINT8(1U, loaded[0U]);
    TEST_ASSERT_EQUAL_UINT32(1U, info.generation);
    TEST_ASSERT_TRUE(info.recovered_from_redundant_copy);
}

void test_config_store_reports_crc_failure_without_valid_copy() {
    MemoryStorage storage;
    gravimetra::system::AtomicConfigStore store(
        storage, gravimetra::system::ConfigStoreLayout{0U, 128U, 8U, 128U});
    const std::array<std::uint8_t, 2U> payload{9U, 8U};
    assert_status(
        gravimetra::Status::ok,
        store.save(1U, payload.data(), payload.size()));
    storage.bytes[32U] ^= 0x01U;
    std::array<std::uint8_t, 8U> loaded{};
    std::size_t length = 0U;
    assert_status(
        gravimetra::Status::verification_failed,
        store.load(1U, loaded.data(), loaded.size(), length));
}

void test_config_store_requires_geometry_and_never_rolls_back_schema() {
    MemoryStorage storage;
    gravimetra::system::AtomicConfigStore missing_geometry(
        storage, gravimetra::system::ConfigStoreLayout{0U, 128U});
    const std::array<std::uint8_t, 2U> payload{1U, 2U};
    assert_status(
        gravimetra::Status::invalid_argument,
        missing_geometry.save(1U, payload.data(), payload.size()));

    gravimetra::system::AtomicConfigStore store(
        storage, gravimetra::system::ConfigStoreLayout{0U, 128U, 8U, 128U});
    assert_status(
        gravimetra::Status::ok,
        store.save(7U, payload.data(), payload.size()));
    assert_status(
        gravimetra::Status::ok,
        store.save(8U, payload.data(), payload.size()));
    std::array<std::uint8_t, 8U> loaded{};
    std::size_t length = 0U;
    assert_status(
        gravimetra::Status::not_configured,
        store.load(7U, loaded.data(), loaded.size(), length));

    // Only an integrity failure in the newer slot permits redundant recovery.
    storage.bytes[128U + 32U] ^= 0x01U;
    assert_status(
        gravimetra::Status::ok,
        store.load(7U, loaded.data(), loaded.size(), length));
}

void test_config_store_interrupted_update_preserves_previous_copy() {
    for (std::size_t failure_phase = 0U; failure_phase < 4U; ++failure_phase) {
        MemoryStorage storage;
        gravimetra::system::AtomicConfigStore store(
            storage,
            gravimetra::system::ConfigStoreLayout{0U, 128U, 8U, 128U});
        const std::array<std::uint8_t, 3U> old_payload{1U, 2U, 3U};
        const std::array<std::uint8_t, 3U> new_payload{4U, 5U, 6U};
        assert_status(
            gravimetra::Status::ok,
            store.save(1U, old_payload.data(), old_payload.size()));
        storage.write_call_count = 0U;
        if (failure_phase == 0U) {
            storage.erase_status = gravimetra::Status::io_error;
        } else {
            storage.fail_write_call = failure_phase;
        }
        assert_status(
            gravimetra::Status::io_error,
            store.save(1U, new_payload.data(), new_payload.size()));
        storage.erase_status = gravimetra::Status::ok;
        storage.fail_write_call = 0U;

        std::array<std::uint8_t, 8U> loaded{};
        std::size_t length = 0U;
        assert_status(
            gravimetra::Status::ok,
            store.load(1U, loaded.data(), loaded.size(), length));
        TEST_ASSERT_EQUAL_UINT32(old_payload.size(), length);
        TEST_ASSERT_EQUAL_UINT8(old_payload[0U], loaded[0U]);
    }
}

void test_power_transition_and_configured_charge_suspend() {
    gravimetra::power::PowerManager monitor_only(
        gravimetra::power::ChargerControlConfig{false}, nullptr);
    gravimetra::power::PowerInputs inputs{};
    inputs.valid = true;
    inputs.external_power_present = true;
    inputs.battery_present = true;
    inputs.charger_status_available = true;
    inputs.charger_charging = true;
    inputs.battery_voltage_v = 11.8;
    assert_status(
        gravimetra::Status::ok, monitor_only.update(inputs, false));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::power::PowerSource::external),
        static_cast<int>(monitor_only.state().source));
    assert_status(
        gravimetra::Status::ok, monitor_only.update(inputs, true));
    TEST_ASSERT_FALSE(monitor_only.state().charge_suspend_requested);

    inputs.valid = false;
    assert_status(
        gravimetra::Status::ok, monitor_only.update(inputs, false));
    inputs.valid = true;
    assert_status(
        gravimetra::Status::ok, monitor_only.update(inputs, false));
    TEST_ASSERT_EQUAL_UINT32(0U, monitor_only.state().source_transition_count);

    inputs.external_power_present = false;
    assert_status(
        gravimetra::Status::ok, monitor_only.update(inputs, false));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::power::PowerSource::battery),
        static_cast<int>(monitor_only.state().source));
    TEST_ASSERT_EQUAL_UINT32(1U, monitor_only.state().source_transition_count);

    MockOutput suspend;
    gravimetra::power::PowerManager controllable(
        gravimetra::power::ChargerControlConfig{true}, &suspend);
    inputs.external_power_present = true;
    assert_status(
        gravimetra::Status::ok, controllable.update(inputs, true));
    TEST_ASSERT_TRUE(suspend.state);
    TEST_ASSERT_TRUE(controllable.state().charge_suspend_requested);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::power::ChargerState::suspended_for_precision),
        static_cast<int>(controllable.state().charger));
    assert_status(
        gravimetra::Status::ok, controllable.update(inputs, false));
    TEST_ASSERT_FALSE(suspend.state);
}

void test_fault_history_captures_snapshot_once_per_transition() {
    gravimetra::system::FaultManager faults;
    gravimetra::system::FaultSnapshot snapshot{};
    snapshot.monotonic_time_us = 123456U;
    snapshot.raw_adc_code = -42;
    snapshot.tared_mass_mg = 16.25;
    snapshot.power_source = gravimetra::power::PowerSource::battery;
    snapshot.estop_active = true;
    assert_status(
        gravimetra::Status::ok,
        faults.raise(gravimetra::system::FaultCode::estop_active, snapshot));
    assert_status(
        gravimetra::Status::ok,
        faults.raise(gravimetra::system::FaultCode::estop_active, snapshot));
    TEST_ASSERT_EQUAL_UINT32(1U, faults.history_size());
    TEST_ASSERT_TRUE(faults.active(gravimetra::system::FaultCode::estop_active));
    const gravimetra::system::FaultRecord* record = faults.history_at(0U);
    TEST_ASSERT_NOT_NULL(record);
    TEST_ASSERT_TRUE(record->became_active);
    TEST_ASSERT_EQUAL_UINT64(123456U, record->snapshot.monotonic_time_us);
    TEST_ASSERT_EQUAL_INT32(-42, record->snapshot.raw_adc_code);
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 16.25, record->snapshot.tared_mass_mg);

    snapshot.monotonic_time_us = 123999U;
    assert_status(
        gravimetra::Status::ok,
        faults.clear(gravimetra::system::FaultCode::estop_active, snapshot));
    TEST_ASSERT_EQUAL_UINT32(2U, faults.history_size());
    TEST_ASSERT_FALSE(faults.active(gravimetra::system::FaultCode::estop_active));
}

void test_watchdog_requires_progress_from_every_required_subsystem() {
    MockWatchdog watchdog;
    const gravimetra::system::SubsystemMask required =
        gravimetra::system::subsystem_mask(
            gravimetra::system::Subsystem::measurement) |
        gravimetra::system::subsystem_mask(
            gravimetra::system::Subsystem::safety);
    gravimetra::system::HealthSupervisor health(watchdog, required);
    assert_status(gravimetra::Status::busy, health.try_refresh());
    assert_status(
        gravimetra::Status::ok,
        health.note_progress(gravimetra::system::Subsystem::measurement));
    assert_status(gravimetra::Status::busy, health.try_refresh());
    assert_status(
        gravimetra::Status::ok,
        health.note_progress(gravimetra::system::Subsystem::safety));
    assert_status(gravimetra::Status::ok, health.try_refresh());
    TEST_ASSERT_EQUAL_UINT32(1U, watchdog.refreshes);
    assert_status(gravimetra::Status::busy, health.try_refresh());

    assert_status(
        gravimetra::Status::ok,
        health.note_progress(gravimetra::system::Subsystem::measurement));
    assert_status(
        gravimetra::Status::ok,
        health.note_progress(gravimetra::system::Subsystem::safety));
    health.set_refresh_permitted(false);
    assert_status(gravimetra::Status::fault_active, health.try_refresh());
    TEST_ASSERT_EQUAL_UINT32(1U, watchdog.refreshes);
}

void test_telemetry_csv_is_fixed_buffer_and_structured() {
    gravimetra::system::TelemetrySample sample{};
    sample.valid_fields =
        gravimetra::system::telemetry_field_mask(
            gravimetra::system::TelemetryField::monotonic_time) |
        gravimetra::system::telemetry_field_mask(
            gravimetra::system::TelemetryField::application_state) |
        gravimetra::system::telemetry_field_mask(
            gravimetra::system::TelemetryField::raw_ads1262_code) |
        gravimetra::system::telemetry_field_mask(
            gravimetra::system::TelemetryField::tared_mass) |
        gravimetra::system::telemetry_field_mask(
            gravimetra::system::TelemetryField::fault_flags);
    sample.monotonic_time_us = 99U;
    sample.raw_ads1262_code = -10;
    sample.tared_mass_mg = 123.5;
    sample.fault_flags = 0x10U;
    std::array<char, 768U> csv{};
    std::size_t written = 0U;
    assert_status(
        gravimetra::Status::ok,
        gravimetra::system::TelemetryCsv::serialize(
            sample, csv.data(), csv.size(), written));
    TEST_ASSERT_GREATER_THAN_UINT32(0U, written);
    TEST_ASSERT_NOT_NULL(std::strstr(csv.data(), "99,0,-10,"));
    TEST_ASSERT_EQUAL_CHAR('\n', csv[written - 1U]);

    std::array<char, 8U> too_small{};
    assert_status(
        gravimetra::Status::queue_full,
        gravimetra::system::TelemetryCsv::serialize(
            sample, too_small.data(), too_small.size(), written));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_nextion_parser_is_incremental_and_ui_only_queues_requests);
    RUN_TEST(test_nextion_command_and_event_queues_are_bounded);
    RUN_TEST(test_service_frame_round_trip_and_crc_rejection);
    RUN_TEST(test_rs485_keeps_driver_enabled_until_tx_complete);
    RUN_TEST(test_rs485_direction_failure_stays_fail_closed);
    RUN_TEST(test_config_store_recovers_older_copy_after_newest_crc_failure);
    RUN_TEST(test_config_store_reports_crc_failure_without_valid_copy);
    RUN_TEST(test_config_store_requires_geometry_and_never_rolls_back_schema);
    RUN_TEST(test_config_store_interrupted_update_preserves_previous_copy);
    RUN_TEST(test_power_transition_and_configured_charge_suspend);
    RUN_TEST(test_fault_history_captures_snapshot_once_per_transition);
    RUN_TEST(test_watchdog_requires_progress_from_every_required_subsystem);
    RUN_TEST(test_telemetry_csv_is_fixed_buffer_and_structured);
    return UNITY_END();
}
