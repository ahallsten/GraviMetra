#include "gravimetra/calibration/check_mass_controller.hpp"
#include "gravimetra/drivers/tmc2209.hpp"
#include "gravimetra/motion/auger_manager.hpp"
#include "gravimetra/motion/dispense_planner.hpp"
#include "gravimetra/safety/estop_manager.hpp"

#include <unity.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

using gravimetra::Status;

class FakeClock final : public gravimetra::hal::MonotonicClock {
public:
    [[nodiscard]] std::uint64_t now_us() const noexcept override {
        return now;
    }

    std::uint64_t now{0U};
};

class FakeOutput final : public gravimetra::hal::DigitalOutput {
public:
    [[nodiscard]] Status write(const bool asserted) noexcept override {
        ++write_count;
        value = asserted;
        return asserted ? assert_result : deassert_result;
    }

    bool value{false};
    std::uint32_t write_count{0U};
    Status assert_result{Status::ok};
    Status deassert_result{Status::ok};
};

class FakeInput final : public gravimetra::hal::DigitalInput {
public:
    [[nodiscard]] Status read(bool& asserted) noexcept override {
        asserted = value;
        return result;
    }

    bool value{false};
    Status result{Status::ok};
};

class FakeTimer final : public gravimetra::hal::StepPulseTimer {
public:
    [[nodiscard]] Status start(
        const std::uint8_t channel,
        const std::uint32_t frequency_hz,
        const std::uint32_t pulse_count) noexcept override {
        if (channel >= active_channels.size() || frequency_hz == 0U ||
            pulse_count == 0U) {
            return Status::invalid_argument;
        }
        if (!gravimetra::is_ok(start_result)) {
            return start_result;
        }
        active_channels[channel] = true;
        frequencies[channel] = frequency_hz;
        pulses[channel] = pulse_count;
        return Status::ok;
    }

    void stop(const std::uint8_t channel) noexcept override {
        if (channel < active_channels.size()) {
            active_channels[channel] = false;
        }
    }

    [[nodiscard]] bool active(
        const std::uint8_t channel) const noexcept override {
        return channel < active_channels.size() && active_channels[channel];
    }

    void complete(const std::uint8_t channel) noexcept {
        stop(channel);
    }

    std::array<bool, 4U> active_channels{};
    std::array<std::uint32_t, 4U> frequencies{};
    std::array<std::uint32_t, 4U> pulses{};
    Status start_result{Status::ok};
};

class FakeMotorDriver final : public gravimetra::motion::MotorDriver {
public:
    [[nodiscard]] Status configure(
        const gravimetra::motion::MotorElectricalConfig& config) noexcept
        override {
        ++configure_count;
        configured = config;
        return configure_result;
    }

    [[nodiscard]] Status read_status(
        gravimetra::motion::MotorDriverStatus& status) noexcept override {
        status = reported_status;
        return status_result;
    }

    [[nodiscard]] Status set_enabled(const bool requested) noexcept override {
        ++enable_write_count;
        const Status result = requested ? enable_result : disable_result;
        if (!gravimetra::is_ok(result)) {
            enable_state_value = gravimetra::motion::MotorEnableState::unknown;
            return result;
        }
        enable_state_value = requested
                                 ? gravimetra::motion::MotorEnableState::enabled
                                 : gravimetra::motion::MotorEnableState::disabled;
        return Status::ok;
    }

    [[nodiscard]] bool enabled() const noexcept override {
        return enable_state_value !=
               gravimetra::motion::MotorEnableState::disabled;
    }

    [[nodiscard]] gravimetra::motion::MotorEnableState enable_state()
        const noexcept override {
        return enable_state_value;
    }

    gravimetra::motion::MotorElectricalConfig configured{};
    gravimetra::motion::MotorDriverStatus reported_status{
        true,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        0U,
        0U,
        0U,
    };
    Status configure_result{Status::ok};
    Status status_result{Status::ok};
    Status enable_result{Status::ok};
    Status disable_result{Status::ok};
    gravimetra::motion::MotorEnableState enable_state_value{
        gravimetra::motion::MotorEnableState::disabled};
    std::uint32_t configure_count{0U};
    std::uint32_t enable_write_count{0U};
};

class FakeSafetyTarget final : public gravimetra::safety::SafetyInhibitTarget {
public:
    [[nodiscard]] Status inhibit_for_safety() noexcept override {
        inhibited = true;
        ++inhibit_count;
        return inhibit_result;
    }

    [[nodiscard]] Status release_safety_inhibit() noexcept override {
        if (!gravimetra::is_ok(release_result)) {
            return release_result;
        }
        inhibited = false;
        ++release_count;
        return Status::ok;
    }

    bool inhibited{false};
    std::uint32_t inhibit_count{0U};
    std::uint32_t release_count{0U};
    Status inhibit_result{Status::ok};
    Status release_result{Status::ok};
};

class FakeCheckActuator final
    : public gravimetra::calibration::CheckMassActuator {
public:
    enum class Command : std::uint8_t { disabled, apply, remove };

    [[nodiscard]] Status apply() noexcept override {
        command = Command::apply;
        ++apply_count;
        return apply_result;
    }

    [[nodiscard]] Status remove() noexcept override {
        command = Command::remove;
        ++remove_count;
        return remove_result;
    }

    [[nodiscard]] Status disable() noexcept override {
        command = Command::disabled;
        ++disable_count;
        return disable_result;
    }

    Command command{Command::disabled};
    Status apply_result{Status::ok};
    Status remove_result{Status::ok};
    Status disable_result{Status::ok};
    std::uint32_t apply_count{0U};
    std::uint32_t remove_count{0U};
    std::uint32_t disable_count{0U};
};

