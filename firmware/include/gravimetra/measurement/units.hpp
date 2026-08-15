#pragma once

namespace gravimetra::measurement {

inline constexpr double kMilligramsPerGrain = 64.79891;

[[nodiscard]] constexpr double milligrams_to_grains(
    const double milligrams) noexcept {
    return milligrams / kMilligramsPerGrain;
}

[[nodiscard]] constexpr double grains_to_milligrams(
    const double grains) noexcept {
    return grains * kMilligramsPerGrain;
}

}  // namespace gravimetra::measurement
