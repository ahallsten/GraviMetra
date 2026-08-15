#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"
#include "gravimetra/motion/motor_driver.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::drivers {

enum class Tmc2209Register : std::uint8_t {
    gconf = 0x00U,
    gstat = 0x01U,
    ifcnt = 0x02U,
    nodeconf = 0x03U,
    ioin = 0x06U,
    ihold_irun = 0x10U,
    chopconf = 0x6CU,
    drv_status = 0x6FU,
    pwmconf = 0x70U,
};

struct Tmc2209InitConfig {
    static constexpr std::uint8_t kInvalidNodeAddress = 0xFFU;

    std::uint8_t node_address{kInvalidNodeAddress};
    motion::MotorElectricalConfig motor{};

    // These options depend on the released carrier/module implementation and
    // therefore have no implied hardware default.
    bool interface_options_configured{false};
    bool use_vref_pin_for_current_scale{false};
    bool use_internal_sense_resistors{false};
    bool multistep_filter{false};

    std::uint32_t read_timeout_us{0U};
    std::uint16_t maximum_read_polls{0U};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return node_address <= 3U && motor.valid() &&
               interface_options_configured && read_timeout_us > 0U &&
               maximum_read_polls > 0U;
    }
};

class Tmc2209 final : public motion::MotorDriver {
public:
    static constexpr std::uint8_t kSyncByte = 0x05U;
    static constexpr std::uint8_t kMasterAddress = 0xFFU;
    static constexpr std::uint8_t kExpectedVersion = 0x21U;
    static constexpr std::size_t kReadRequestLength = 4U;
    static constexpr std::size_t kDatagramLength = 8U;

    Tmc2209(
        hal::Uart& uart,
        hal::DigitalOutput& enable_output,
        const hal::MonotonicClock& clock) noexcept;

    [[nodiscard]] Status initialize(
        const Tmc2209InitConfig& config) noexcept;
    [[nodiscard]] Status verify_configuration() noexcept;

    [[nodiscard]] Status read_register(
        Tmc2209Register reg,
        std::uint32_t& value) noexcept;
    [[nodiscard]] Status write_register(
        Tmc2209Register reg,
        std::uint32_t value) noexcept;

    [[nodiscard]] Status set_current(
        std::uint8_t run_current_scale,
        std::uint8_t hold_current_scale,
        std::uint8_t hold_delay) noexcept;
    [[nodiscard]] Status set_microsteps(
        motion::MicrostepResolution resolution,
        bool interpolate_to_256) noexcept;
    [[nodiscard]] Status set_chopper_mode(
        motion::ChopperMode mode) noexcept;

    [[nodiscard]] Status configure(
        const motion::MotorElectricalConfig& config) noexcept override;
    [[nodiscard]] Status read_status(
        motion::MotorDriverStatus& status) noexcept override;
    [[nodiscard]] Status set_enabled(bool enabled) noexcept override;
    [[nodiscard]] bool enabled() const noexcept override {
        return enable_state_ != motion::MotorEnableState::disabled;
    }
    [[nodiscard]] motion::MotorEnableState enable_state() const noexcept override {
        return enable_state_;
    }

    [[nodiscard]] bool initialized() const noexcept {
        return initialized_;
    }
    [[nodiscard]] std::uint8_t node_address() const noexcept {
        return node_address_;
    }

    [[nodiscard]] static std::uint8_t crc8(
        const std::uint8_t* data,
        std::size_t length) noexcept;
    [[nodiscard]] static std::array<std::uint8_t, kReadRequestLength>
    make_read_request(std::uint8_t node_address, Tmc2209Register reg) noexcept;
    [[nodiscard]] static std::array<std::uint8_t, kDatagramLength>
    make_write_datagram(
        std::uint8_t node_address,
        Tmc2209Register reg,
        std::uint32_t value) noexcept;

private:
    [[nodiscard]] Status write_with_counter_verification(
        Tmc2209Register reg,
        std::uint32_t value) noexcept;
    [[nodiscard]] Status read_reply(
        Tmc2209Register requested_register,
        std::uint32_t& value) noexcept;
    [[nodiscard]] Status force_disabled() noexcept;
    void discard_received_bytes() noexcept;

    hal::Uart& uart_;
    hal::DigitalOutput& enable_output_;
    const hal::MonotonicClock& clock_;
    std::uint8_t node_address_{Tmc2209InitConfig::kInvalidNodeAddress};
    std::uint32_t read_timeout_us_{0U};
    std::uint16_t maximum_read_polls_{0U};
    std::uint32_t gconf_shadow_{0U};
    std::uint32_t chopconf_shadow_{0U};
    std::uint32_t current_shadow_{0U};
    bool initialized_{false};
    motion::MotorEnableState enable_state_{motion::MotorEnableState::unknown};
};

}  // namespace gravimetra::drivers
