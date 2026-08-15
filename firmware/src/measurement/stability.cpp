#include "gravimetra/measurement/stability.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace gravimetra::measurement {

Status StabilityDetector::configure(
    const StabilityConfiguration& configuration) noexcept {
    if (!configuration.configured) {
        return Status::not_configured;
    }
    if (configuration.window_size == 0U ||
        configuration.window_size > kMaximumStabilityWindow ||
        configuration.minimum_valid_samples < 2U ||
        configuration.minimum_valid_samples > configuration.window_size ||
        configuration.maximum_sample_gap_us == 0U ||
        !std::isfinite(configuration.maximum_absolute_slope_mg_per_s) ||
        configuration.maximum_absolute_slope_mg_per_s < 0.0 ||
        !std::isfinite(configuration.maximum_standard_deviation_mg) ||
        configuration.maximum_standard_deviation_mg < 0.0 ||
        !std::isfinite(configuration.maximum_peak_to_peak_mg) ||
        configuration.maximum_peak_to_peak_mg < 0.0 ||
        !std::isfinite(configuration.maximum_absolute_optical_error) ||
        configuration.maximum_absolute_optical_error < 0.0 ||
        !std::isfinite(configuration.maximum_absolute_coil_current_a) ||
        configuration.maximum_absolute_coil_current_a <= 0.0) {
        return Status::invalid_argument;
    }
    configuration_ = configuration;
    reset();
    return Status::ok;
}

void StabilityDetector::reset() noexcept {
    samples_.fill(StabilitySample{});
    count_ = 0U;
    next_ = 0U;
    passing_since_valid_ = false;
    passing_since_us_ = 0U;
    diagnostics_ = StabilityDiagnostics{};
    diagnostics_.reasons = configuration_.configured
                               ? reason_mask(StabilityReason::insufficient_samples)
                               : reason_mask(StabilityReason::not_configured);
}

const StabilitySample& StabilityDetector::chronological_sample(
    const std::size_t index) const noexcept {
    const std::size_t oldest = count_ == configuration_.window_size ? next_ : 0U;
    return samples_[(oldest + index) % configuration_.window_size];
}

void StabilityDetector::add_reason(const StabilityReason reason) noexcept {
    diagnostics_.reasons |= reason_mask(reason);
}

Status StabilityDetector::add_sample(const StabilitySample& sample) noexcept {
    if (!configuration_.configured) {
        return Status::not_configured;
    }
    if (count_ > 0U &&
        sample.timestamp_us <= chronological_sample(count_ - 1U).timestamp_us) {
        diagnostics_.stable = false;
        diagnostics_.continuously_passing_duration_us = 0U;
        add_reason(StabilityReason::invalid_timestamp);
        passing_since_valid_ = false;
        return Status::invalid_argument;
    }
    if ((sample.measurement_valid && !std::isfinite(sample.mass_mg)) ||
        !std::isfinite(sample.coil_current_a) ||
        (sample.optical_valid &&
          !std::isfinite(sample.optical_position_error))) {
        diagnostics_.stable = false;
        diagnostics_.continuously_passing_duration_us = 0U;
        add_reason(StabilityReason::invalid_measurement);
        if (sample.optical_valid &&
            !std::isfinite(sample.optical_position_error)) {
            add_reason(StabilityReason::optical_invalid);
        }
        passing_since_valid_ = false;
        return Status::invalid_argument;
    }

    bool sample_gap = false;
    if (count_ > 0U) {
        const std::uint64_t previous_timestamp =
            chronological_sample(count_ - 1U).timestamp_us;
        sample_gap = (sample.timestamp_us - previous_timestamp) >
                     configuration_.maximum_sample_gap_us;
        if (sample_gap) {
            // Samples before the outage cannot establish continuous behavior
            // after it. Start a new window with the current valid sample.
            samples_.fill(StabilitySample{});
            count_ = 0U;
            next_ = 0U;
            passing_since_valid_ = false;
            passing_since_us_ = 0U;
        }
    }

    samples_[next_] = sample;
    next_ = (next_ + 1U) % configuration_.window_size;
    if (count_ < configuration_.window_size) {
        ++count_;
    }
    recompute();
    if (sample_gap) {
        diagnostics_.stable = false;
        diagnostics_.continuously_passing_duration_us = 0U;
        add_reason(StabilityReason::sample_gap);
    }
    return Status::ok;
}