class FakeSpanSink final : public gravimetra::calibration::SpanCorrectionSink {
public:
    [[nodiscard]] Status apply_check_span_factor(
        const double multiplicative_factor) noexcept override {
        ++apply_count;
        factor = multiplicative_factor;
        return result;
    }

    double factor{1.0};
    std::uint32_t apply_count{0U};
    Status result{Status::ok};
};

class FakeTmcUart final : public gravimetra::hal::Uart {
public:
    FakeTmcUart() noexcept {
        registers[static_cast<std::uint8_t>(
            gravimetra::drivers::Tmc2209Register::ioin)] = 0x21000000UL;
        registers[static_cast<std::uint8_t>(
            gravimetra::drivers::Tmc2209Register::chopconf)] = 0x10000053UL;
        registers[static_cast<std::uint8_t>(
            gravimetra::drivers::Tmc2209Register::gstat)] = 0x00000001UL;
    }

    [[nodiscard]] Status write(
        const std::uint8_t* const data,
        const std::size_t length) noexcept override {
        if (data == nullptr ||
            (length != gravimetra::drivers::Tmc2209::kReadRequestLength &&
             length != gravimetra::drivers::Tmc2209::kDatagramLength)) {
            return Status::invalid_argument;
        }
        if (gravimetra::drivers::Tmc2209::crc8(data, length - 1U) !=
            data[length - 1U]) {
            return Status::protocol_error;
        }

        if (length == gravimetra::drivers::Tmc2209::kDatagramLength) {
            const std::uint8_t address =
                static_cast<std::uint8_t>(data[2U] & 0x7FU);
            const std::uint32_t value =
                (static_cast<std::uint32_t>(data[3U]) << 24U) |
                (static_cast<std::uint32_t>(data[4U]) << 16U) |
                (static_cast<std::uint32_t>(data[5U]) << 8U) |
                static_cast<std::uint32_t>(data[6U]);
            if (address == static_cast<std::uint8_t>(
                               gravimetra::drivers::Tmc2209Register::gstat)) {
                registers[address] &= ~value;
            } else {
                registers[address] = value;
            }
            auto& counter = registers[static_cast<std::uint8_t>(
                gravimetra::drivers::Tmc2209Register::ifcnt)];
            counter = static_cast<std::uint8_t>(counter + 1U);
            return Status::ok;
        }

        const std::uint8_t address =
            static_cast<std::uint8_t>(data[2U] & 0x7FU);
        const std::uint32_t value = address == corrupt_read_address
                                        ? corrupt_read_value
                                        : registers[address];
        std::size_t response_offset = 0U;
        if (echo_requests) {
            for (std::size_t index = 0U; index < length; ++index) {
                reply[index] = data[index];
            }
            response_offset = length;
        }
        reply[response_offset + 0U] = gravimetra::drivers::Tmc2209::kSyncByte;
        reply[response_offset + 1U] = gravimetra::drivers::Tmc2209::kMasterAddress;
        reply[response_offset + 2U] = address;
        reply[response_offset + 3U] =
            static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
        reply[response_offset + 4U] =
            static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
        reply[response_offset + 5U] =
            static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
        reply[response_offset + 6U] = static_cast<std::uint8_t>(value & 0xFFU);
        reply[response_offset + 7U] = gravimetra::drivers::Tmc2209::crc8(
            reply.data() + response_offset,
            gravimetra::drivers::Tmc2209::kDatagramLength - 1U);
        reply_offset = 0U;
        reply_size = response_offset + gravimetra::drivers::Tmc2209::kDatagramLength;
        return Status::ok;
    }

    [[nodiscard]] std::size_t read(
        std::uint8_t* const data,
        const std::size_t capacity) noexcept override {
        if (data == nullptr || capacity == 0U || reply_offset >= reply_size) {
            return 0U;
        }
        const std::size_t count = std::min(
            std::min(capacity, maximum_read_chunk), reply_size - reply_offset);
        for (std::size_t index = 0U; index < count; ++index) {
            data[index] = reply[reply_offset + index];
        }
        reply_offset += count;
        return count;
    }

    std::array<std::uint32_t, 128U> registers{};
    std::array<std::uint8_t, 16U> reply{};
    std::size_t reply_offset{0U};
    std::size_t reply_size{0U};
    std::size_t maximum_read_chunk{16U};
    bool echo_requests{false};
    std::uint8_t corrupt_read_address{0xFFU};
    std::uint32_t corrupt_read_value{0U};
};

[[nodiscard]] gravimetra::motion::MotorElectricalConfig motor_config() {
    gravimetra::motion::MotorElectricalConfig config{};
    config.run_current_scale = 20U;
    config.hold_current_scale = 8U;
    config.hold_delay = 4U;
    config.microsteps = gravimetra::motion::MicrostepResolution::x16;
    config.chopper_mode = gravimetra::motion::ChopperMode::stealth_chop;
    config.interpolate_to_256 = true;
    return config;
}

