#include "gravimetra/drivers/tmc2209.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gravimetra::drivers {
namespace {

constexpr std::uint32_t kGconfUseVref = 1UL << 0U;
constexpr std::uint32_t kGconfInternalSense = 1UL << 1U;
constexpr std::uint32_t kGconfSpreadCycle = 1UL << 2U;
constexpr std::uint32_t kGconfPdnDisable = 1UL << 6U;
constexpr std::uint32_t kGconfMstepRegisterSelect = 1UL << 7U;
constexpr std::uint32_t kGconfMultistepFilter = 1UL << 8U;
constexpr std::uint32_t kChopconfInterpolate = 1UL << 28U;
constexpr std::uint32_t kChopconfMresMask = 0x0FUL << 24U;
constexpr std::uint32_t kChopconfToffMask = 0x0FUL;
constexpr std::uint32_t kChopconfTblMask = 0x03UL << 15U;
constexpr std::uint32_t kChopconfDoubleEdge = 1UL << 29U;
constexpr std::uint32_t kChopconfDisableShortToGround = 1UL << 30U;
constexpr std::uint32_t kChopconfDisableLowSideShort = 1UL << 31U;
constexpr std::uint32_t kChopconfUnsafeMask =
    kChopconfDoubleEdge | kChopconfDisableShortToGround |
    kChopconfDisableLowSideShort;
constexpr std::uint32_t kChopconfReservedMustBeZeroMask =
    (0x3FUL << 18U) | (0x0FUL << 11U);

[[nodiscard]] constexpr bool safe_chopconf(
    const std::uint32_t value) noexcept {
    if ((value & (kChopconfUnsafeMask | kChopconfReservedMustBeZeroMask)) != 0U) {
        return false;
    }
    const std::uint32_t toff = value & kChopconfToffMask;
    const std::uint32_t tbl = (value & kChopconfTblMask) >> 15U;
    // TOFF=0 disables the bridges; TOFF=1 is valid only with TBL >= 2.
    // All other chopper timing remains released-hardware configuration.
    return toff != 0U && (toff != 1U || tbl >= 2U);
}

[[nodiscard]] constexpr std::uint8_t register_address(
    const Tmc2209Register reg) noexcept {
    return static_cast<std::uint8_t>(reg);
}

[[nodiscard]] constexpr std::uint8_t microstep_code(
    const motion::MicrostepResolution resolution) noexcept {
    switch (resolution) {
        case motion::MicrostepResolution::x256:
            return 0U;
        case motion::MicrostepResolution::x128:
            return 1U;
        case motion::MicrostepResolution::x64:
            return 2U;
        case motion::MicrostepResolution::x32:
            return 3U;
        case motion::MicrostepResolution::x16:
            return 4U;
        case motion::MicrostepResolution::x8:
            return 5U;
        case motion::MicrostepResolution::x4:
            return 6U;
        case motion::MicrostepResolution::x2:
            return 7U;
        case motion::MicrostepResolution::full_step:
            return 8U;
        case motion::MicrostepResolution::invalid:
            return 0xFFU;
    }
    return 0xFFU;
}

[[nodiscard]] constexpr std::uint64_t deadline_after(
    const std::uint64_t now,
    const std::uint32_t duration) noexcept {
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    if (duration > maximum - now) {
        return maximum;
    }
    return now + duration;
}

}  // namespace

Tmc2209::Tmc2209(
    hal::Uart& uart,
    hal::DigitalOutput& enable_output,
    const hal::MonotonicClock& clock) noexcept
    : uart_(uart), enable_output_(enable_output), clock_(clock) {}

std::uint8_t Tmc2209::crc8(
    const std::uint8_t* const data,
    const std::size_t length) noexcept {
    if (data == nullptr && length != 0U) {
        return 0U;
    }

    std::uint8_t crc = 0U;
    for (std::size_t index = 0U; index < length; ++index) {
        std::uint8_t current = data[index];
        for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
            const bool feedback =
                (((crc >> 7U) ^ (current & 0x01U)) & 0x01U) != 0U;
            crc = static_cast<std::uint8_t>(crc << 1U);
            if (feedback) {
                crc = static_cast<std::uint8_t>(crc ^ 0x07U);
            }
            current = static_cast<std::uint8_t>(current >> 1U);
        }
    }
    return crc;
}