void StabilityDetector::recompute() noexcept {
    diagnostics_ = StabilityDiagnostics{};
    diagnostics_.sample_count = count_;
    if (!configuration_.configured) {
        diagnostics_.reasons = reason_mask(StabilityReason::not_configured);
        return;
    }
    // StabilityDiagnostics defaults to the safe "not configured" reason for
    // callers that have never configured a detector. Once configuration is
    // known valid, each window must recompute its reasons from a clear mask.
    diagnostics_.reasons = 0U;
    if (count_ == 0U) {
        diagnostics_.reasons = reason_mask(StabilityReason::insufficient_samples);
        passing_since_valid_ = false;
        return;
    }

    const StabilitySample& first = chronological_sample(0U);
    const StabilitySample& last = chronological_sample(count_ - 1U);
    diagnostics_.window_duration_us = last.timestamp_us - first.timestamp_us;
    double mass_sum = 0.0;
    double minimum_mass = std::numeric_limits<double>::infinity();
    double maximum_mass = -std::numeric_limits<double>::infinity();
    bool invalid_measurement = false;
    bool optical_invalid = false;

    for (std::size_t index = 0U; index < count_; ++index) {
        const StabilitySample& sample = chronological_sample(index);
        if (!sample.measurement_valid) {
            invalid_measurement = true;
        } else {
            ++diagnostics_.valid_sample_count;
            mass_sum += sample.mass_mg;
            minimum_mass = std::min(minimum_mass, sample.mass_mg);
            maximum_mass = std::max(maximum_mass, sample.mass_mg);
        }
        if (configuration_.require_optical_diagnostic) {
            if (!sample.optical_valid) {
                optical_invalid = true;
            } else {
                diagnostics_.maximum_absolute_optical_error = std::max(
                    diagnostics_.maximum_absolute_optical_error,
                    std::fabs(sample.optical_position_error));
            }
        }
        diagnostics_.maximum_absolute_coil_current_a = std::max(
            diagnostics_.maximum_absolute_coil_current_a,
            std::fabs(sample.coil_current_a));
        if (sample.power_fault) {
            add_reason(StabilityReason::power_fault);
        }
        if (sample.temperature_fault) {
            add_reason(StabilityReason::temperature_fault);
        }
    }

    if (invalid_measurement) {
        add_reason(StabilityReason::invalid_measurement);
    }
    if (optical_invalid) {
        add_reason(StabilityReason::optical_invalid);
    }
    if (diagnostics_.maximum_absolute_optical_error >
        configuration_.maximum_absolute_optical_error) {
        add_reason(StabilityReason::optical_out_of_bounds);
    }
    if (diagnostics_.maximum_absolute_coil_current_a >=
        configuration_.maximum_absolute_coil_current_a) {
        add_reason(StabilityReason::current_saturated);
    }
    if (diagnostics_.valid_sample_count < configuration_.minimum_valid_samples) {
        add_reason(StabilityReason::insufficient_samples);
    }

    if (diagnostics_.valid_sample_count > 0U) {
        diagnostics_.mean_mg = mass_sum /
                               static_cast<double>(diagnostics_.valid_sample_count);
        diagnostics_.peak_to_peak_mg = maximum_mass - minimum_mass;
        double squared_sum = 0.0;
        double time_sum_s = 0.0;
        double time_squared_sum_s2 = 0.0;
        double time_mass_sum = 0.0;
        for (std::size_t index = 0U; index < count_; ++index) {
            const StabilitySample& sample = chronological_sample(index);
            if (!sample.measurement_valid) {
                continue;
            }
            const double residual = sample.mass_mg - diagnostics_.mean_mg;
            squared_sum += residual * residual;
            const double time_s = static_cast<double>(
                                      sample.timestamp_us - first.timestamp_us) /
                                  1'000'000.0;
            time_sum_s += time_s;
            time_squared_sum_s2 += time_s * time_s;
            time_mass_sum += time_s * sample.mass_mg;
        }
        const double valid_count =
            static_cast<double>(diagnostics_.valid_sample_count);
        diagnostics_.standard_deviation_mg =
            std::sqrt(squared_sum / valid_count);
        const double slope_denominator =
            time_squared_sum_s2 - ((time_sum_s * time_sum_s) / valid_count);
        if (slope_denominator > std::numeric_limits<double>::epsilon()) {
            diagnostics_.slope_mg_per_s =
                (time_mass_sum -
                 ((time_sum_s * mass_sum) / valid_count)) /
                slope_denominator;
        } else if (diagnostics_.valid_sample_count >= 2U) {
            add_reason(StabilityReason::invalid_timestamp);
        }
    }

    if (std::fabs(diagnostics_.slope_mg_per_s) >
        configuration_.maximum_absolute_slope_mg_per_s) {
        add_reason(StabilityReason::excessive_slope);
    }
    if (diagnostics_.standard_deviation_mg >
        configuration_.maximum_standard_deviation_mg) {
        add_reason(StabilityReason::excessive_standard_deviation);
    }
    if (diagnostics_.peak_to_peak_mg >
        configuration_.maximum_peak_to_peak_mg) {
        add_reason(StabilityReason::excessive_peak_to_peak);
    }

    if (diagnostics_.reasons == 0U) {
        if (!passing_since_valid_) {
            passing_since_valid_ = true;
            passing_since_us_ = last.timestamp_us;
        }
        diagnostics_.continuously_passing_duration_us =
            last.timestamp_us - passing_since_us_;
        if (diagnostics_.continuously_passing_duration_us <
            configuration_.minimum_stable_duration_us) {
            add_reason(StabilityReason::insufficient_duration);
        }
    } else {
        passing_since_valid_ = false;
    }
    diagnostics_.stable = diagnostics_.reasons == 0U;
}

}  // namespace gravimetra::measurement
