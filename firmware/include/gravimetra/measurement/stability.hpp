#pragma once

#include "gravimetra/common/status.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::measurement {

inline constexpr std::size_t kMaximumStabilityWindow = 128U;

enum class StabilityReason : std::uint32_t {
    none = 0U,
    not_configured = 1U << 0U,
    insufficient_samples = 1U << 1U,
    insufficient_duration = 1U << 2U,
    excessive_slope = 1U << 3U,
    excessive_standard_deviation = 1U << 4U,
    excessive_peak_to_peak = 1U << 5U,
    invalid_measurement = 1U << 6U,
    invalid_timestamp = 1U << 7U,
    optical_invalid = 1U << 8U,
    optical_out_of_bounds = 1U << 9U,
    current_saturated = 1U << 10U,
    power_fault = 1U << 11U,
    temperature_fault = 1U << 12U,
    sample_gap = 1U << 13U,
};

[[nodiscard]] constexpr std::uint32_t reason_mask(
    const StabilityReason reason) noexcept {
    return static_cast<std::uint32_t>(reason);
}

struct StabilityConfiguration {
    bool configured{false};
    std::size_t window_size{0U};
    std::size_t minimum_valid_samples{0U};
    std::uint64_t minimum_stable_duration_us{0U};
    std::uint64_t maximum_sample_gap_us{0U};
    double maximum_absolute_slope_mg_per_s{0.0};
    double maximum_standard_deviation_mg{0.0};
    double maximum_peak_to_peak_mg{0.0};
    bool require_optical_diagnostic{false};
    double maximum_absolute_optical_error{0.0};
    double maximum_absolute_coil_current_a{0.0};
};

struct StabilitySample {
    std::uint64_t timestamp_us{0U};
    double mass_mg{0.0};
    bool measurement_valid{false};
    double optical_position_error{0.0};
    bool optical_valid{false};
    double coil_current_a{0.0};
    bool power_fault{false};
    bool temperature_fault{false};
};

struct StabilityDiagnostics {
    bool stable{false};
    std::uint32_t reasons{reason_mask(StabilityReason::not_configured)};
    std::size_t sample_count{0U};
    std::size_t valid_sample_count{0U};
    std::uint64_t window_duration_us{0U};
    std::uint64_t continuously_passing_duration_us{0U};
    double mean_mg{0.0};
    double slope_mg_per_s{0.0};
    double standard_deviation_mg{0.0};
    double peak_to_peak_mg{0.0};
    double maximum_absolute_optical_error{0.0};
    double maximum_absolute_coil_current_a{0.0};

    [[nodiscard]] constexpr bool has_reason(
        const StabilityReason reason) const noexcept {
        return (reasons & reason_mask(reason)) != 0U;
    }
};

class StabilityDetector {
public:
    [[nodiscard]] Status configure(
        const StabilityConfiguration& configuration) noexcept;
    void reset() noexcept;
    [[nodiscard]] Status add_sample(const StabilitySample& sample) noexcept;
    [[nodiscard]] const StabilityDiagnostics& diagnostics() const noexcept {
        return diagnostics_;
    }

private:
    void recompute() noexcept;
    void add_reason(StabilityReason reason) noexcept;
    [[nodiscard]] const StabilitySample& chronological_sample(
        std::size_t index) const noexcept;

    StabilityConfiguration configuration_{};
    std::array<StabilitySample, kMaximumStabilityWindow> samples_{};
    std::size_t count_{0U};
    std::size_t next_{0U};
    bool passing_since_valid_{false};
    std::uint64_t passing_since_us_{0U};
    StabilityDiagnostics diagnostics_{};
};

}  // namespace gravimetra::measurement