std::array<std::uint8_t, Tmc2209::kReadRequestLength>
Tmc2209::make_read_request(
    const std::uint8_t node_address,
    const Tmc2209Register reg) noexcept {
    std::array<std::uint8_t, kReadRequestLength> datagram{
        kSyncByte,
        node_address,
        static_cast<std::uint8_t>(register_address(reg) & 0x7FU),
        0U,
    };
    datagram.back() = crc8(datagram.data(), datagram.size() - 1U);
    return datagram;
}

std::array<std::uint8_t, Tmc2209::kDatagramLength>
Tmc2209::make_write_datagram(
    const std::uint8_t node_address,
    const Tmc2209Register reg,
    const std::uint32_t value) noexcept {
    std::array<std::uint8_t, kDatagramLength> datagram{
        kSyncByte,
        node_address,
        static_cast<std::uint8_t>(register_address(reg) | 0x80U),
        static_cast<std::uint8_t>((value >> 24U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 16U) & 0xFFU),
        static_cast<std::uint8_t>((value >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(value & 0xFFU),
        0U,
    };
    datagram.back() = crc8(datagram.data(), datagram.size() - 1U);
    return datagram;
}

Status Tmc2209::initialize(const Tmc2209InitConfig& config) noexcept {
    initialized_ = false;
    const Status disable_status = force_disabled();
    if (!is_ok(disable_status)) {
        return disable_status;
    }
    if (!config.valid()) {
        return Status::not_configured;
    }

    node_address_ = config.node_address;
    read_timeout_us_ = config.read_timeout_us;
    maximum_read_polls_ = config.maximum_read_polls;

    std::uint32_t input_register = 0U;
    Status status = read_register(Tmc2209Register::ioin, input_register);
    if (!is_ok(status)) {
        return status;
    }
    const auto version =
        static_cast<std::uint8_t>((input_register >> 24U) & 0xFFU);
    if (version != kExpectedVersion) {
        return Status::verification_failed;
    }

    std::uint32_t initial_counter = 0U;
    status = read_register(Tmc2209Register::ifcnt, initial_counter);
    if (!is_ok(status)) {
        return status;
    }

    std::uint32_t initial_chopconf = 0U;
    status = read_register(Tmc2209Register::chopconf, initial_chopconf);
    if (!is_ok(status)) {
        return status;
    }
    if (!safe_chopconf(initial_chopconf)) {
        // Do not inherit disabled protection, double-edge stepping, nonzero
        // reserved fields, or structurally invalid chopper timing. Do not
        // invent replacement timing for an unreleased module configuration.
        return Status::not_configured;
    }

    std::uint32_t gconf = kGconfPdnDisable | kGconfMstepRegisterSelect;
    if (config.use_vref_pin_for_current_scale) {
        gconf |= kGconfUseVref;
    }
    if (config.use_internal_sense_resistors) {
        gconf |= kGconfInternalSense;
    }
    if (config.motor.chopper_mode == motion::ChopperMode::spread_cycle) {
        gconf |= kGconfSpreadCycle;
    }
    if (config.multistep_filter) {
        gconf |= kGconfMultistepFilter;
    }

    const std::uint32_t current =
        static_cast<std::uint32_t>(config.motor.hold_current_scale) |
        (static_cast<std::uint32_t>(config.motor.run_current_scale) << 8U) |
        (static_cast<std::uint32_t>(config.motor.hold_delay) << 16U);

    const std::uint8_t mres = microstep_code(config.motor.microsteps);
    if (mres == 0xFFU) {
        return Status::invalid_argument;
    }
    std::uint32_t chopconf =
        initial_chopconf & ~(kChopconfMresMask | kChopconfInterpolate);
    chopconf |= static_cast<std::uint32_t>(mres) << 24U;
    if (config.motor.interpolate_to_256) {
        chopconf |= kChopconfInterpolate;
    }

    status = write_register(Tmc2209Register::gconf, gconf);
    if (is_ok(status)) {
        status = write_register(Tmc2209Register::ihold_irun, current);
    }
    if (is_ok(status)) {
        status = write_register(Tmc2209Register::chopconf, chopconf);
    }
    if (!is_ok(status)) {
        return status;
    }

    std::uint32_t final_counter = 0U;
    status = read_register(Tmc2209Register::ifcnt, final_counter);
    if (!is_ok(status)) {
        return status;
    }
    const auto expected_counter = static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(initial_counter & 0xFFU) + 3U);
    if (static_cast<std::uint8_t>(final_counter & 0xFFU) !=
        expected_counter) {
        return Status::verification_failed;
    }

    gconf_shadow_ = gconf;
    chopconf_shadow_ = chopconf;
    current_shadow_ = current;
    initialized_ = true;

    status = verify_configuration();
    if (!is_ok(status)) {
        initialized_ = false;
        return status;
    }

    motion::MotorDriverStatus driver_status{};
    status = read_status(driver_status);
    if (!is_ok(status)) {
        initialized_ = false;
        return status;
    }
    if (driver_status.reset_detected) {
        // GSTAT is write-one-to-clear. Consume the expected power-on reset
        // indication only after every configuration write and readback passed.
        status = write_with_counter_verification(
            Tmc2209Register::gstat, 1UL << 0U);
        if (!is_ok(status)) {
            initialized_ = false;
            return status;
        }
        // Confirm the W1C bit actually cleared and re-evaluate every critical
        // status bit. A reset observed after this point is never benign.
        status = read_status(driver_status);
        if (!is_ok(status)) {
            initialized_ = false;
            return status;
        }
    }
    if (driver_status.critical_fault()) {
        initialized_ = false;
        return Status::fault_active;
    }
    return Status::ok;
}

Status Tmc2209::verify_configuration() noexcept {
    if (!initialized_) {
        return Status::not_configured;
    }

    std::uint32_t gconf = 0U;
    Status status = read_register(Tmc2209Register::gconf, gconf);
    if (!is_ok(status)) {
        return status;
    }
    std::uint32_t chopconf = 0U;
    status = read_register(Tmc2209Register::chopconf, chopconf);
    if (!is_ok(status)) {
        return status;
    }
    std::uint32_t current = 0U;
    status = read_register(Tmc2209Register::ihold_irun, current);
    if (!is_ok(status)) {
        return status;
    }
    if (gconf != gconf_shadow_ || chopconf != chopconf_shadow_ ||
        current != current_shadow_) {
        initialized_ = false;
        static_cast<void>(force_disabled());
        return Status::verification_failed;
    }
    return Status::ok;
}

void Tmc2209::discard_received_bytes() noexcept {
    std::array<std::uint8_t, 16U> discarded{};
    for (std::uint8_t attempt = 0U; attempt < 4U; ++attempt) {
        const std::size_t count = uart_.read(discarded.data(), discarded.size());
        if (count == 0U) {
            break;
        }
    }
}

Status Tmc2209::read_register(
    const Tmc2209Register reg,
    std::uint32_t& value) noexcept {
    if (node_address_ > 3U || read_timeout_us_ == 0U ||
        maximum_read_polls_ == 0U) {
        return Status::not_configured;
    }
    discard_received_bytes();
    const auto request = make_read_request(node_address_, reg);
    const Status write_status = uart_.write(request.data(), request.size());
    if (!is_ok(write_status)) {
        return write_status;
    }
    return read_reply(reg, value);
}

Status Tmc2209::read_reply(
    const Tmc2209Register requested_register,
    std::uint32_t& value) noexcept {
    std::array<std::uint8_t, kDatagramLength> candidate{};
    std::array<std::uint8_t, 16U> received{};
    std::size_t candidate_length = 0U;
    const std::uint64_t deadline =
        deadline_after(clock_.now_us(), read_timeout_us_);

    for (std::uint16_t poll = 0U; poll < maximum_read_polls_; ++poll) {
        const std::size_t count =
            std::min(uart_.read(received.data(), received.size()), received.size());
        for (std::size_t index = 0U; index < count; ++index) {
            const std::uint8_t byte = received[index];
            if (candidate_length == 0U) {
                if (byte == kSyncByte) {
                    candidate[0U] = byte;
                    candidate_length = 1U;
                }
                continue;
            }

            candidate[candidate_length] = byte;
            ++candidate_length;
            if (candidate_length == 2U && candidate[1U] != kMasterAddress) {
                candidate_length = byte == kSyncByte ? 1U : 0U;
                if (candidate_length == 1U) {
                    candidate[0U] = kSyncByte;
                }
                continue;
            }
            if (candidate_length == 3U &&
                candidate[2U] !=
                    (register_address(requested_register) & 0x7FU)) {
                candidate_length = byte == kSyncByte ? 1U : 0U;
                if (candidate_length == 1U) {
                    candidate[0U] = kSyncByte;
                }
                continue;
            }
            if (candidate_length != candidate.size()) {
                continue;
            }

            const std::uint8_t expected_crc =
                crc8(candidate.data(), candidate.size() - 1U);
            if (candidate.back() != expected_crc) {
                return Status::protocol_error;
            }
            value = (static_cast<std::uint32_t>(candidate[3U]) << 24U) |
                    (static_cast<std::uint32_t>(candidate[4U]) << 16U) |
                    (static_cast<std::uint32_t>(candidate[5U]) << 8U) |
                    static_cast<std::uint32_t>(candidate[6U]);
            return Status::ok;
        }

        if (clock_.now_us() >= deadline) {
            return Status::timeout;
        }
    }
    return Status::timeout;
}

Status Tmc2209::write_register(
    const Tmc2209Register reg,
    const std::uint32_t value) noexcept {
    if (node_address_ > 3U) {
        return Status::not_configured;
    }
    if (reg == Tmc2209Register::chopconf && !safe_chopconf(value)) {
        return Status::invalid_argument;
    }
    const auto datagram = make_write_datagram(node_address_, reg, value);
    return uart_.write(datagram.data(), datagram.size());
}

Status Tmc2209::write_with_counter_verification(
    const Tmc2209Register reg,
    const std::uint32_t value) noexcept {
    std::uint32_t before = 0U;
    Status status = read_register(Tmc2209Register::ifcnt, before);
    if (!is_ok(status)) {
        return status;
    }
    status = write_register(reg, value);
    if (!is_ok(status)) {
        return status;
    }
    std::uint32_t after = 0U;
    status = read_register(Tmc2209Register::ifcnt, after);
    if (!is_ok(status)) {
        return status;
    }
    const auto expected = static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(before & 0xFFU) + 1U);
    return static_cast<std::uint8_t>(after & 0xFFU) == expected
               ? Status::ok
               : Status::verification_failed;
}

