#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::system {

enum class Subsystem : std::uint8_t {
    acquisition = 0,
    measurement,
    safety,
    motion,
    application,
    communications,
    power,
    storage,
    count,
};

using SubsystemMask = std::uint32_t;

[[nodiscard]] constexpr SubsystemMask subsystem_mask(
    const Subsystem subsystem) noexcept {
    return SubsystemMask{1U} << static_cast<std::uint8_t>(subsystem);
}

class HealthSupervisor {
public:
    HealthSupervisor(
        hal::Watchdog& watchdog,
        SubsystemMask required_subsystems) noexcept;

    // Heartbeat calls and refresh attempts must be serialized by the scheduler
    // (or externally protected) when used from multiple execution contexts.
    [[nodiscard]] Status note_progress(Subsystem subsystem) noexcept;
    [[nodiscard]] Status try_refresh() noexcept;

    void set_refresh_permitted(bool permitted) noexcept;
    void set_required_subsystems(SubsystemMask required_subsystems) noexcept;
    [[nodiscard]] SubsystemMask required_subsystems() const noexcept;
    [[nodiscard]] SubsystemMask waiting_for() const noexcept;
    [[nodiscard]] std::uint32_t refresh_count() const noexcept;

private:
    static constexpr std::size_t kSubsystemCount =
        static_cast<std::size_t>(Subsystem::count);
    static constexpr SubsystemMask kKnownSubsystems =
        (SubsystemMask{1U} << kSubsystemCount) - 1U;

    hal::Watchdog& watchdog_;
    SubsystemMask required_subsystems_{0U};
    std::array<std::uint32_t, kSubsystemCount> progress_{};
    std::array<std::uint32_t, kSubsystemCount> consumed_{};
    bool refresh_permitted_{true};
    std::uint32_t refresh_count_{0U};
};

}  // namespace gravimetra::system
