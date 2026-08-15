#include "gravimetra/power/power_manager.hpp"

namespace gravimetra::power {

PowerManager::PowerManager(
    const ChargerControlConfig& config,
    hal::DigitalOutput* const charge_suspend_output) noexcept
    : config_(config), charge_suspend_output_(charge_suspend_output) {}

PowerSource PowerManager::determine_source(const PowerInputs& inputs) noexcept {
    if (!inputs.valid) {
        return PowerSource::unknown;
    }
    if (inputs.external_power_present) {
        return PowerSource::external;
    }
    if (inputs.battery_present) {
        return PowerSource::battery;
    }
    return PowerSource::none;
}

Status PowerManager::set_suspend_request(const bool requested) noexcept {
    if (!config_.suspend_control_available) {
        state_.charge_suspend_requested = false;
        return requested ? Status::not_configured : Status::ok;
    }
    if (charge_suspend_output_ == nullptr) {
        state_.charge_suspend_requested = false;
        return Status::not_configured;
    }
    if (state_.charge_suspend_requested == requested) {
        return Status::ok;
    }
    const Status status = charge_suspend_output_->write(requested);
    if (is_ok(status)) {
        state_.charge_suspend_requested = requested;
    }
    return status;
}

Status PowerManager::update(
    const PowerInputs& inputs,
    const bool precision_settling) noexcept {
    const PowerSource next_source = determine_source(inputs);
    if (inputs.valid) {
        if (have_valid_source_ && next_source != last_valid_source_) {
            ++state_.source_transition_count;
        }
        have_valid_source_ = true;
        last_valid_source_ = next_source;
    }
    state_.source = next_source;
    state_.battery_voltage_v = inputs.battery_voltage_v;

    const bool should_suspend = config_.suspend_control_available &&
        inputs.valid && next_source == PowerSource::external &&
        precision_settling;
    const Status suspend_status = set_suspend_request(should_suspend);

    if (!inputs.valid || !inputs.charger_status_available) {
        state_.charger = ChargerState::unavailable;
    } else if (inputs.charger_fault) {
        state_.charger = ChargerState::fault;
    } else if (state_.charge_suspend_requested) {
        state_.charger = ChargerState::suspended_for_precision;
    } else if (inputs.charger_charging) {
        state_.charger = ChargerState::charging;
    } else if (inputs.charger_charge_complete) {
        state_.charger = ChargerState::charge_complete;
    } else {
        state_.charger = ChargerState::idle;
    }

    return suspend_status;
}

const PowerState& PowerManager::state() const noexcept {
    return state_;
}

}  // namespace gravimetra::power