Status Tmc2209::set_current(
    const std::uint8_t run_current_scale,
    const std::uint8_t hold_current_scale,
    const std::uint8_t hold_delay) noexcept {
    if (!initialized_) {
        return Status::not_configured;
    }
    if (enabled()) {
        return Status::busy;
    }
    if (run_current_scale > 31U || hold_current_scale > 31U ||
        hold_delay > 15U) {
        return Status::invalid_argument;
    }
    const std::uint32_t value =
        static_cast<std::uint32_t>(hold_current_scale) |
        (static_cast<std::uint32_t>(run_current_scale) << 8U) |
        (static_cast<std::uint32_t>(hold_delay) << 16U);
    Status status =
        write_with_counter_verification(Tmc2209Register::ihold_irun, value);
    if (!is_ok(status)) {
        return status;
    }
    std::uint32_t readback = 0U;
    status = read_register(Tmc2209Register::ihold_irun, readback);
    if (!is_ok(status)) {
        initialized_ = false;
        static_cast<void>(force_disabled());
        return status;
    }
    if (readback != value) {
        initialized_ = false;
        static_cast<void>(force_disabled());
        return Status::verification_failed;
    }
    current_shadow_ = value;
    return Status::ok;
}

Status Tmc2209::set_microsteps(
    const motion::MicrostepResolution resolution,
    const bool interpolate_to_256) noexcept {
    if (!initialized_) {
        return Status::not_configured;
    }
    if (enabled()) {
        return Status::busy;
    }
    const std::uint8_t code = microstep_code(resolution);
    if (code == 0xFFU) {
        return Status::invalid_argument;
    }
    std::uint32_t value =
        chopconf_shadow_ & ~(kChopconfMresMask | kChopconfInterpolate);
    value |= static_cast<std::uint32_t>(code) << 24U;
    if (interpolate_to_256) {
        value |= kChopconfInterpolate;
    }
    Status status =
        write_with_counter_verification(Tmc2209Register::chopconf, value);
    if (!is_ok(status)) {
        return status;
    }
    chopconf_shadow_ = value;
    std::uint32_t readback = 0U;
    status = read_register(Tmc2209Register::chopconf, readback);
    if (!is_ok(status)) {
        return status;
    }
    return readback == value ? Status::ok : Status::verification_failed;
}

