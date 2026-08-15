#include "gravimetra/measurement/tare.hpp"

#include <cmath>

namespace gravimetra::measurement {

Status accept_tare(
    const TarePolicy& policy,
    const TareCandidate& candidate,
    TareState& state) noexcept {
    if (!policy.configured) {
        return Status::not_configured;
    }
    if (!std::isfinite(policy.maximum_absolute_candidate_mg) ||
        policy.maximum_absolute_candidate_mg < 0.0 ||
        !std::isfinite(policy.maximum_candidate_stddev_mg) ||
        policy.maximum_candidate_stddev_mg < 0.0 ||
        !std::isfinite(candidate.corrected_mass_mg) ||
        !std::isfinite(candidate.standard_deviation_mg) ||
        candidate.standard_deviation_mg < 0.0) {
        return Status::invalid_argument;
    }
    if (!candidate.stable || !candidate.fault_free ||
        std::fabs(candidate.corrected_mass_mg) >
            policy.maximum_absolute_candidate_mg ||
        candidate.standard_deviation_mg >
            policy.maximum_candidate_stddev_mg) {
        return Status::verification_failed;
    }
    if (state.valid && candidate.timestamp_us <= state.accepted_at_us) {
        return Status::verification_failed;
    }
    state.valid = true;
    state.offset_mg = candidate.corrected_mass_mg;
    state.accepted_at_us = candidate.timestamp_us;
    return Status::ok;
}

}  // namespace gravimetra::measurement
