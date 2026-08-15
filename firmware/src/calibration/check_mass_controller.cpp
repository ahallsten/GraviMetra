#include "gravimetra/calibration/check_mass_controller.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace gravimetra::calibration {

bool CheckMassConfig::valid(const bool span_sink_available) const noexcept {
    if (!std::isfinite(certified_mass_mg) || certified_mass_mg <= 0.0 ||
        !std::isfinite(check_tolerance_mg) || check_tolerance_mg < 0.0 ||
        check_tolerance_mg >= certified_mass_mg ||
        !std::isfinite(empty_tolerance_mg) || empty_tolerance_mg < 0.0 ||
        !std::isfinite(zero_return_tolerance_mg) ||
        zero_return_tolerance_mg < 0.0 ||
        empty_stability_timeout_us == 0U || actuator_timeout_us == 0U ||
        loaded_stability_timeout_us == 0U || zero_return_timeout_us == 0U) {
        return false;
    }
    if (!allow_automatic_span_adjustment) {
        return true;
    }
    return span_sink_available &&
           std::isfinite(maximum_relative_span_adjustment) &&
           maximum_relative_span_adjustment > 0.0 &&
           maximum_relative_span_adjustment <
               kMaximumSafeRelativeSpanAdjustment;
}

CheckMassController::CheckMassController(
    CheckMassActuator& actuator,
    hal::DigitalInput& home_sensor,
    hal::DigitalInput& applied_sensor,
    const hal::MonotonicClock& clock,
    SpanCorrectionSink* const span_sink) noexcept
    : actuator_(actuator),
      home_sensor_(home_sensor),
      applied_sensor_(applied_sensor),
      clock_(clock),
      span_sink_(span_sink) {
    static_cast<void>(disable_actuator());
}

Status CheckMassController::configure(const CheckMassConfig& config) noexcept {
    if (running()) {
        return Status::busy;
    }
    if (!config.valid(span_sink_ != nullptr)) {
        configured_ = false;
        return Status::not_configured;
    }
    config_ = config;
    configured_ = true;
    return Status::ok;
}

Status CheckMassController::start() noexcept {
    if (running()) {
        return Status::busy;
    }
    if (!configured_) {
        return Status::not_configured;
    }
    if (!safety_permitted_) {
        return Status::fault_active;
    }
    if (actuator_disable_fault_latched_) {
        return Status::fault_active;
    }

    const Status disable_status = disable_actuator();
    if (!is_ok(disable_status)) {
        result_ = {};
        result_.outcome = CheckMassOutcome::actuator_io_error;
        state_ = CheckMassState::failed;
        return disable_status;
    }
    result_ = {};
    check_within_tolerance_ = false;
    pending_span_factor_ = 1.0;
    request_fresh_precision_profile();
    state_ = CheckMassState::verify_empty;
    deadline_us_ = deadline_after(
        clock_.now_us(), config_.empty_stability_timeout_us);
    return Status::ok;
}

Status CheckMassController::update(const CheckMassSample& sample) noexcept {
    if (running() && !safety_permitted_) {
        const Status inhibit_status = inhibit_for_safety();
        return is_ok(inhibit_status) ? Status::fault_active : inhibit_status;
    }
    switch (state_) {
        case CheckMassState::verify_empty:
            return update_verify_empty(sample);
        case CheckMassState::applying:
            return update_applying();
        case CheckMassState::settling_with_mass:
            return update_settling(sample);
        case CheckMassState::removing:
            return update_removing();
        case CheckMassState::verify_zero_return:
            return update_verify_zero(sample);
        case CheckMassState::idle:
        case CheckMassState::complete:
            return Status::ok;
        case CheckMassState::failed:
        case CheckMassState::estop_inhibited:
            return Status::fault_active;
    }
    fail(CheckMassOutcome::actuator_sensor_fault);
    return Status::fault_active;
}

Status CheckMassController::update_verify_empty(
    const CheckMassSample& sample) noexcept {
    bool home = false;
    bool applied = false;
    Status status = read_sensors(home, applied);
    if (!is_ok(status)) {
        fail(CheckMassOutcome::actuator_sensor_fault);
        return status;
    }
    if (home && applied) {
        fail(CheckMassOutcome::actuator_sensor_fault);
        return Status::verification_failed;
    }
    if (!accepts_precision_sample(sample)) {
        if (deadline_expired()) {
            fail(CheckMassOutcome::empty_not_stable);
            return Status::timeout;
        }
        return Status::busy;
    }
    if (sample.valid && !std::isfinite(sample.mass_mg)) {
        fail(CheckMassOutcome::empty_not_stable);
        return Status::invalid_argument;
    }
    if (home && !applied && sample.valid && sample.stable &&
        std::abs(sample.mass_mg) <= config_.empty_tolerance_mg) {
        status = actuator_.apply();
        if (!is_ok(status)) {
            fail(CheckMassOutcome::actuator_io_error);
            return status;
        }
        state_ = CheckMassState::applying;
        deadline_us_ =
            deadline_after(clock_.now_us(), config_.actuator_timeout_us);
        return Status::ok;
    }
    if (deadline_expired()) {
        fail(CheckMassOutcome::empty_not_stable);
        return Status::timeout;
    }
    return Status::busy;
}