Status Tmc2209::set_chopper_mode(const motion::ChopperMode mode) noexcept {
    if (!initialized_) {
        return Status::not_configured;
    }
    if (enabled()) {
        return Status::busy;
    }
    if (!motion::valid_chopper_mode(mode)) {
        return Status::invalid_argument;
    }
    std::uint32_t value = gconf_shadow_ & ~kGconfSpreadCycle;
    if (mode == motion::ChopperMode::spread_cycle) {
        value |= kGconfSpreadCycle;
    }
    Status status = write_with_counter_verification(Tmc2209Register::gconf, value);
    if (!is_ok(status)) {
        return status;
    }
    gconf_shadow_ = value;
    std::uint32_t readback = 0U;
    status = read_register(Tmc2209Register::gconf, readback);
    if (!is_ok(status)) {
        return status;
    }
    return readback == value ? Status::ok : Status::verification_failed;
}

Status Tmc2209::configure(
    const motion::MotorElectricalConfig& config) noexcept {
    if (!config.valid()) {
        return Status::not_configured;
    }
    Status status = set_current(
        config.run_current_scale,
        config.hold_current_scale,
        config.hold_delay);
    if (is_ok(status)) {
        status = set_microsteps(
            config.microsteps, config.interpolate_to_256);
    }
    if (is_ok(status)) {
        status = set_chopper_mode(config.chopper_mode);
    }
    if (!is_ok(status)) {
        static_cast<void>(set_enabled(false));
    }
    return status;
}

