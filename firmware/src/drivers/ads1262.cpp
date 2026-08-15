#include "gravimetra/drivers/ads1262.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

namespace gravimetra::drivers {
namespace {

constexpr std::uint8_t kResetCommand = 0x06U;
constexpr std::uint8_t kStartCommand = 0x08U;
constexpr std::uint8_t kStopCommand = 0x0AU;
constexpr std::uint8_t kReadDataCommand = 0x12U;
constexpr std::uint8_t kReadRegisterCommand = 0x20U;
constexpr std::uint8_t kWriteRegisterCommand = 0x40U;

[[nodiscard]] constexpr std::uint8_t register_value(
    const Ads1262::Register value) noexcept {
    return static_cast<std::uint8_t>(value);
}

[[nodiscard]] constexpr bool fir_rate_valid(
    const Ads1262::DataRate rate) noexcept {
    return rate == Ads1262::DataRate::sps_2_5 ||
           rate == Ads1262::DataRate::sps_5 ||
           rate == Ads1262::DataRate::sps_10 ||
           rate == Ads1262::DataRate::sps_20;
}

[[nodiscard]] constexpr bool paired_internal_input(
    const Ads1262::Input input) noexcept {
    const std::uint8_t value = static_cast<std::uint8_t>(input);
    return value >=
               static_cast<std::uint8_t>(Ads1262::Input::temperature_monitor) &&
           value <= static_cast<std::uint8_t>(
                        Ads1262::Input::digital_supply_monitor);
}

[[nodiscard]] constexpr bool register_range_contains(
    const Ads1262::Register first,
    const std::size_t count,
    const Ads1262::Register target) noexcept {
    const std::size_t first_index = static_cast<std::uint8_t>(first);
    const std::size_t target_index = static_cast<std::uint8_t>(target);
    return target_index >= first_index && target_index < (first_index + count);
}

[[nodiscard]] constexpr bool register_write_affects_profile(
    const Ads1262::Register first,
    const std::size_t count) noexcept {
    return register_range_contains(first, count, Ads1262::Register::power) ||
           register_range_contains(first, count, Ads1262::Register::mode0) ||
           register_range_contains(first, count, Ads1262::Register::mode1) ||
           register_range_contains(first, count, Ads1262::Register::mode2) ||
           register_range_contains(first, count, Ads1262::Register::input_mux) ||
           register_range_contains(
               first, count, Ads1262::Register::offset_calibration_0) ||
           register_range_contains(
               first, count, Ads1262::Register::offset_calibration_1) ||
           register_range_contains(
               first, count, Ads1262::Register::offset_calibration_2) ||
           register_range_contains(
               first, count, Ads1262::Register::full_scale_calibration_0) ||
           register_range_contains(
               first, count, Ads1262::Register::full_scale_calibration_1) ||
           register_range_contains(
               first, count, Ads1262::Register::full_scale_calibration_2) ||
           register_range_contains(first, count, Ads1262::Register::reference_mux);
}

}  // namespace

Ads1262::Ads1262(
    hal::SpiBus& spi,
    hal::DigitalInput& data_ready,
    const hal::MonotonicClock& clock,
    const std::uint32_t spi_timeout_us) noexcept
    : spi_(spi),
      data_ready_(data_ready),
      clock_(clock),
      spi_timeout_us_(spi_timeout_us) {}

Status Ads1262::command(const std::uint8_t opcode) noexcept {
    const std::array<std::uint8_t, 1U> transmit{opcode};
    std::array<std::uint8_t, 1U> receive{};
    return spi_.transfer(
        transmit.data(), receive.data(), transmit.size(), spi_timeout_us_);
}

void Ads1262::invalidate_active_profile() noexcept {
    conversion_context_ = ConversionContext{};
    profile_settling_time_us_ = 0U;
    profile_change_time_us_ = clock_.now_us();
}

Status Ads1262::reset() noexcept {
    invalidate_active_profile();
    interface_configuration_known_ = false;
    const Status result = command(kResetCommand);
    if (is_ok(result)) {
        // Reset values from the ADS1262 data sheet: STATUS and checksum on.
        status_byte_enabled_ = true;
        integrity_ = DataIntegrity::checksum;
        interface_configuration_known_ = true;
    }
    return result;
}

Status Ads1262::start() noexcept {
    return command(kStartCommand);
}

Status Ads1262::verify_device() noexcept {
    std::uint8_t id = 0U;
    const Status result = read_register(Register::id, id);
    if (!is_ok(result)) {
        return result;
    }
    // DEV_ID[2:0] = 000 identifies ADS1262. REV_ID is intentionally ignored.
    return (id & 0xE0U) == 0U ? Status::ok : Status::verification_failed;
}

Status Ads1262::clear_reset_indicator() noexcept {
    std::uint8_t power = 0U;
    Status result = read_register(Register::power, power);
    if (!is_ok(result)) {
        return result;
    }
    power = static_cast<std::uint8_t>(power & 0x03U);
    result = write_register_verified(Register::power, power, 0x13U);
    return result;
}

Status Ads1262::stop() noexcept {
    return command(kStopCommand);
}

Status Ads1262::read_register(
    const Register address,
    std::uint8_t& value) noexcept {
    return read_registers(address, &value, 1U);
}

Status Ads1262::read_registers(
    const Register first,
    std::uint8_t* const destination,
    const std::size_t count) noexcept {
    const std::size_t first_index = register_value(first);
    if (destination == nullptr || count == 0U || count > kRegisterCount ||
        first_index >= kRegisterCount || count > (kRegisterCount - first_index)) {
        return Status::invalid_argument;
    }

    std::array<std::uint8_t, kMaximumTransactionLength> transmit{};
    std::array<std::uint8_t, kMaximumTransactionLength> receive{};
    transmit[0] = static_cast<std::uint8_t>(
        kReadRegisterCommand | register_value(first));
    transmit[1] = static_cast<std::uint8_t>(count - 1U);
    const std::size_t transaction_length = count + 2U;
    const Status result = spi_.transfer(
        transmit.data(), receive.data(), transaction_length, spi_timeout_us_);
    if (!is_ok(result)) {
        return result;
    }
    std::copy_n(receive.begin() + 2, count, destination);
    return Status::ok;
}

Status Ads1262::write_register(
    const Register address,
    const std::uint8_t value) noexcept {
    return write_registers(address, &value, 1U);
}

Status Ads1262::write_registers(
    const Register first,
    const std::uint8_t* const values,
    const std::size_t count) noexcept {
    const std::size_t first_index = register_value(first);
    if (values == nullptr || count == 0U || count > kRegisterCount ||
        first_index >= kRegisterCount || count > (kRegisterCount - first_index)) {
        return Status::invalid_argument;
    }

    if (register_range_contains(first, count, Register::interface)) {
        // A failed transaction can still have changed the ADC. Do not parse a
        // conversion frame until configure_interface() establishes readback.
        interface_configuration_known_ = false;
    }
    if (register_write_affects_profile(first, count)) {
        // Configuration writes are not atomic on the wire. Invalidate the
        // stamped conversion context before attempting any such write.
        invalidate_active_profile();
    }

    std::array<std::uint8_t, kMaximumTransactionLength> transmit{};
    std::array<std::uint8_t, kMaximumTransactionLength> receive{};
    transmit[0] = static_cast<std::uint8_t>(
        kWriteRegisterCommand | register_value(first));
    transmit[1] = static_cast<std::uint8_t>(count - 1U);
    std::copy_n(values, count, transmit.begin() + 2);
    return spi_.transfer(
        transmit.data(), receive.data(), count + 2U, spi_timeout_us_);
}

Status Ads1262::write_register_verified(
    const Register address,
    const std::uint8_t value,
    const std::uint8_t verification_mask) noexcept {
    Status result = write_register(address, value);
    if (!is_ok(result)) {
        return result;
    }
    std::uint8_t actual = 0U;
    result = read_register(address, actual);
    if (!is_ok(result)) {
        return result;
    }
    return ((actual & verification_mask) == (value & verification_mask))
               ? Status::ok
               : Status::verification_failed;
}

Status Ads1262::configure_interface(
    const bool interface_timeout_enabled,
    const bool status_byte_enabled,
    const DataIntegrity integrity) noexcept {
    if (static_cast<std::uint8_t>(integrity) >
        static_cast<std::uint8_t>(DataIntegrity::crc8_atm)) {
        return Status::invalid_argument;
    }
    const std::uint8_t value = static_cast<std::uint8_t>(
        (interface_timeout_enabled ? 0x08U : 0x00U) |
        (status_byte_enabled ? 0x04U : 0x00U) |
        static_cast<std::uint8_t>(integrity));
    Status result = write_register(Register::interface, value);
    if (!is_ok(result)) {
        return result;
    }
    std::uint8_t actual = 0U;
    result = read_register(Register::interface, actual);
    if (!is_ok(result)) {
        return result;
    }
    if ((actual & 0x0FU) != value) {
        return Status::verification_failed;
    }
    status_byte_enabled_ = status_byte_enabled;
    integrity_ = integrity;
    interface_configuration_known_ = true;
    return Status::ok;
}

Status Ads1262::select_channel(
    const Input positive,
    const Input negative,
    const std::uint32_t settling_time_us) noexcept {
    if (positive == Input::floating || negative == Input::floating ||
        (positive == negative && !paired_internal_input(positive))) {
        return Status::invalid_argument;
    }
    invalidate_active_profile();
    const std::uint8_t value = static_cast<std::uint8_t>(
        (static_cast<std::uint8_t>(positive) << 4U) |
        static_cast<std::uint8_t>(negative));
    const Status result = write_register_verified(Register::input_mux, value);
    if (is_ok(result)) {
        profile_change_time_us_ = clock_.now_us();
        profile_settling_time_us_ = settling_time_us;
    }
    return result;
}

Status Ads1262::validate_profile(const Profile& profile) noexcept {
    if (!profile.configured || profile.id == ProfileId::none) {
        return Status::not_configured;
    }
    if (static_cast<std::uint8_t>(profile.id) >
            static_cast<std::uint8_t>(ProfileId::precision_settle) ||
        static_cast<std::uint8_t>(profile.positive_input) >
            static_cast<std::uint8_t>(Input::floating) ||
        static_cast<std::uint8_t>(profile.negative_input) >
            static_cast<std::uint8_t>(Input::floating) ||
        profile.positive_input == Input::floating ||
        profile.negative_input == Input::floating ||
        (profile.positive_input == profile.negative_input &&
         !paired_internal_input(profile.positive_input)) ||
        profile.conversion_delay_code > 11U ||
        profile.settling_time_us == 0U ||
        (profile.bypass_pga && profile.gain != Gain::x1)) {
        return Status::invalid_argument;
    }
    if (static_cast<std::uint8_t>(profile.filter) >
            static_cast<std::uint8_t>(DigitalFilter::fir) ||
        static_cast<std::uint8_t>(profile.gain) >
            static_cast<std::uint8_t>(Gain::x32) ||
        static_cast<std::uint8_t>(profile.data_rate) >
            static_cast<std::uint8_t>(DataRate::sps_38400) ||
        static_cast<std::uint8_t>(profile.reference) >
            static_cast<std::uint8_t>(Reference::analog_supply)) {
        return Status::invalid_argument;
    }
    if (profile.filter == DigitalFilter::fir &&
        !fir_rate_valid(profile.data_rate)) {
        return Status::invalid_argument;
    }
    return Status::ok;
}

Status Ads1262::validate_profile_set(const ProfileSet& profiles) noexcept {
    Status result = validate_profile(profiles.fast_dispense);
    if (!is_ok(result)) {
        return result;
    }
    result = validate_profile(profiles.precision_settle);
    if (!is_ok(result)) {
        return result;
    }
    return profiles.fast_dispense.id == ProfileId::fast_dispense &&
                   profiles.precision_settle.id == ProfileId::precision_settle
               ? Status::ok
               : Status::invalid_argument;
}

Status Ads1262::apply_profile(
    const ProfileId profile,
    const ProfileSet& profiles) noexcept {
    const Status validation = validate_profile_set(profiles);
    if (!is_ok(validation)) {
        return validation;
    }
    if (profile == ProfileId::fast_dispense) {
        return apply_profile(profiles.fast_dispense);
    }
    if (profile == ProfileId::precision_settle) {
        return apply_profile(profiles.precision_settle);
    }
    return Status::invalid_argument;
}

Status Ads1262::apply_profile(const Profile& profile) noexcept {
    Status result = validate_profile(profile);
    if (!is_ok(result)) {
        return result;
    }

    // From this point onward, any failed SPI operation can leave the ADC
    // stopped or partially configured. Never retain the previous context.
    invalidate_active_profile();
    result = stop();
    if (!is_ok(result)) {
        return result;
    }

    const std::array<std::uint8_t, 4U> grouped_registers{
        static_cast<std::uint8_t>(
            (profile.pulse_conversion ? 0x40U : 0x00U) |
            profile.conversion_delay_code),
        static_cast<std::uint8_t>(
            static_cast<std::uint8_t>(profile.filter) << 5U),
        static_cast<std::uint8_t>(
            static_cast<std::uint32_t>(
                profile.bypass_pga ? 0x80U : 0x00U) |
            (static_cast<std::uint32_t>(
                 static_cast<std::uint8_t>(profile.gain)) << 4U) |
            static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(profile.data_rate))),
        static_cast<std::uint8_t>(
            (static_cast<std::uint8_t>(profile.positive_input) << 4U) |
            static_cast<std::uint8_t>(profile.negative_input)),
    };
    result = write_registers(
        Register::mode0, grouped_registers.data(), grouped_registers.size());
    if (!is_ok(result)) {
        return result;
    }