[[nodiscard]] gravimetra::drivers::Tmc2209InitConfig tmc_init_config() {
    gravimetra::drivers::Tmc2209InitConfig config{};
    config.node_address = 0U;
    config.motor = motor_config();
    config.interface_options_configured = true;
    config.use_vref_pin_for_current_scale = true;
    config.use_internal_sense_resistors = false;
    config.multistep_filter = true;
    config.read_timeout_us = 100U;
    config.maximum_read_polls = 32U;
    return config;
}

[[nodiscard]] gravimetra::motion::DispenseStageConfig stage_config(
    const std::uint8_t motor_channel,
    const double transition_error_mg,
    const double predictive_margin_mg) {
    gravimetra::motion::DispenseStageConfig config{};
    config.enabled = true;
    config.motor_channel = motor_channel;
    config.direction = gravimetra::motion::MotorDirection::forward;
    config.motor = motor_config();
    config.start_speed_hz = 100U;
    config.maximum_speed_hz = 500U;
    config.acceleration_hz_per_second = 1'000.0;
    config.minimum_pulse_count = 10U;
    config.transition_error_mg = transition_error_mg;
    config.predictive_margin_mg = predictive_margin_mg;
    config.timeout_us = 1'000U;
    config.maximum_overshoot_mg = 2.0;
    return config;
}

[[nodiscard]] gravimetra::motion::DispensePlannerConfig planner_config() {
    gravimetra::motion::DispensePlannerConfig config{};
    config.maximum_allowed_mass_mg =
        gravimetra::motion::kFrozenLiveCapacityMg;
    config.empty_tolerance_mg = 0.5;
    config.final_underfill_tolerance_mg = 0.5;
    config.final_overfill_tolerance_mg = 0.5;
    config.empty_verification_timeout_us = 1'000U;
    config.settling_timeout_us = 1'000U;
    config.validation_timeout_us = 1'000U;
    return config;
}

[[nodiscard]] gravimetra::motion::DispenseSample planner_sample(
    const gravimetra::motion::DispensePlanner& planner,
    const bool valid,
    const bool stable,
    const double responsive_mass_mg,
    const double settled_mass_mg,
    const bool profile_settled = true) {
    gravimetra::motion::DispenseSample sample{};
    sample.valid = valid;
    sample.stable = stable;
    sample.responsive_mass_mg = responsive_mass_mg;
    sample.settled_mass_mg = settled_mass_mg;
    sample.profile = planner.requested_profile();
    sample.profile_generation = planner.requested_profile_generation();
    sample.profile_settled = profile_settled;
    return sample;
}

[[nodiscard]] gravimetra::calibration::CheckMassConfig check_config(
    const bool allow_adjustment) {
    gravimetra::calibration::CheckMassConfig config{};
    config.certified_mass_mg = 100.0;
    config.check_tolerance_mg = 0.2;
    config.empty_tolerance_mg = 0.1;
    config.zero_return_tolerance_mg = 0.1;
    config.empty_stability_timeout_us = 100U;
    config.actuator_timeout_us = 100U;
    config.loaded_stability_timeout_us = 100U;
    config.zero_return_timeout_us = 100U;
    config.allow_automatic_span_adjustment = allow_adjustment;
    config.maximum_relative_span_adjustment = 0.01;
    return config;
}

[[nodiscard]] gravimetra::calibration::CheckMassSample check_sample(
    const gravimetra::calibration::CheckMassController& controller,
    const bool valid,
    const bool stable,
    const double mass_mg,
    const bool precision_profile_ready = true) {
    gravimetra::calibration::CheckMassSample sample{};
    sample.valid = valid;
    sample.stable = stable;
    sample.mass_mg = mass_mg;
    sample.precision_profile_epoch =
        controller.required_precision_profile_epoch();
    sample.precision_profile_ready = precision_profile_ready;
    return sample;
}

void test_tmc2209_crc_datagrams_and_verified_initialization() {
    using gravimetra::drivers::Tmc2209;
    using gravimetra::drivers::Tmc2209Register;

    const auto request = Tmc2209::make_read_request(0U, Tmc2209Register::gconf);
    TEST_ASSERT_EQUAL_HEX8(0x05U, request[0U]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, request[1U]);
    TEST_ASSERT_EQUAL_HEX8(0x00U, request[2U]);
    TEST_ASSERT_EQUAL_HEX8(0x48U, request[3U]);

    const auto write =
        Tmc2209::make_write_datagram(0U, Tmc2209Register::chopconf, 0x10000053UL);
    TEST_ASSERT_EQUAL_HEX8(0xECU, write[2U]);
    TEST_ASSERT_EQUAL_HEX8(0x9CU, write[7U]);

    FakeTmcUart uart;
    uart.echo_requests = true;
    uart.maximum_read_chunk = 1U;
    FakeOutput enable;
    FakeClock clock;
    Tmc2209 driver(uart, enable, clock);
    gravimetra::drivers::Tmc2209InitConfig config{};
    config.node_address = 0U;
    config.motor = motor_config();
    config.interface_options_configured = true;
    config.use_vref_pin_for_current_scale = true;
    config.use_internal_sense_resistors = false;
    config.multistep_filter = true;
    config.read_timeout_us = 100U;
    config.maximum_read_polls = 32U;

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(driver.initialize(config)));
    TEST_ASSERT_TRUE(driver.initialized());
    TEST_ASSERT_FALSE(driver.enabled());
    TEST_ASSERT_EQUAL_UINT8(
        4U,
        static_cast<std::uint8_t>(
            uart.registers[static_cast<std::uint8_t>(Tmc2209Register::ifcnt)]));
    TEST_ASSERT_EQUAL_HEX32(
        0x14000053UL,
        uart.registers[static_cast<std::uint8_t>(Tmc2209Register::chopconf)]);

    uart.registers[static_cast<std::uint8_t>(Tmc2209Register::gstat)] =
        1UL << 1U;
    uart.registers[static_cast<std::uint8_t>(Tmc2209Register::drv_status)] =
        1UL << 2U;
    gravimetra::motion::MotorDriverStatus status{};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(driver.read_status(status)));
    TEST_ASSERT_TRUE(status.driver_error_latched);
    TEST_ASSERT_TRUE(status.short_to_ground);
    TEST_ASSERT_TRUE(status.critical_fault());

    uart.registers[static_cast<std::uint8_t>(Tmc2209Register::gstat)] =
        1UL << 0U;
    uart.registers[static_cast<std::uint8_t>(Tmc2209Register::drv_status)] = 0U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::verification_failed),
        static_cast<int>(driver.set_enabled(true)));
    TEST_ASSERT_FALSE(driver.initialized());
    TEST_ASSERT_FALSE(driver.enabled());
}

