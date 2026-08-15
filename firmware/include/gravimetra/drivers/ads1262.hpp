#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::drivers {

class Ads1262 {
public:
    enum class Register : std::uint8_t {
        id = 0x00U,
        power = 0x01U,
        interface = 0x02U,
        mode0 = 0x03U,
        mode1 = 0x04U,
        mode2 = 0x05U,
        input_mux = 0x06U,
        offset_calibration_0 = 0x07U,
        offset_calibration_1 = 0x08U,
        offset_calibration_2 = 0x09U,
        full_scale_calibration_0 = 0x0AU,
        full_scale_calibration_1 = 0x0BU,
        full_scale_calibration_2 = 0x0CU,
        idac_mux = 0x0DU,
        idac_magnitude = 0x0EU,
        reference_mux = 0x0FU,
        tdac_positive = 0x10U,
        tdac_negative = 0x11U,
        gpio_connection = 0x12U,
        gpio_direction = 0x13U,
        gpio_data = 0x14U,
    };

    enum class Input : std::uint8_t {
        ain0 = 0U,
        ain1 = 1U,
        ain2 = 2U,
        ain3 = 3U,
        ain4 = 4U,
        ain5 = 5U,
        ain6 = 6U,
        ain7 = 7U,
        ain8 = 8U,
        ain9 = 9U,
        ain_common = 10U,
        temperature_monitor = 11U,
        analog_supply_monitor = 12U,
        digital_supply_monitor = 13U,
        tdac_test = 14U,
        floating = 15U,
    };

    enum class DigitalFilter : std::uint8_t {
        sinc1 = 0U,
        sinc2 = 1U,
        sinc3 = 2U,
        sinc4 = 3U,
        fir = 4U,
    };

    enum class DataRate : std::uint8_t {
        sps_2_5 = 0U,
        sps_5 = 1U,
        sps_10 = 2U,
        sps_16_6 = 3U,
        sps_20 = 4U,
        sps_50 = 5U,
        sps_60 = 6U,
        sps_100 = 7U,
        sps_400 = 8U,
        sps_1200 = 9U,
        sps_2400 = 10U,
        sps_4800 = 11U,
        sps_7200 = 12U,
        sps_14400 = 13U,
        sps_19200 = 14U,
        sps_38400 = 15U,
    };

    enum class Gain : std::uint8_t {
        x1 = 0U,
        x2 = 1U,
        x4 = 2U,
        x8 = 3U,
        x16 = 4U,
        x32 = 5U,
    };

    enum class Reference : std::uint8_t {
        internal_2_5_v = 0U,
        external_ain0_ain1 = 1U,
        external_ain2_ain3 = 2U,
        external_ain4_ain5 = 3U,
        analog_supply = 4U,
    };

    enum class ProfileId : std::uint8_t {
        none = 0U,
        fast_dispense,
        precision_settle,
    };

    enum class DataIntegrity : std::uint8_t {
        disabled = 0U,
        checksum = 1U,
        crc8_atm = 2U,
    };

    struct Profile {
        bool configured{false};
        ProfileId id{ProfileId::none};
        Input positive_input{Input::floating};
        Input negative_input{Input::floating};
        DigitalFilter filter{DigitalFilter::sinc1};
        DataRate data_rate{DataRate::sps_2_5};
        Gain gain{Gain::x1};
        Reference reference{Reference::internal_2_5_v};
        bool bypass_pga{false};
        bool pulse_conversion{false};
        std::uint8_t conversion_delay_code{0U};
        std::uint32_t settling_time_us{0U};
    };

    struct ProfileSet {
        Profile fast_dispense{};
        Profile precision_settle{};
    };

    struct StatusByte {
        bool adc2_new{false};
        bool adc1_new{false};
        bool external_clock{false};
        bool reference_alarm{false};
        bool pga_low_alarm{false};
        bool pga_high_alarm{false};
        bool pga_differential_alarm{false};
        bool reset_detected{false};

        [[nodiscard]] constexpr bool faulted() const noexcept {
            return reference_alarm || pga_low_alarm || pga_high_alarm ||
                   pga_differential_alarm || reset_detected;
        }
    };

    struct ConversionContext {
        bool verified{false};
        ProfileId profile{ProfileId::none};
        Gain gain{Gain::x1};
        Reference reference{Reference::internal_2_5_v};
        bool pga_bypassed{false};
    };

    struct Conversion {
        std::int32_t code{0};
        std::uint8_t raw_status{0U};
        StatusByte status{};
        bool saturated{false};
        ConversionContext context{};
    };