    const std::uint8_t reference = static_cast<std::uint8_t>(
        (static_cast<std::uint8_t>(profile.reference) << 3U) |
        static_cast<std::uint8_t>(profile.reference));
    result = write_register(Register::reference_mux, reference);
    if (!is_ok(result)) {
        return result;
    }
    result = verify_profile(profile);
    if (!is_ok(result)) {
        return result;
    }
    result = start();
    if (!is_ok(result)) {
        return result;
    }

    conversion_context_.verified = true;
    conversion_context_.profile = profile.id;
    conversion_context_.gain = profile.gain;
    conversion_context_.reference = profile.reference;
    conversion_context_.pga_bypassed = profile.bypass_pga;
    profile_change_time_us_ = clock_.now_us();
    profile_settling_time_us_ = profile.settling_time_us;
    return Status::ok;
}

Status Ads1262::verify_profile(const Profile& profile) noexcept {
    Status result = validate_profile(profile);
    if (!is_ok(result)) {
        return result;
    }
    std::array<std::uint8_t, 4U> actual{};
    result = read_registers(Register::mode0, actual.data(), actual.size());
    if (!is_ok(result)) {
        return result;
    }
    const std::array<std::uint8_t, 4U> expected{
        static_cast<std::uint8_t>(
            (profile.pulse_conversion ? 0x40U : 0x00U) |
            profile.conversion_delay_code),
        static_cast<std::uint8_t>(
            static_cast<std::uint8_t>(profile.filter) << 5U),
        static_cast<std::uint8_t>(
            static_cast<std::uint32_t>(
                profile.bypass_pga ? 0x80U : 0x00U) |
            (static_cast<std::uint32_t>(
                 static_cast<std::uint8_t>(profile.gain)) << 4U) |
            static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(profile.data_rate))),
        static_cast<std::uint8_t>(
            (static_cast<std::uint8_t>(profile.positive_input) << 4U) |
            static_cast<std::uint8_t>(profile.negative_input)),
    };
    if (actual != expected) {
        return Status::verification_failed;
    }

    std::uint8_t actual_reference = 0U;
    result = read_register(Register::reference_mux, actual_reference);
    if (!is_ok(result)) {
        return result;
    }
    const std::uint8_t expected_reference = static_cast<std::uint8_t>(
        (static_cast<std::uint8_t>(profile.reference) << 3U) |
        static_cast<std::uint8_t>(profile.reference));
    return actual_reference == expected_reference ? Status::ok
                                                   : Status::verification_failed;
}