Status CheckMassController::update_applying() noexcept {
    bool home = false;
    bool applied = false;
    const Status status = read_sensors(home, applied);
    if (!is_ok(status)) {
        fail(CheckMassOutcome::actuator_sensor_fault);
        return status;
    }
    if (home && applied) {
        fail(CheckMassOutcome::actuator_sensor_fault);
        return Status::verification_failed;
    }
    if (!home && applied) {
        // The actuator is electrically quiet and mechanically unloaded while
        // the internal mass rests on the scale load path.
        const Status disable_status = disable_actuator();
        if (!is_ok(disable_status)) {
            fail(CheckMassOutcome::actuator_io_error);
            return disable_status;
        }
        request_fresh_precision_profile();
        state_ = CheckMassState::settling_with_mass;
        deadline_us_ = deadline_after(
            clock_.now_us(), config_.loaded_stability_timeout_us);
        return Status::ok;
    }
    if (deadline_expired()) {
        fail(CheckMassOutcome::actuator_timeout);
        return Status::timeout;
    }
    return Status::busy;
}

Status CheckMassController::update_settling(
    const CheckMassSample& sample) noexcept {
    bool home = false;
    bool applied = false;
    Status status = read_sensors(home, applied);
    if (!is_ok(status) || home || !applied) {
        fail(CheckMassOutcome::actuator_sensor_fault);
        return is_ok(status) ? Status::verification_failed : status;
    }
    if (!accepts_precision_sample(sample)) {
        if (deadline_expired()) {
            fail(CheckMassOutcome::loaded_stability_timeout);
            return Status::timeout;
        }
        return Status::busy;
    }
    if (sample.valid &&
        (!std::isfinite(sample.mass_mg) || sample.mass_mg <= 0.0)) {
        fail(CheckMassOutcome::check_failed);
        return Status::invalid_argument;
    }
    if (!(sample.valid && sample.stable)) {
        if (deadline_expired()) {
            fail(CheckMassOutcome::loaded_stability_timeout);
            return Status::timeout;
        }
        return Status::busy;
    }

    result_.measured_mass_mg = sample.mass_mg;
    result_.error_mg = sample.mass_mg - config_.certified_mass_mg;
    check_within_tolerance_ =
        std::abs(result_.error_mg) <= config_.check_tolerance_mg;
    // A passing loaded reading is provisional until removal and stable zero
    // return succeed. Do not publish success while the sequence is running.
    result_.outcome = check_within_tolerance_ ? CheckMassOutcome::none
                                              : CheckMassOutcome::check_failed;

    if (check_within_tolerance_ &&
        config_.allow_automatic_span_adjustment) {
        const double factor = config_.certified_mass_mg / sample.mass_mg;
        if (!std::isfinite(factor) || factor <= 0.0 ||
            std::abs(factor - 1.0) >
                config_.maximum_relative_span_adjustment) {
            check_within_tolerance_ = false;
            result_.outcome = CheckMassOutcome::span_adjustment_rejected;
        } else {
            // Defer persistence until removal and zero return both succeed.
            // A later failure must not alter calibration from a failed check.
            pending_span_factor_ = factor;
        }
    }

    return begin_removal();
}

Status CheckMassController::begin_removal() noexcept {
    const Status status = actuator_.remove();
    if (!is_ok(status)) {
        fail(CheckMassOutcome::actuator_io_error);
        return status;
    }
    state_ = CheckMassState::removing;
    deadline_us_ = deadline_after(clock_.now_us(), config_.actuator_timeout_us);
    return Status::ok;
}

Status CheckMassController::update_removing() noexcept {
    bool home = false;
    bool applied = false;
    const Status status = read_sensors(home, applied);
    if (!is_ok(status)) {
        fail(CheckMassOutcome::actuator_sensor_fault);
        return status;
    }
    if (home && applied) {
        fail(CheckMassOutcome::actuator_sensor_fault);
        return Status::verification_failed;
    }
    if (home && !applied) {
        const Status disable_status = disable_actuator();
        if (!is_ok(disable_status)) {
            fail(CheckMassOutcome::actuator_io_error);
            return disable_status;
        }
        request_fresh_precision_profile();
        state_ = CheckMassState::verify_zero_return;
        deadline_us_ =
            deadline_after(clock_.now_us(), config_.zero_return_timeout_us);
        return Status::ok;
    }
    if (deadline_expired()) {
        fail(CheckMassOutcome::actuator_timeout);
        return Status::timeout;
    }
    return Status::busy;
}

