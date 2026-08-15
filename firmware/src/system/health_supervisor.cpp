#include "gravimetra/system/health_supervisor.hpp"

namespace gravimetra::system {

HealthSupervisor::HealthSupervisor(
    hal::Watchdog& watchdog,
    const SubsystemMask required_subsystems) noexcept
    : watchdog_(watchdog),
      required_subsystems_(required_subsystems & kKnownSubsystems) {}

Status HealthSupervisor::note_progress(const Subsystem subsystem) noexcept {
    const std::size_t index = static_cast<std::size_t>(subsystem);
    if (index >= progress_.size()) {
        return Status::invalid_argument;
    }
    ++progress_[index];
    return Status::ok;
}

Status HealthSupervisor::try_refresh() noexcept {
    if (!refresh_permitted_) {
        return Status::fault_active;
    }
    if (required_subsystems_ == 0U) {
        return Status::not_configured;
    }
    if (waiting_for() != 0U) {
        return Status::busy;
    }

    watchdog_.refresh();
    for (std::size_t index = 0U; index < progress_.size(); ++index) {
        const SubsystemMask mask = SubsystemMask{1U} << index;
        if ((required_subsystems_ & mask) != 0U) {
            consumed_[index] = progress_[index];
        }
    }
    ++refresh_count_;
    return Status::ok;
}

void HealthSupervisor::set_refresh_permitted(const bool permitted) noexcept {
    if (permitted && !refresh_permitted_) {
        // Progress accumulated before or during an inhibit is not evidence of
        // post-recovery health. Start a fresh epoch for every subsystem before
        // allowing the next watchdog refresh.
        consumed_ = progress_;
    }
    refresh_permitted_ = permitted;
}

void HealthSupervisor::set_required_subsystems(
    const SubsystemMask required_subsystems) noexcept {
    const SubsystemMask next = required_subsystems & kKnownSubsystems;
    const SubsystemMask newly_required = next & ~required_subsystems_;
    for (std::size_t index = 0U; index < progress_.size(); ++index) {
        const SubsystemMask mask = SubsystemMask{1U} << index;
        if ((newly_required & mask) != 0U) {
            consumed_[index] = progress_[index];
        }
    }
    required_subsystems_ = next;
}

SubsystemMask HealthSupervisor::required_subsystems() const noexcept {
    return required_subsystems_;
}

SubsystemMask HealthSupervisor::waiting_for() const noexcept {
    SubsystemMask waiting = 0U;
    for (std::size_t index = 0U; index < progress_.size(); ++index) {
        const SubsystemMask mask = SubsystemMask{1U} << index;
        if ((required_subsystems_ & mask) != 0U &&
            progress_[index] == consumed_[index]) {
            waiting |= mask;
        }
    }
    return waiting;
}

std::uint32_t HealthSupervisor::refresh_count() const noexcept {
    return refresh_count_;
}

}  // namespace gravimetra::system