bool Ads1262::profile_settled() const noexcept {
    if (profile_settling_time_us_ == 0U) {
        return true;
    }
    return (clock_.now_us() - profile_change_time_us_) >=
           profile_settling_time_us_;
}

std::uint32_t Ads1262::remaining_settling_time_us() const noexcept {
    const std::uint64_t elapsed = clock_.now_us() - profile_change_time_us_;
    if (elapsed >= profile_settling_time_us_) {
        return 0U;
    }
    return static_cast<std::uint32_t>(
        static_cast<std::uint64_t>(profile_settling_time_us_) - elapsed);
}

Status Ads1262::wait_profile_settled(const std::uint32_t timeout_us) noexcept {
    const std::uint64_t started = clock_.now_us();
    while (!profile_settled()) {
        if ((clock_.now_us() - started) >= timeout_us) {
            return Status::timeout;
        }
    }
    return Status::ok;
}

Status Ads1262::wait_data_ready(const std::uint32_t timeout_us) noexcept {
    const std::uint64_t started = clock_.now_us();
    for (;;) {
        bool ready = false;
        const Status result = data_ready_.read(ready);
        if (!is_ok(result)) {
            return result;
        }
        if (ready) {
            return Status::ok;
        }
        if ((clock_.now_us() - started) >= timeout_us) {
            return Status::timeout;
        }
    }
}

