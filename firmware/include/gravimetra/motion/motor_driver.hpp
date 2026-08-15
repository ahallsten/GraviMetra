#pragma once

#include "gravimetra/common/status.hpp"

#include <cstdint>

namespace gravimetra::motion {

enum class MotorDirection : std::uint8_t {
    forward = 0,
    reverse,
};

[[nodiscard]] constexpr bool valid_motor_direction(
    const MotorDirection direction) noexcept {
    switch (direction) {
        case MotorDirection::forward:
        case MotorDirection::reverse:
            return true;
    }
    return false;
}

enum class MicrostepResolution : std::uint16_t {
    invalid = 0,
    full_step = 1,
    x2 = 2,
    x4 = 4,
    x8 = 8,
    x16 = 16,
    x32 = 32,
    x64 = 64,
    x128 = 128,
    x256 = 256,
};

enum class ChopperMode : std::uint8_t {
    unconfigured = 0,
    stealth_chop,
    spread_cycle,
};

[[nodiscard]] constexpr bool valid_chopper_mode(
    const ChopperMode mode) noexcept {
    switch (mode) {
        case ChopperMode::stealth_chop:
        case ChopperMode::spread_cycle:
            return true;
        case ChopperMode::unconfigured:
            return false;
    }
    return false;
}

enum class MotorEnableState : std::uint8_t {
    disabled = 0,
    enabled,
    unknown,
};

[[nodiscard]] constexpr bool valid_microstep_resolution(
    const MicrostepResolution resolution) noexcept {
    switch (resolution) {
        case MicrostepResolution::full_step:
        case MicrostepResolution::x2:
        case MicrostepResolution::x4:
        case MicrostepResolution::x8:
        case MicrostepResolution::x16:
        case MicrostepResolution::x32:
        case MicrostepResolution::x64:
        case MicrostepResolution::x128:
        case MicrostepResolution::x256:
            return true;
        case MicrostepResolution::invalid:
            return false;
    }
    return false;
}

struct MotorElectricalConfig {
    static constexpr std::uint8_t kInvalidCurrentScale = 0xFFU;

    // TMC2209 current scale codes, not milliamps. Conversion to physical
    // current depends on the released module sense network and VREF setting.
    std::uint8_t run_current_scale{kInvalidCurrentScale};
    std::uint8_t hold_current_scale{kInvalidCurrentScale};
    std::uint8_t hold_delay{0xFFU};
    MicrostepResolution microsteps{MicrostepResolution::invalid};
    ChopperMode chopper_mode{ChopperMode::unconfigured};
    bool interpolate_to_256{false};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return run_current_scale <= 31U && hold_current_scale <= 31U &&
               hold_delay <= 15U && valid_microstep_resolution(microsteps) &&
               valid_chopper_mode(chopper_mode);
    }
};

struct MotorDriverStatus {
    bool communication_ok{false};
    bool reset_detected{false};
    bool driver_error_latched{false};
    bool charge_pump_undervoltage{false};
    bool overtemperature_warning{false};
    bool overtemperature_shutdown{false};
    bool short_to_ground{false};
    bool low_side_short{false};
    bool open_load_a{false};
    bool open_load_b{false};
    bool standstill{false};
    bool stealth_chop_active{false};
    std::uint8_t current_scale_actual{0U};
    std::uint32_t global_status_raw{0U};
    std::uint32_t driver_status_raw{0U};

    [[nodiscard]] constexpr bool critical_fault() const noexcept {
        return !communication_ok || reset_detected || driver_error_latched ||
               charge_pump_undervoltage ||
               overtemperature_shutdown || short_to_ground || low_side_short;
    }
};

class MotorDriver {
public:
    virtual ~MotorDriver() = default;

    [[nodiscard]] virtual Status configure(
        const MotorElectricalConfig& config) noexcept = 0;
    [[nodiscard]] virtual Status read_status(
        MotorDriverStatus& status) noexcept = 0;
    [[nodiscard]] virtual Status set_enabled(bool enabled) noexcept = 0;
    [[nodiscard]] virtual bool enabled() const noexcept = 0;
    // Unknown must be treated as energized by interlocks. The default keeps
    // simple drivers source-compatible; hardware drivers with fallible enable
    // outputs should override it.
    [[nodiscard]] virtual MotorEnableState enable_state() const noexcept {
        return enabled() ? MotorEnableState::enabled
                         : MotorEnableState::disabled;
    }
};

}  // namespace gravimetra::motion
