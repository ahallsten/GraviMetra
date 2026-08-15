#include "gravimetra/drivers/tmp117.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace gravimetra::drivers {

Tmp117::Tmp117(
    hal::I2cBus& bus,
    const std::uint8_t address,
    const std::uint32_t bus_timeout_us) noexcept
    : bus_(bus), address_(address), bus_timeout_us_(bus_timeout_us) {}

bool Tmp117::address_valid() const noexcept {
    return address_ >= kMinimumAddress && address_ <= kMaximumAddress;
}

Status Tmp117::read_register(
    const Register address,
    std::uint16_t& value) noexcept {
    if (!address_valid()) {
        return Status::not_configured;
    }
    const std::array<std::uint8_t, 1U> transmit{
        static_cast<std::uint8_t>(address)};
    std::array<std::uint8_t, 2U> receive{};
    const Status result = bus_.write_read(
        address_,
        transmit.data(),
        transmit.size(),
        receive.data(),
        receive.size(),
        bus_timeout_us_);
    if (!is_ok(result)) {
        return result;
    }
    value = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(receive[0]) << 8U) |
        static_cast<std::uint16_t>(receive[1]));
    return Status::ok;
}

Status Tmp117::write_register(
    const Register address,
    const std::uint16_t value) noexcept {
    if (!address_valid()) {
        return Status::not_configured;
    }
    const std::array<std::uint8_t, 3U> data{
        static_cast<std::uint8_t>(address),
        static_cast<std::uint8_t>(value >> 8U),
        static_cast<std::uint8_t>(value & 0xFFU),
    };
    return bus_.write(address_, data.data(), data.size(), bus_timeout_us_);
}

Status Tmp117::write_register_verified(
    const Register address,
    const std::uint16_t value,
    const std::uint16_t verification_mask) noexcept {
    Status result = write_register(address, value);
    if (!is_ok(result)) {
        return result;
    }
    std::uint16_t actual = 0U;
    result = read_register(address, actual);
    if (!is_ok(result)) {
        return result;
    }
    return ((actual & verification_mask) == (value & verification_mask))
               ? Status::ok
               : Status::verification_failed;
}

Status Tmp117::verify_device() noexcept {
    std::uint16_t id = 0U;
    const Status result = read_register(Register::device_id, id);
    if (!is_ok(result)) {
        return result;
    }
    return (id & 0x0FFFU) == kDeviceId ? Status::ok
                                       : Status::verification_failed;
}

Status Tmp117::soft_reset() noexcept {
    // The reset bit self-clears and therefore cannot be verified by readback.
    return write_register(Register::configuration, 0x0002U);
}

Status Tmp117::read_temperature(Reading& reading) noexcept {
    std::uint16_t raw = 0U;
    const Status result = read_register(Register::temperature, raw);
    if (!is_ok(result)) {
        return result;
    }
    if (raw == 0x8000U) {
        // Datasheet power-on sentinel: no completed conversion yet.
        return Status::busy;
    }
    const std::int32_t signed_value =
        raw <= static_cast<std::uint16_t>(std::numeric_limits<std::int16_t>::max())
            ? static_cast<std::int32_t>(raw)
            : -1 - static_cast<std::int32_t>(0xFFFFU - raw);
    reading.raw_code = static_cast<std::int16_t>(signed_value);
    reading.celsius = code_to_celsius(reading.raw_code);
    return temperature_in_operating_range(reading.celsius)
               ? Status::ok
               : Status::fault_active;
}

}  // namespace gravimetra::drivers
