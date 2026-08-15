#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"

#include <cstdint>

namespace gravimetra::power {

enum class PowerSource : std::uint8_t {
    unknown = 0,
    none,
    battery,
    external,
};

enum class ChargerState : std::uint8_t {
    unavailable = 0,
    idle,
    charging,
    charge_complete,
    suspended_for_precision,
    fault,
};

struct PowerInputs {
    bool valid{false};
    bool external_power_present{false};
    bool battery_present{false};
    bool charger_status_available{false};
    bool charger_charging{false};
    bool charger_charge_complete{false};
    bool charger_fault{false};
    double battery_voltage_v{0.0};
};

struct ChargerControlConfig {
    // The board layer must map logical "suspend requested" to the released
    // hardware polarity. False is the mandatory default while that capability
    // is unresolved.
    bool suspend_control_available{false};
};

struct PowerState {
    PowerSource source{PowerSource::unknown};
    ChargerState charger{ChargerState::unavailable};
    double battery_voltage_v{0.0};
    bool charge_suspend_requested{false};
    std::uint32_t source_transition_count{0U};
};

class PowerManager {
public:
    PowerManager(
        const ChargerControlConfig& config,
        hal::DigitalOutput* charge_suspend_output) noexcept;

    [[nodiscard]] Status update(
        const PowerInputs& inputs,
        bool precision_settling) noexcept;
    [[nodiscard]] const PowerState& state() const noexcept;

private:
    [[nodiscard]] static PowerSource determine_source(
        const PowerInputs& inputs) noexcept;
    [[nodiscard]] Status set_suspend_request(bool requested) noexcept;

    ChargerControlConfig config_{};
    hal::DigitalOutput* charge_suspend_output_{nullptr};
    PowerState state_{};
    bool have_valid_source_{false};
    PowerSource last_valid_source_{PowerSource::unknown};
};

}  // namespace gravimetra::power