void test_two_motor_attempt_stops_every_channel() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver0;
    FakeMotorDriver driver1;
    FakeOutput direction0;
    FakeOutput direction1;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver0, &direction0, 0U, 5U};
    channels[1U] = {&driver1, &direction1, 1U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.set_motion_permitted(true)));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(manager.start_pulses(
            0U, gravimetra::motion::MotorDirection::forward, 100U, 20U)));
    clock.now = 5U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.start_pulses(
            0U, gravimetra::motion::MotorDirection::forward, 100U, 20U)));
    TEST_ASSERT_TRUE(timer.active(0U));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::interlock_violation),
        static_cast<int>(manager.start_pulses(
            1U, gravimetra::motion::MotorDirection::forward, 100U, 20U)));
    TEST_ASSERT_FALSE(timer.active(0U));
    TEST_ASSERT_FALSE(timer.active(1U));
    TEST_ASSERT_FALSE(driver0.enabled());
    TEST_ASSERT_FALSE(driver1.enabled());
}

void test_staged_planner_skips_disabled_stage_and_validates() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver0;
    FakeMotorDriver driver2;
    FakeOutput direction0;
    FakeOutput direction2;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver0, &direction0, 0U, 5U};
    channels[2U] = {&driver2, &direction2, 2U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.set_motion_permitted(true)));
    gravimetra::motion::DispensePlanner planner(manager, clock);
    auto config = planner_config();
    config.stages[0U] = stage_config(0U, 20.0, 5.0);
    config.stages[2U] = stage_config(2U, 3.0, 1.0);

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.configure(config)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.start(100.0)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 0.0, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::dispense_stage_1),
        static_cast<int>(planner.state()));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 0.0, 0.0))));
    clock.now += 5U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 0.0, 0.0))));
    TEST_ASSERT_TRUE(timer.active(0U));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 76.0, 76.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::settle),
        static_cast<int>(planner.state()));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 76.0, 76.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::dispense_stage_3),
        static_cast<int>(planner.state()));
    TEST_ASSERT_EQUAL_UINT8(2U, planner.active_stage());

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 76.0, 76.0))));
    clock.now += 5U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 76.0, 76.0))));
    TEST_ASSERT_TRUE(timer.active(2U));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 96.0, 96.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::settle),
        static_cast<int>(planner.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 100.0, 100.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::validate),
        static_cast<int>(planner.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 100.0, 100.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::complete),
        static_cast<int>(planner.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(
            gravimetra::motion::MeasurementProfileRequest::precision_settle),
        static_cast<int>(planner.requested_profile()));
}

void test_estop_during_motion_stops_and_latches() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeOutput direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver, &direction, 0U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    FakeInput estop_input;
    FakeSafetyTarget check_target;
    gravimetra::safety::EstopManager estop(
        estop_input, manager, check_target);

    TEST_ASSERT_TRUE(check_target.inhibited);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(estop.poll()));
    TEST_ASSERT_TRUE(manager.motion_permitted());
    TEST_ASSERT_FALSE(check_target.inhibited);

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(manager.start_pulses(
            0U, gravimetra::motion::MotorDirection::forward, 100U, 20U)));
    clock.now = 5U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.start_pulses(
            0U, gravimetra::motion::MotorDirection::forward, 100U, 20U)));
    TEST_ASSERT_TRUE(timer.active(0U));

    estop_input.value = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(estop.poll()));
    TEST_ASSERT_FALSE(timer.active(0U));
    TEST_ASSERT_FALSE(driver.enabled());
    TEST_ASSERT_FALSE(manager.motion_permitted());
    TEST_ASSERT_TRUE(check_target.inhibited);

    estop_input.value = false;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(estop.poll()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::safety::EstopState::released_waiting_reset),
        static_cast<int>(estop.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(estop.reset_latch()));
    TEST_ASSERT_TRUE(manager.motion_permitted());
}

