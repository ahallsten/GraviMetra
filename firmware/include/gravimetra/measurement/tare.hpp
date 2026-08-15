#pragma once

#include "gravimetra/common/status.hpp"

#include <cstdint>

namespace gravimetra::measurement {

struct TarePolicy {
    bool configured{false};
    double maximum_absolute_candidate_mg{0.0};
    double maximum_candidate_stddev_mg{0.0};
};

struct TareCandidate {
    double corrected_mass_mg{0.0};
    double standard_deviation_mg{0.0};
    bool stable{false};
    bool fault_free{false};
    std::uint64_t timestamp_us{0U};
};

struct TareState {
    bool valid{false};
    double offset_mg{0.0};
    std::uint64_t accepted_at_us{0U};
};

[[nodiscard]] Status accept_tare(
    const TarePolicy& policy,
    const TareCandidate& candidate,
    TareState& state) noexcept;

}  // namespace gravimetra::measurement
