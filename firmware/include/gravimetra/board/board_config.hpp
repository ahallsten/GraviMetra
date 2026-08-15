#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::board {

enum class Port : std::uint8_t { unassigned, a, b, c, d, e, f, g };

struct Pin {
    Port port{Port::unassigned};
    std::uint8_t number{0xFFU};
    bool active_low{false};

    [[nodiscard]] constexpr bool assigned() const noexcept {
        return port != Port::unassigned && number < 16U;
    }
};

enum class Peripheral : std::uint8_t {
    unassigned,
    spi1,
    spi2,
    spi3,
    i2c1,
    i2c2,
    i2c3,
    usart1,
    usart2,
    usart3,
    uart4,
    uart5,
    lpuart1,
    fdcan1,
    fdcan2,
    usb_fs,
};

enum class TimerPeripheral : std::uint8_t {
    unassigned,
    tim1,
    tim2,
    tim3,
    tim4,
    tim5,
    tim8,
    tim15,
    tim16,
    tim17,
};

struct TimerChannel {
    TimerPeripheral timer{TimerPeripheral::unassigned};
    std::uint8_t channel{0U};

    [[nodiscard]] constexpr bool assigned() const noexcept {
        return timer != TimerPeripheral::unassigned && channel > 0U &&
               channel <= 6U;
    }
};

struct MotorPins {
    Pin step{};
    TimerChannel step_timer{};
    Pin direction{};
    Pin enable{};
    Peripheral uart{Peripheral::unassigned};
};

struct BoardConfig {
    bool pin_map_released{false};
    std::array<MotorPins, 4U> motors{};
    Peripheral ads1262_spi{Peripheral::unassigned};
    Pin ads1262_chip_select{};
    Pin ads1262_drdy{};
    Pin ads1262_reset{};
    Pin ads1262_start{};
    Peripheral temperature_i2c{Peripheral::unassigned};
    Peripheral nextion_uart{Peripheral::unassigned};
    Peripheral rs485_uart{Peripheral::unassigned};
    Peripheral can_controller{Peripheral::unassigned};
    Peripheral usb{Peripheral::usb_fs};
    Pin rs485_direction_enable{};
    Pin estop_monitor{};
    Pin opa593_current_limit{};
    Pin opa593_thermal_warning{};
    Pin analog_servo_reset{};
    Pin check_mass_drive{};
    Pin check_mass_home{};
    Pin check_mass_applied{};
};

// Deliberately safe and unusable for motion. Replace only with a CubeMX- and
// schematic-validated mapping. Keeping this object constexpr lets target code
// compile without representing an unresolved mapping as released hardware.
inline constexpr BoardConfig kRevisionAUnreleased{};

[[nodiscard]] constexpr bool is_uart_peripheral(
    const Peripheral peripheral) noexcept {
    switch (peripheral) {
        case Peripheral::usart1:
        case Peripheral::usart2:
        case Peripheral::usart3:
        case Peripheral::uart4:
        case Peripheral::uart5:
        case Peripheral::lpuart1:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] constexpr bool same_pin(
    const Pin& left,
    const Pin& right) noexcept {
    return left.assigned() && right.assigned() && left.port == right.port &&
           left.number == right.number;
}

[[nodiscard]] constexpr bool same_timer_channel(
    const TimerChannel& left,
    const TimerChannel& right) noexcept {
    return left.assigned() && right.assigned() && left.timer == right.timer &&
           left.channel == right.channel;
}

[[nodiscard]] constexpr bool motion_mapping_complete(
    const BoardConfig& config) noexcept {
    if (!config.pin_map_released || !config.estop_monitor.assigned()) {
        return false;
    }
    for (std::size_t index = 0U; index < config.motors.size(); ++index) {
        const auto& motor = config.motors[index];
        if (!motor.step.assigned() || !motor.direction.assigned() ||
            !motor.enable.assigned() || !motor.step_timer.assigned() ||
            !is_uart_peripheral(motor.uart) ||
            same_pin(motor.step, motor.direction) ||
            same_pin(motor.step, motor.enable) ||
            same_pin(motor.direction, motor.enable)) {
            return false;
        }
        for (std::size_t other_index = index + 1U;
             other_index < config.motors.size(); ++other_index) {
            const auto& other = config.motors[other_index];
            if (motor.uart == other.uart ||
                same_timer_channel(motor.step_timer, other.step_timer) ||
                same_pin(motor.step, other.step) ||
                same_pin(motor.step, other.direction) ||
                same_pin(motor.step, other.enable) ||
                same_pin(motor.direction, other.step) ||
                same_pin(motor.direction, other.direction) ||
                same_pin(motor.direction, other.enable) ||
                same_pin(motor.enable, other.step) ||
                same_pin(motor.enable, other.direction) ||
                same_pin(motor.enable, other.enable)) {
                return false;
            }
        }
        if (same_pin(config.estop_monitor, motor.step) ||
            same_pin(config.estop_monitor, motor.direction) ||
            same_pin(config.estop_monitor, motor.enable)) {
            return false;
        }
    }
    return true;
}

}  // namespace gravimetra::board