void test_planner_detects_overshoot() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeOutput direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver, &direction, 0U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.set_motion_permitted(true)));
    gravimetra::motion::DispensePlanner planner(manager, clock);
    auto config = planner_config();
    config.stages[0U] = stage_config(0U, 5.0, 1.0);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.configure(config)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.start(100.0)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 0.0, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 103.0, 103.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::overfill),
        static_cast<int>(planner.state()));
    TEST_ASSERT_FALSE(manager.any_active());
}

void test_planner_reports_underfill_after_last_enabled_stage() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeOutput direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver, &direction, 0U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.set_motion_permitted(true)));
    gravimetra::motion::DispensePlanner planner(manager, clock);
    auto config = planner_config();
    config.stages[0U] = stage_config(0U, 5.0, 1.0);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.configure(config)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.start(100.0)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 0.0, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 94.0, 94.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::settle),
        static_cast<int>(planner.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 94.0, 94.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::validate),
        static_cast<int>(planner.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 94.0, 94.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::underfill),
        static_cast<int>(planner.state()));
}

void test_planner_detects_motion_timeout_and_tmc_fault() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeOutput direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver, &direction, 0U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.set_motion_permitted(true)));
    gravimetra::motion::DispensePlanner planner(manager, clock);
    auto config = planner_config();
    config.stages[0U] = stage_config(0U, 5.0, 1.0);
    config.stages[0U].timeout_us = 100U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.configure(config)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.start(100.0)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 0.0, 0.0))));
    clock.now = 101U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::timeout),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 0.0, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseFault::motion_timeout),
        static_cast<int>(planner.fault()));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(planner.abort()));
    clock.now = 0U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.start(100.0)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 0.0, 0.0))));
    driver.reported_status.driver_error_latched = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 0.0, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseFault::motor_driver_fault),
        static_cast<int>(planner.fault()));
}

void test_internal_check_success_can_apply_explicit_narrow_span_policy() {
    FakeClock clock;
    FakeCheckActuator actuator;
    FakeInput home;
    FakeInput applied;
    FakeSpanSink sink;
    home.value = true;
    gravimetra::calibration::CheckMassController controller(
        actuator, home, applied, clock, &sink);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.release_safety_inhibit()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.configure(check_config(true))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.start()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(FakeCheckActuator::Command::apply),
        static_cast<int>(actuator.command));

    home.value = false;
    applied.value = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, false, false, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(FakeCheckActuator::Command::disabled),
        static_cast<int>(actuator.command));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 99.9))));
    TEST_ASSERT_EQUAL_UINT32(0U, sink.apply_count);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::calibration::CheckMassOutcome::none),
        static_cast<int>(controller.result().outcome));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(FakeCheckActuator::Command::remove),
        static_cast<int>(actuator.command));

    home.value = true;
    applied.value = false;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, false, false, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 0.0))));
    TEST_ASSERT_EQUAL_UINT32(1U, sink.apply_count);
    TEST_ASSERT_TRUE(std::abs(sink.factor - (100.0 / 99.9)) <= 0.000001);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::calibration::CheckMassState::complete),
        static_cast<int>(controller.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::calibration::CheckMassOutcome::success),
        static_cast<int>(controller.result().outcome));
    TEST_ASSERT_TRUE(controller.result().span_adjustment_applied);
}

void test_failed_internal_check_never_adjusts_calibration() {
    FakeClock clock;
    FakeCheckActuator actuator;
    FakeInput home;
    FakeInput applied;
    FakeSpanSink sink;
    home.value = true;
    gravimetra::calibration::CheckMassController controller(
        actuator, home, applied, clock, &sink);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.release_safety_inhibit()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.configure(check_config(true))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.start()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 0.0))));
    home.value = false;
    applied.value = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, false, false, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 98.0))));
    TEST_ASSERT_EQUAL_UINT32(0U, sink.apply_count);

    home.value = true;
    applied.value = false;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, false, false, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::calibration::CheckMassState::failed),
        static_cast<int>(controller.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::calibration::CheckMassOutcome::check_failed),
        static_cast<int>(controller.result().outcome));
    TEST_ASSERT_FALSE(controller.result().span_adjustment_applied);
}