Status CheckMassController::update_verify_zero(
    const CheckMassSample& sample) noexcept {
    bool home = false;
    bool applied = false;
    Status status = read_sensors(home, applied);
    if (!is_ok(status) || !home || applied) {
        fail(CheckMassOutcome::actuator_sensor_fault);
        return is_ok(status) ? Status::verification_failed : status;
    }
    if (!accepts_precision_sample(sample)) {
        if (deadline_expired()) {
            fail(CheckMassOutcome::zero_return_failed);
            return Status::timeout;
        }
        return Status::busy;
    }
    if (sample.valid && !std::isfinite(sample.mass_mg)) {
        fail(CheckMassOutcome::zero_return_failed);
        return Status::invalid_argument;
    }
    if (sample.valid && sample.stable &&
        std::abs(sample.mass_mg) <= config_.zero_return_tolerance_mg) {
        if (!check_within_tolerance_) {
            state_ = CheckMassState::failed;
            return Status::fault_active;
        }
        if (config_.allow_automatic_span_adjustment) {
            status = span_sink_->apply_check_span_factor(pending_span_factor_);
            if (!is_ok(status)) {
                fail(CheckMassOutcome::span_adjustment_io_error);
                return status;
            }
            result_.applied_span_factor = pending_span_factor_;
            result_.span_adjustment_applied = true;
        }
        result_.outcome = CheckMassOutcome::success;
        state_ = CheckMassState::complete;
        return Status::ok;
    }
    if (deadline_expired()) {
        fail(CheckMassOutcome::zero_return_failed);
        return Status::timeout;
    }
    return Status::busy;
}

Status CheckMassController::read_sensors(
    bool& home,
    bool& applied) noexcept {
    Status status = home_sensor_.read(home);
    if (!is_ok(status)) {
        return status;
    }
    status = applied_sensor_.read(applied);
    return status;
}

bool CheckMassController::accepts_precision_sample(
    const CheckMassSample& sample) const noexcept {
    return sample.precision_profile_ready &&
           sample.precision_profile_epoch ==
               required_precision_profile_epoch_;
}

void CheckMassController::request_fresh_precision_profile() noexcept {
    ++required_precision_profile_epoch_;
    // Reserve zero as the never-requested/default sample epoch. Unsigned
    // wrap is defined; skipping zero avoids accepting a default-constructed
    // sample even after the physically unreachable 2^64 request horizon.
    if (required_precision_profile_epoch_ == 0U) {
        ++required_precision_profile_epoch_;
    }
}

Status CheckMassController::disable_actuator() noexcept {
    const Status status = actuator_.disable();
    if (!is_ok(status)) {
        actuator_disable_fault_latched_ = true;
        last_actuator_disable_status_ = status;
    }
    return status;
}

void CheckMassController::fail(const CheckMassOutcome outcome) noexcept {
    const Status disable_status = disable_actuator();
    result_.outcome =
        (!is_ok(disable_status) || actuator_disable_fault_latched_)
            ? CheckMassOutcome::actuator_io_error
            : outcome;
    state_ = CheckMassState::failed;
}

void CheckMassController::cancel() noexcept {
    if (!running()) {
        return;
    }
    const Status disable_status = disable_actuator();
    result_.outcome =
        (!is_ok(disable_status) || actuator_disable_fault_latched_)
            ? CheckMassOutcome::actuator_io_error
            : CheckMassOutcome::cancelled;
    state_ = CheckMassState::failed;
}

Status CheckMassController::inhibit_for_safety() noexcept {
    safety_permitted_ = false;
    const Status disable_status = disable_actuator();
    if (running()) {
        result_.outcome = CheckMassOutcome::estop_interrupted;
        state_ = CheckMassState::estop_inhibited;
    }
    if (!is_ok(disable_status)) {
        return disable_status;
    }
    return actuator_disable_fault_latched_ ? Status::fault_active
                                           : Status::ok;
}

Status CheckMassController::release_safety_inhibit() noexcept {
    if (actuator_disable_fault_latched_) {
        safety_permitted_ = false;
        return Status::fault_active;
    }
    safety_permitted_ = true;
    return Status::ok;
}

Status CheckMassController::clear_actuator_disable_fault() noexcept {
    if (running()) {
        return Status::busy;
    }
    safety_permitted_ = false;
    const Status status = actuator_.disable();
    if (!is_ok(status)) {
        actuator_disable_fault_latched_ = true;
        last_actuator_disable_status_ = status;
        return status;
    }
    actuator_disable_fault_latched_ = false;
    last_actuator_disable_status_ = Status::ok;
    return Status::ok;
}

bool CheckMassController::running() const noexcept {
    switch (state_) {
        case CheckMassState::verify_empty:
        case CheckMassState::applying:
        case CheckMassState::settling_with_mass:
        case CheckMassState::removing:
        case CheckMassState::verify_zero_return:
            return true;
        case CheckMassState::idle:
        case CheckMassState::complete:
        case CheckMassState::failed:
        case CheckMassState::estop_inhibited:
            return false;
    }
    return false;
}

bool CheckMassController::deadline_expired() const noexcept {
    return clock_.now_us() >= deadline_us_;
}

std::uint64_t CheckMassController::deadline_after(
    const std::uint64_t now,
    const std::uint64_t duration) noexcept {
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    if (duration > maximum - now) {
        return maximum;
    }
    return now + duration;
}

}  // namespace gravimetra::calibration