Status Tmc2209::read_status(motion::MotorDriverStatus& status) noexcept {
    status = {};
    if (!initialized_) {
        return Status::not_configured;
    }
    std::uint32_t global = 0U;
    Status result = read_register(Tmc2209Register::gstat, global);
    if (!is_ok(result)) {
        return result;
    }
    std::uint32_t driver = 0U;
    result = read_register(Tmc2209Register::drv_status, driver);
    if (!is_ok(result)) {
        return result;
    }

    status.communication_ok = true;
    status.global_status_raw = global;
    status.driver_status_raw = driver;
    status.reset_detected = (global & (1UL << 0U)) != 0U;
    status.driver_error_latched = (global & (1UL << 1U)) != 0U;
    status.charge_pump_undervoltage = (global & (1UL << 2U)) != 0U;
    status.overtemperature_warning = (driver & (1UL << 0U)) != 0U;
    status.overtemperature_shutdown = (driver & (1UL << 1U)) != 0U;
    status.short_to_ground = (driver & ((1UL << 2U) | (1UL << 3U))) != 0U;
    status.low_side_short = (driver & ((1UL << 4U) | (1UL << 5U))) != 0U;
    status.open_load_a = (driver & (1UL << 6U)) != 0U;
    status.open_load_b = (driver & (1UL << 7U)) != 0U;
    status.current_scale_actual =
        static_cast<std::uint8_t>((driver >> 16U) & 0x1FU);
    status.stealth_chop_active = (driver & (1UL << 30U)) != 0U;
    status.standstill = (driver & (1UL << 31U)) != 0U;
    return Status::ok;
}

Status Tmc2209::set_enabled(const bool enabled) noexcept {
    if (enabled && !initialized_) {
        return Status::not_configured;
    }
    if (enabled) {
        motion::MotorDriverStatus status{};
        const Status read_result = read_status(status);
        if (!is_ok(read_result)) {
            static_cast<void>(force_disabled());
            return read_result;
        }
        if (status.reset_detected) {
            static_cast<void>(force_disabled());
            initialized_ = false;
            return Status::verification_failed;
        }
        if (status.critical_fault()) {
            static_cast<void>(force_disabled());
            return Status::fault_active;
        }
    }
    if (!enabled) {
        return force_disabled();
    }
    // DigitalOutput uses logical assertion; the board/platform adapter owns
    // the unresolved physical ENN polarity.
    enable_state_ = motion::MotorEnableState::unknown;
    const Status result = enable_output_.write(true);
    if (is_ok(result)) {
        enable_state_ = motion::MotorEnableState::enabled;
        return Status::ok;
    }
    // A failed assertion has unknown physical effect. Immediately attempt a
    // deassertion; force_disabled preserves unknown if that attempt also fails.
    static_cast<void>(force_disabled());
    return result;
}

Status Tmc2209::force_disabled() noexcept {
    enable_state_ = motion::MotorEnableState::unknown;
    const Status result = enable_output_.write(false);
    if (is_ok(result)) {
        enable_state_ = motion::MotorEnableState::disabled;
    }
    return result;
}

}  // namespace gravimetra::drivers