void test_tmc_rejects_unsafe_chopconf_and_tracks_unknown_enable_output() {
    using gravimetra::drivers::Tmc2209;
    using gravimetra::drivers::Tmc2209Register;
    using gravimetra::motion::MotorEnableState;

    FakeClock clock;
    FakeOutput unsafe_enable;
    FakeTmcUart unsafe_uart;
    unsafe_uart.registers[static_cast<std::uint8_t>(Tmc2209Register::chopconf)] |=
        1UL << 30U;
    Tmc2209 unsafe_driver(unsafe_uart, unsafe_enable, clock);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::not_configured),
        static_cast<int>(unsafe_driver.initialize(tmc_init_config())));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(MotorEnableState::disabled),
        static_cast<int>(unsafe_driver.enable_state()));

    FakeOutput corrupt_init_enable;
    FakeTmcUart corrupt_init_uart;
    corrupt_init_uart.corrupt_read_address = static_cast<std::uint8_t>(
        Tmc2209Register::ihold_irun);
    corrupt_init_uart.corrupt_read_value = 0U;
    Tmc2209 corrupt_init_driver(
        corrupt_init_uart, corrupt_init_enable, clock);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::verification_failed),
        static_cast<int>(corrupt_init_driver.initialize(tmc_init_config())));
    TEST_ASSERT_FALSE(corrupt_init_driver.initialized());
    TEST_ASSERT_FALSE(corrupt_init_driver.enabled());

    FakeOutput enable;
    FakeTmcUart uart;
    Tmc2209 driver(uart, enable, clock);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(driver.initialize(tmc_init_config())));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(driver.set_chopper_mode(
            static_cast<gravimetra::motion::ChopperMode>(0xFEU))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(driver.write_register(
            Tmc2209Register::chopconf, 0x50000053UL)));

    enable.assert_result = Status::io_error;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(driver.set_enabled(true)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(MotorEnableState::disabled),
        static_cast<int>(driver.enable_state()));

    enable.deassert_result = Status::io_error;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(driver.set_enabled(true)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(MotorEnableState::unknown),
        static_cast<int>(driver.enable_state()));
    TEST_ASSERT_TRUE(driver.enabled());

    enable.deassert_result = Status::ok;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(driver.set_enabled(false)));
    TEST_ASSERT_FALSE(driver.enabled());

    uart.corrupt_read_address =
        static_cast<std::uint8_t>(Tmc2209Register::ihold_irun);
    uart.corrupt_read_value = 0U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::verification_failed),
        static_cast<int>(driver.set_current(21U, 9U, 5U)));
    TEST_ASSERT_FALSE(driver.initialized());
    TEST_ASSERT_FALSE(driver.enabled());
}

void test_estop_latches_ambiguous_shutdown_until_verified_clear() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeMotorDriver second_driver;
    FakeOutput direction;
    FakeOutput second_direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver, &direction, 0U, 5U};
    channels[1U] = {&second_driver, &second_direction, 1U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    FakeInput estop_input;
    FakeSafetyTarget check_target;
    gravimetra::safety::EstopManager estop(
        estop_input, manager, check_target);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(estop.poll()));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(manager.start_pulses(
            0U, gravimetra::motion::MotorDirection::forward, 100U, 20U)));
    clock.now = 5U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.start_pulses(
            0U, gravimetra::motion::MotorDirection::forward, 100U, 20U)));

    driver.disable_result = Status::io_error;
    second_driver.disable_result = Status::timeout;
    estop_input.value = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(estop.poll()));
    TEST_ASSERT_FALSE(timer.active(0U));
    TEST_ASSERT_TRUE(manager.shutdown_fault_latched());
    TEST_ASSERT_EQUAL_HEX8(0x03U, manager.shutdown_failure_mask());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(manager.last_shutdown_status()));
    TEST_ASSERT_FALSE(manager.motion_permitted());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::MotorEnableState::unknown),
        static_cast<int>(driver.enable_state()));

    estop_input.value = false;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(estop.poll()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(estop.reset_latch()));
    TEST_ASSERT_TRUE(check_target.inhibited);

    driver.disable_result = Status::ok;
    second_driver.disable_result = Status::ok;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.clear_shutdown_fault()));
    TEST_ASSERT_EQUAL_HEX8(0x00U, manager.shutdown_failure_mask());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(estop.reset_latch()));
    TEST_ASSERT_TRUE(manager.motion_permitted());
    TEST_ASSERT_FALSE(check_target.inhibited);
}

void test_timer_completion_disable_failure_is_latched_with_channel_mask() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeOutput direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[2U] = {&driver, &direction, 2U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.set_motion_permitted(true)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(manager.start_pulses(
            2U, gravimetra::motion::MotorDirection::forward, 100U, 20U)));
    clock.now = 5U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.start_pulses(
            2U, gravimetra::motion::MotorDirection::forward, 100U, 20U)));

    timer.complete(2U);
    driver.disable_result = Status::io_error;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(manager.service()));
    TEST_ASSERT_TRUE(manager.shutdown_fault_latched());
    TEST_ASSERT_FALSE(manager.motion_permitted());
    TEST_ASSERT_EQUAL_HEX8(0x04U, manager.shutdown_failure_mask());
    TEST_ASSERT_EQUAL_UINT8(
        gravimetra::motion::AugerManager::kNoActiveAuger,
        manager.active_channel());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(manager.service()));

    driver.disable_result = Status::ok;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.clear_shutdown_fault()));
    TEST_ASSERT_EQUAL_HEX8(0x00U, manager.shutdown_failure_mask());
}

void test_estop_refuses_to_arm_after_check_actuator_inhibit_failure() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeOutput direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver, &direction, 0U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    FakeInput estop_input;
    FakeSafetyTarget check_target;
    gravimetra::safety::EstopManager estop(
        estop_input, manager, check_target);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(estop.poll()));
    TEST_ASSERT_TRUE(manager.motion_permitted());

    check_target.inhibit_result = Status::io_error;
    estop_input.value = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(estop.poll()));
    TEST_ASSERT_FALSE(manager.motion_permitted());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(estop.last_shutdown_status()));

    estop_input.value = false;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(estop.poll()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(estop.reset_latch()));
    TEST_ASSERT_FALSE(manager.motion_permitted());
    TEST_ASSERT_TRUE(check_target.inhibited);

    check_target.inhibit_result = Status::ok;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(estop.reset_latch()));
    TEST_ASSERT_TRUE(manager.motion_permitted());
    TEST_ASSERT_FALSE(check_target.inhibited);
}

