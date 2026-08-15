#include "gravimetra/safety/estop_manager.hpp"

namespace gravimetra::safety {

EstopManager::EstopManager(
    hal::DigitalInput& monitored_contact,
    motion::AugerManager& augers,
    SafetyInhibitTarget& check_mass_actuator) noexcept
    : monitored_contact_(monitored_contact),
      augers_(augers),
      check_mass_actuator_(check_mass_actuator) {
    // Fail safe until the first successful read proves the contact released.
    last_shutdown_status_ = enforce_shutdown();
}

Status EstopManager::poll() noexcept {
    bool asserted = false;
    const Status read_status = monitored_contact_.read(asserted);
    if (!is_ok(read_status)) {
        contact_asserted_ = true;
        trip(EstopState::monitor_fault);
        return read_status;
    }

    contact_asserted_ = asserted;
    if (asserted) {
        trip(EstopState::asserted);
        return Status::fault_active;
    }

    if (state_ == EstopState::uninitialized) {
        return arm();
    }
    if (state_ == EstopState::asserted) {
        state_ = EstopState::released_waiting_reset;
    }
    if (state_ == EstopState::monitor_fault ||
        state_ == EstopState::released_waiting_reset) {
        last_shutdown_status_ = enforce_shutdown();
        return is_ok(last_shutdown_status_) ? Status::fault_active
                                            : last_shutdown_status_;
    }
    return Status::ok;
}

Status EstopManager::reset_latch() noexcept {
    bool asserted = false;
    const Status read_status = monitored_contact_.read(asserted);
    if (!is_ok(read_status)) {
        contact_asserted_ = true;
        trip(EstopState::monitor_fault);
        return read_status;
    }
    contact_asserted_ = asserted;
    if (asserted) {
        trip(EstopState::asserted);
        return Status::fault_active;
    }
    return arm();
}

void EstopManager::trip(const EstopState state) noexcept {
    last_shutdown_status_ = enforce_shutdown();
    state_ = state;
}

Status EstopManager::arm() noexcept {
    contact_asserted_ = false;
    const Status shutdown_status = enforce_shutdown();
    if (!is_ok(shutdown_status) || augers_.shutdown_fault_latched()) {
        state_ = EstopState::released_waiting_reset;
        last_shutdown_status_ = is_ok(shutdown_status)
                                    ? Status::fault_active
                                    : shutdown_status;
        return last_shutdown_status_;
    }
    const Status motion_status = augers_.set_motion_permitted(true);
    if (!is_ok(motion_status)) {
        static_cast<void>(check_mass_actuator_.inhibit_for_safety());
        state_ = EstopState::released_waiting_reset;
        last_shutdown_status_ = motion_status;
        return motion_status;
    }
    const Status release_status =
        check_mass_actuator_.release_safety_inhibit();
    if (!is_ok(release_status)) {
        static_cast<void>(augers_.set_motion_permitted(false));
        static_cast<void>(check_mass_actuator_.inhibit_for_safety());
        state_ = EstopState::released_waiting_reset;
        last_shutdown_status_ = release_status;
        return release_status;
    }
    state_ = EstopState::armed;
    last_shutdown_status_ = Status::ok;
    return Status::ok;
}

Status EstopManager::enforce_shutdown() noexcept {
    // Always attempt both independent firmware shutdown paths. Returning the
    // first failure preserves a concrete diagnostic without allowing one
    // failed output to skip the other safety action.
    const Status motion_status = augers_.set_motion_permitted(false);
    const Status check_status = check_mass_actuator_.inhibit_for_safety();
    return !is_ok(motion_status) ? motion_status : check_status;
}

}  // namespace gravimetra::safety