std::uint8_t Ads1262::checksum(
    const std::uint8_t* const data,
    const std::size_t length) noexcept {
    std::uint8_t result = 0x9BU;
    for (std::size_t index = 0U; index < length; ++index) {
        result = static_cast<std::uint8_t>(result + data[index]);
    }
    return result;
}

std::uint8_t Ads1262::crc8_atm(
    const std::uint8_t* const data,
    const std::size_t length) noexcept {
    std::uint8_t crc = 0U;
    for (std::size_t index = 0U; index < length; ++index) {
        crc = static_cast<std::uint8_t>(crc ^ data[index]);
        for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x80U) != 0U
                      ? static_cast<std::uint8_t>(
                            (static_cast<std::uint32_t>(crc) << 1U) ^
                            std::uint32_t{0x07U})
                      : static_cast<std::uint8_t>(
                            static_cast<std::uint32_t>(crc) << 1U);
        }
    }
    return crc;
}

Status Ads1262::read_conversion_frame(Conversion& conversion) noexcept {
    if (!interface_configuration_known_) {
        return Status::not_configured;
    }
    const std::size_t status_length = status_byte_enabled_ ? 1U : 0U;
    const std::size_t integrity_length =
        integrity_ == DataIntegrity::disabled ? 0U : 1U;
    const std::size_t response_length = status_length + 4U + integrity_length;
    std::array<std::uint8_t, 7U> transmit{};
    std::array<std::uint8_t, 7U> receive{};
    transmit[0] = kReadDataCommand;
    const Status transfer_result = spi_.transfer(
        transmit.data(), receive.data(), response_length + 1U, spi_timeout_us_);
    if (!is_ok(transfer_result)) {
        return transfer_result;
    }

    std::size_t data_index = 1U;
    if (status_byte_enabled_) {
        conversion.raw_status = receive[data_index];
        conversion.status = decode_status(conversion.raw_status);
        ++data_index;
    } else {
        conversion.raw_status = 0U;
        conversion.status = StatusByte{};
    }

    if (integrity_ != DataIntegrity::disabled) {
        const std::uint8_t expected =
            integrity_ == DataIntegrity::checksum
                ? checksum(receive.data() + data_index, 4U)
                : crc8_atm(receive.data() + data_index, 4U);
        if (receive[data_index + 4U] != expected) {
            return Status::protocol_error;
        }
    }

    const std::uint32_t raw =
        (static_cast<std::uint32_t>(receive[data_index]) << 24U) |
        (static_cast<std::uint32_t>(receive[data_index + 1U]) << 16U) |
        (static_cast<std::uint32_t>(receive[data_index + 2U]) << 8U) |
        static_cast<std::uint32_t>(receive[data_index + 3U]);
    if (raw <= static_cast<std::uint32_t>(
                   std::numeric_limits<std::int32_t>::max())) {
        conversion.code = static_cast<std::int32_t>(raw);
    } else {
        conversion.code = static_cast<std::int32_t>(
            -1 - static_cast<std::int32_t>(
                     std::numeric_limits<std::uint32_t>::max() - raw));
    }
    conversion.saturated =
        conversion.code == std::numeric_limits<std::int32_t>::max() ||
        conversion.code == std::numeric_limits<std::int32_t>::min();
    conversion.context = conversion_context_;

    if (status_byte_enabled_ && conversion.status.faulted()) {
        return Status::fault_active;
    }
    if (status_byte_enabled_ && !conversion.status.adc1_new) {
        return Status::busy;
    }
    if (conversion.saturated) {
        return Status::fault_active;
    }
    return Status::ok;
}

Status Ads1262::read_conversion(
    Conversion& conversion,
    const std::uint32_t timeout_us) noexcept {
    conversion = Conversion{};
    if (!interface_configuration_known_) {
        return Status::not_configured;
    }
    const std::uint64_t started = clock_.now_us();
    Status result = wait_profile_settled(timeout_us);
    if (!is_ok(result)) {
        return result;
    }
    const std::uint64_t elapsed = clock_.now_us() - started;
    if (elapsed >= timeout_us) {
        return Status::timeout;
    }
    result = wait_data_ready(
        static_cast<std::uint32_t>(static_cast<std::uint64_t>(timeout_us) - elapsed));
    if (!is_ok(result)) {
        return result;
    }
    return read_conversion_frame(conversion);
}

}  // namespace gravimetra::drivers
