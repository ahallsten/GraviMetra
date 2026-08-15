#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"
#include "gravimetra/motion/auger_manager.hpp"

#include <cstdint>

namespace gravimetra::safety {

class SafetyInhibitTarget {
public:
    virtual ~SafetyInhibitTarget() = default;
    [[nodiscard]] virtual Status inhibit_for_safety() noexcept = 0;
    [[nodiscard]] virtual Status release_safety_inhibit() noexcept = 0;
};

enum class EstopState : std::uint8_t {
    uninitialized = 0,
    armed,
    asserted,
    released_waiting_reset,
    monitor_fault,
};

class EstopManager {
public:
    EstopManager(
        hal::DigitalInput& monitored_contact,
        motion::AugerManager& augers,
        SafetyInhibitTarget& check_mass_actuator) noexcept;

    // Poll must be called from the safety-rate loop. Hardware independently
    // removes motion capability; this manager provides the supplementary
    // firmware stop, latch, and diagnostic state.
    [[nodiscard]] Status poll() noexcept;
    [[nodiscard]] Status reset_latch() noexcept;

    [[nodiscard]] EstopState state() const noexcept {
        return state_;
    }
    [[nodiscard]] bool motion_inhibited() const noexcept {
        return state_ != EstopState::armed;
    }
    [[nodiscard]] bool asserted() const noexcept {
        return contact_asserted_;
    }
    [[nodiscard]] Status last_shutdown_status() const noexcept {
        return last_shutdown_status_;
    }

private:
    void trip(EstopState state) noexcept;
    [[nodiscard]] Status arm() noexcept;
    [[nodiscard]] Status enforce_shutdown() noexcept;

    hal::DigitalInput& monitored_contact_;
    motion::AugerManager& augers_;
    SafetyInhibitTarget& check_mass_actuator_;
    EstopState state_{EstopState::uninitialized};
    bool contact_asserted_{false};
    Status last_shutdown_status_{Status::ok};
};

}  // namespace gravimetra::safety