void test_planner_requires_profile_ack_and_stops_on_driver_reset() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeOutput direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver, &direction, 0U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.set_motion_permitted(true)));
    gravimetra::motion::DispensePlanner planner(manager, clock);
    auto config = planner_config();
    config.stages[0U] = stage_config(0U, 5.0, 1.0);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(planner.configure(config)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(planner.start(100.0)));

    const auto stale_precision = planner_sample(planner, true, true, 0.0, 0.0);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(stale_precision)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::MeasurementProfileRequest::active_dispense),
        static_cast<int>(planner.requested_profile()));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(planner.update(stale_precision)));
    TEST_ASSERT_FALSE(driver.enabled());
    TEST_ASSERT_FALSE(timer.active(0U));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 0.0, 0.0, false))));
    TEST_ASSERT_FALSE(driver.enabled());

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 0.0, 0.0))));
    clock.now = 5U;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 0.0, 0.0))));
    TEST_ASSERT_TRUE(timer.active(0U));

    driver.reported_status.reset_detected = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(planner.update(
            planner_sample(planner, true, false, 0.0, 0.0))));
    TEST_ASSERT_FALSE(timer.active(0U));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseFault::motor_driver_fault),
        static_cast<int>(planner.fault()));
}

void test_planner_enforces_configured_and_frozen_capacity_for_both_samples() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeOutput direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver, &direction, 0U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(manager.set_motion_permitted(true)));
    gravimetra::motion::DispensePlanner planner(manager, clock);
    auto config = planner_config();
    config.maximum_allowed_mass_mg = 100.0;
    config.stages[0U] = stage_config(0U, 5.0, 1.0);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(planner.configure(config)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(planner.start(100.001)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(planner.start(100.0)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 0.0, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(planner.update(
            planner_sample(planner, false, false, 100.001, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseFault::capacity_exceeded),
        static_cast<int>(planner.fault()));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(planner.abort()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(planner.start(100.0)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(planner.update(
            planner_sample(planner, true, true, 0.0, 100.001))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseFault::capacity_exceeded),
        static_cast<int>(planner.fault()));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(planner.abort()));
    config.maximum_allowed_mass_mg =
        gravimetra::motion::kFrozenLiveCapacityMg + 0.001;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::not_configured),
        static_cast<int>(planner.configure(config)));
}

void test_planner_rejects_relationally_unsafe_limits_and_abort_failure() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeOutput direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver, &direction, 0U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    gravimetra::motion::DispensePlanner planner(manager, clock);
    auto config = planner_config();
    config.maximum_allowed_mass_mg = 100.0;
    config.stages[0U] = stage_config(0U, 5.0, 1.0);

    config.empty_tolerance_mg = 100.001;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::not_configured),
        static_cast<int>(planner.configure(config)));
    config.empty_tolerance_mg = 0.5;
    config.final_overfill_tolerance_mg = 100.001;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::not_configured),
        static_cast<int>(planner.configure(config)));
    config.final_overfill_tolerance_mg = 0.5;
    config.stages[0U].transition_error_mg = 1.0e308;
    config.stages[0U].predictive_margin_mg = 1.0e308;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::not_configured),
        static_cast<int>(planner.configure(config)));
    config.stages[0U] = stage_config(0U, 5.0, 1.0);
    config.stages[0U].maximum_overshoot_mg = 100.001;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::not_configured),
        static_cast<int>(planner.configure(config)));

    config.stages[0U] = stage_config(0U, 5.0, 1.0);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(planner.configure(config)));
    driver.disable_result = Status::io_error;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(planner.abort()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseState::fault),
        static_cast<int>(planner.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::motion::DispenseFault::motor_driver_fault),
        static_cast<int>(planner.fault()));
    TEST_ASSERT_TRUE(manager.shutdown_fault_latched());
}

void test_invalid_motion_enums_are_rejected() {
    FakeClock clock;
    FakeTimer timer;
    FakeMotorDriver driver;
    FakeOutput direction;
    std::array<gravimetra::motion::AugerChannelConfig, 4U> channels{};
    channels[0U] = {&driver, &direction, 0U, 5U};
    gravimetra::motion::AugerManager manager(timer, clock, channels);
    gravimetra::motion::DispensePlanner planner(manager, clock);
    auto config = planner_config();
    config.stages[0U] = stage_config(0U, 5.0, 1.0);
    config.stages[0U].direction =
        static_cast<gravimetra::motion::MotorDirection>(0xFEU);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::not_configured),
        static_cast<int>(planner.configure(config)));

    config.stages[0U].direction = gravimetra::motion::MotorDirection::forward;
    config.stages[0U].motor.chopper_mode =
        static_cast<gravimetra::motion::ChopperMode>(0xFEU);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::not_configured),
        static_cast<int>(planner.configure(config)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(manager.start_pulses(
            0U,
            static_cast<gravimetra::motion::MotorDirection>(0xFEU),
            100U,
            1U)));
}