    Ads1262(
        hal::SpiBus& spi,
        hal::DigitalInput& data_ready,
        const hal::MonotonicClock& clock,
        std::uint32_t spi_timeout_us) noexcept;

    [[nodiscard]] Status reset() noexcept;
    // Call only after the data-sheet reset recovery interval has elapsed.
    [[nodiscard]] Status verify_device() noexcept;
    [[nodiscard]] Status clear_reset_indicator() noexcept;
    [[nodiscard]] Status start() noexcept;
    [[nodiscard]] Status stop() noexcept;

    [[nodiscard]] Status read_register(
        Register address,
        std::uint8_t& value) noexcept;
    [[nodiscard]] Status read_registers(
        Register first,
        std::uint8_t* destination,
        std::size_t count) noexcept;
    [[nodiscard]] Status write_register(
        Register address,
        std::uint8_t value) noexcept;
    [[nodiscard]] Status write_registers(
        Register first,
        const std::uint8_t* values,
        std::size_t count) noexcept;
    [[nodiscard]] Status write_register_verified(
        Register address,
        std::uint8_t value,
        std::uint8_t verification_mask = 0xFFU) noexcept;

    [[nodiscard]] Status configure_interface(
        bool interface_timeout_enabled,
        bool status_byte_enabled,
        DataIntegrity integrity) noexcept;
    [[nodiscard]] Status select_channel(
        Input positive,
        Input negative,
        std::uint32_t settling_time_us) noexcept;
    [[nodiscard]] Status apply_profile(const Profile& profile) noexcept;
    [[nodiscard]] Status apply_profile(
        ProfileId profile,
        const ProfileSet& profiles) noexcept;
    [[nodiscard]] Status verify_profile(const Profile& profile) noexcept;

    [[nodiscard]] Status wait_data_ready(std::uint32_t timeout_us) noexcept;
    [[nodiscard]] Status read_conversion(
        Conversion& conversion,
        std::uint32_t timeout_us) noexcept;

    [[nodiscard]] bool profile_settled() const noexcept;
    [[nodiscard]] std::uint32_t remaining_settling_time_us() const noexcept;
    [[nodiscard]] ProfileId active_profile() const noexcept {
        return conversion_context_.profile;
    }
    [[nodiscard]] const ConversionContext& conversion_context() const noexcept {
        return conversion_context_;
    }

    [[nodiscard]] static constexpr double gain_value(const Gain gain) noexcept {
        return static_cast<std::uint8_t>(gain) <=
                       static_cast<std::uint8_t>(Gain::x32)
                   ? static_cast<double>(static_cast<std::uint32_t>(1U)
                                         << static_cast<std::uint8_t>(gain))
                   : 0.0;
    }
    [[nodiscard]] static Status validate_profile(const Profile& profile) noexcept;
    [[nodiscard]] static Status validate_profile_set(
        const ProfileSet& profiles) noexcept;
    [[nodiscard]] static constexpr StatusByte decode_status(
        const std::uint8_t value) noexcept {
        return StatusByte{
            (value & 0x80U) != 0U,
            (value & 0x40U) != 0U,
            (value & 0x20U) != 0U,
            (value & 0x10U) != 0U,
            (value & 0x08U) != 0U,
            (value & 0x04U) != 0U,
            (value & 0x02U) != 0U,
            (value & 0x01U) != 0U,
        };
    }

private:
    static constexpr std::size_t kRegisterCount = 21U;
    static constexpr std::size_t kMaximumTransactionLength = kRegisterCount + 2U;

    [[nodiscard]] Status command(std::uint8_t opcode) noexcept;
    void invalidate_active_profile() noexcept;
    [[nodiscard]] Status wait_profile_settled(std::uint32_t timeout_us) noexcept;
    [[nodiscard]] Status read_conversion_frame(Conversion& conversion) noexcept;
    [[nodiscard]] static std::uint8_t checksum(
        const std::uint8_t* data,
        std::size_t length) noexcept;
    [[nodiscard]] static std::uint8_t crc8_atm(
        const std::uint8_t* data,
        std::size_t length) noexcept;

    hal::SpiBus& spi_;
    hal::DigitalInput& data_ready_;
    const hal::MonotonicClock& clock_;
    std::uint32_t spi_timeout_us_;
    bool status_byte_enabled_{true};
    DataIntegrity integrity_{DataIntegrity::checksum};
    bool interface_configuration_known_{false};
    ConversionContext conversion_context_{};
    std::uint64_t profile_change_time_us_{0U};
    std::uint32_t profile_settling_time_us_{0U};
};

}  // namespace gravimetra::drivers
