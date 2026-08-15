#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"

#include <cstdint>

namespace gravimetra::drivers {

class Tmp117 {
public:
    enum class Register : std::uint8_t {
        temperature = 0x00U,
        configuration = 0x01U,
        high_limit = 0x02U,
        low_limit = 0x03U,
        eeprom_unlock = 0x04U,
        eeprom1 = 0x05U,
        eeprom2 = 0x06U,
        temperature_offset = 0x07U,
        eeprom3 = 0x08U,
        device_id = 0x0FU,
    };

    struct Reading {
        std::int16_t raw_code{0};
        double celsius{0.0};
    };

    static constexpr std::uint8_t kMinimumAddress = 0x48U;
    static constexpr std::uint8_t kMaximumAddress = 0x4BU;
    static constexpr std::uint16_t kDeviceId = 0x0117U;
    static constexpr double kDegreesCPerLsb = 0.0078125;
    static constexpr std::uint32_t kSoftResetRecoveryUs = 2'000U;

    Tmp117(
        hal::I2cBus& bus,
        std::uint8_t address,
        std::uint32_t bus_timeout_us) noexcept;

    [[nodiscard]] bool address_valid() const noexcept;
    [[nodiscard]] Status verify_device() noexcept;
    // Starts the device's reset sequence; success confirms only that the I2C
    // write was accepted. The caller/target scheduler must enforce the TMP117
    // data-sheet recovery interval (kSoftResetRecoveryUs) before subsequent
    // access.
    [[nodiscard]] Status soft_reset() noexcept;
    [[nodiscard]] Status read_register(
        Register address,
        std::uint16_t& value) noexcept;
    [[nodiscard]] Status write_register(
        Register address,
        std::uint16_t value) noexcept;
    [[nodiscard]] Status write_register_verified(
        Register address,
        std::uint16_t value,
        std::uint16_t verification_mask = 0xFFFFU) noexcept;
    [[nodiscard]] Status read_temperature(Reading& reading) noexcept;

    [[nodiscard]] static constexpr double code_to_celsius(
        const std::int16_t code) noexcept {
        return static_cast<double>(code) * kDegreesCPerLsb;
    }
    [[nodiscard]] static constexpr bool temperature_in_operating_range(
        const double celsius) noexcept {
        return celsius >= -55.0 && celsius <= 150.0;
    }

private:
    hal::I2cBus& bus_;
    std::uint8_t address_;
    std::uint32_t bus_timeout_us_;
};

}  // namespace gravimetra::drivers