void test_check_mass_rejects_nonpositive_span_inputs_and_unsafe_policy() {
    FakeClock clock;
    FakeCheckActuator actuator;
    FakeInput home;
    FakeInput applied;
    FakeSpanSink sink;
    home.value = true;
    gravimetra::calibration::CheckMassController controller(
        actuator, home, applied, clock, &sink);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.release_safety_inhibit()));

    auto invalid_policy = check_config(true);
    invalid_policy.maximum_relative_span_adjustment = 1.0;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::not_configured),
        static_cast<int>(controller.configure(invalid_policy)));
    invalid_policy = check_config(true);
    invalid_policy.check_tolerance_mg = invalid_policy.certified_mass_mg;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::not_configured),
        static_cast<int>(controller.configure(invalid_policy)));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.configure(check_config(true))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(controller.start()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 0.0))));
    home.value = false;
    applied.value = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, false, false, 0.0))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, -100.0))));
    TEST_ASSERT_EQUAL_UINT32(0U, sink.apply_count);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::calibration::CheckMassOutcome::check_failed),
        static_cast<int>(controller.result().outcome));
}

void test_check_mass_requires_fresh_precision_epochs_for_loaded_and_zero() {
    FakeClock clock;
    FakeCheckActuator actuator;
    FakeInput home;
    FakeInput applied;
    FakeSpanSink sink;
    home.value = true;
    gravimetra::calibration::CheckMassController controller(
        actuator, home, applied, clock, &sink);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.release_safety_inhibit()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.configure(check_config(true))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok), static_cast<int>(controller.start()));

    const std::uint64_t empty_epoch =
        controller.required_precision_profile_epoch();
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 0.0, false))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 0.0))));

    home.value = false;
    applied.value = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, false, false, 0.0))));
    const std::uint64_t loaded_epoch =
        controller.required_precision_profile_epoch();
    TEST_ASSERT_TRUE(loaded_epoch != empty_epoch);
    gravimetra::calibration::CheckMassSample stale_loaded{
        true, true, 99.9, empty_epoch, true};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(controller.update(stale_loaded)));
    TEST_ASSERT_EQUAL_UINT32(0U, actuator.remove_count);
    TEST_ASSERT_EQUAL_UINT32(0U, sink.apply_count);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 99.9))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::calibration::CheckMassOutcome::none),
        static_cast<int>(controller.result().outcome));

    home.value = true;
    applied.value = false;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, false, false, 0.0))));
    TEST_ASSERT_TRUE(
        controller.required_precision_profile_epoch() != loaded_epoch);
    gravimetra::calibration::CheckMassSample stale_zero{
        true, true, 0.0, loaded_epoch, true};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::busy),
        static_cast<int>(controller.update(stale_zero)));
    TEST_ASSERT_EQUAL_UINT32(0U, sink.apply_count);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::calibration::CheckMassOutcome::none),
        static_cast<int>(controller.result().outcome));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.update(
            check_sample(controller, true, true, 0.0))));
    TEST_ASSERT_EQUAL_UINT32(1U, sink.apply_count);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(gravimetra::calibration::CheckMassOutcome::success),
        static_cast<int>(controller.result().outcome));
}

void test_check_mass_latches_ambiguous_actuator_disable() {
    FakeClock clock;
    FakeCheckActuator actuator;
    FakeInput home;
    FakeInput applied;
    actuator.disable_result = Status::io_error;
    gravimetra::calibration::CheckMassController controller(
        actuator, home, applied, clock);
    TEST_ASSERT_TRUE(controller.actuator_disable_fault_latched());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::io_error),
        static_cast<int>(controller.last_actuator_disable_status()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(controller.release_safety_inhibit()));

    actuator.disable_result = Status::ok;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.clear_actuator_disable_fault()));
    TEST_ASSERT_FALSE(controller.actuator_disable_fault_latched());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(controller.release_safety_inhibit()));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_tmc2209_crc_datagrams_and_verified_initialization);
    RUN_TEST(test_two_motor_attempt_stops_every_channel);
    RUN_TEST(test_staged_planner_skips_disabled_stage_and_validates);
    RUN_TEST(test_estop_during_motion_stops_and_latches);
    RUN_TEST(test_planner_detects_overshoot);
    RUN_TEST(test_planner_reports_underfill_after_last_enabled_stage);
    RUN_TEST(test_planner_detects_motion_timeout_and_tmc_fault);
    RUN_TEST(test_internal_check_success_can_apply_explicit_narrow_span_policy);
    RUN_TEST(test_failed_internal_check_never_adjusts_calibration);
    RUN_TEST(test_tmc_rejects_unsafe_chopconf_and_tracks_unknown_enable_output);
    RUN_TEST(test_estop_latches_ambiguous_shutdown_until_verified_clear);
    RUN_TEST(test_timer_completion_disable_failure_is_latched_with_channel_mask);
    RUN_TEST(test_estop_refuses_to_arm_after_check_actuator_inhibit_failure);
    RUN_TEST(test_planner_requires_profile_ack_and_stops_on_driver_reset);
    RUN_TEST(test_planner_enforces_configured_and_frozen_capacity_for_both_samples);
    RUN_TEST(test_planner_rejects_relationally_unsafe_limits_and_abort_failure);
    RUN_TEST(test_invalid_motion_enums_are_rejected);
    RUN_TEST(test_check_mass_rejects_nonpositive_span_inputs_and_unsafe_policy);
    RUN_TEST(test_check_mass_requires_fresh_precision_epochs_for_loaded_and_zero);
    RUN_TEST(test_check_mass_latches_ambiguous_actuator_disable);
    return UNITY_END();
}
