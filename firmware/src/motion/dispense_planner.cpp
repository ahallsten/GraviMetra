#include "gravimetra/motion/dispense_planner.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gravimetra::motion {

bool DispenseStageConfig::valid() const noexcept {
    if (!enabled) {
        return true;
    }
    return motor_channel < AugerManager::kMaximumAugers &&
           valid_motor_direction(direction) && motor.valid() &&
           start_speed_hz > 0U && maximum_speed_hz >= start_speed_hz &&
           std::isfinite(acceleration_hz_per_second) &&
           acceleration_hz_per_second > 0.0 && minimum_pulse_count > 0U &&
           std::isfinite(transition_error_mg) && transition_error_mg >= 0.0 &&
           std::isfinite(predictive_margin_mg) && predictive_margin_mg >= 0.0 &&
           timeout_us > 0U && std::isfinite(maximum_overshoot_mg) &&
           maximum_overshoot_mg >= 0.0;
}

bool DispensePlannerConfig::valid() const noexcept {
    if (!std::isfinite(maximum_allowed_mass_mg) ||
        maximum_allowed_mass_mg <= 0.0 ||
        maximum_allowed_mass_mg > kFrozenLiveCapacityMg ||
        !std::isfinite(empty_tolerance_mg) || empty_tolerance_mg < 0.0 ||
        !std::isfinite(final_underfill_tolerance_mg) ||
        final_underfill_tolerance_mg < 0.0 ||
        !std::isfinite(final_overfill_tolerance_mg) ||
        final_overfill_tolerance_mg < 0.0 ||
        empty_tolerance_mg > maximum_allowed_mass_mg ||
        final_underfill_tolerance_mg > maximum_allowed_mass_mg ||
        final_overfill_tolerance_mg > maximum_allowed_mass_mg ||
        empty_verification_timeout_us == 0U || settling_timeout_us == 0U ||
        validation_timeout_us == 0U) {
        return false;
    }

    bool found_enabled_stage = false;
    for (const auto& stage : stages) {
        if (!stage.valid()) {
            return false;
        }
        if (stage.enabled) {
            const double stop_margin =
                stage.transition_error_mg + stage.predictive_margin_mg;
            if (!std::isfinite(stop_margin) ||
                stop_margin > maximum_allowed_mass_mg ||
                stage.maximum_overshoot_mg > maximum_allowed_mass_mg) {
                return false;
            }
        }
        found_enabled_stage = found_enabled_stage || stage.enabled;
    }
    return found_enabled_stage;
}

DispensePlanner::DispensePlanner(
    AugerManager& augers,
    const hal::MonotonicClock& clock) noexcept
    : augers_(augers), clock_(clock) {}

Status DispensePlanner::configure(
    const DispensePlannerConfig& config) noexcept {
    if (running()) {
        return Status::busy;
    }
    if (!config.valid()) {
        configured_ = false;
        fault_ = DispenseFault::invalid_configuration;
        return Status::not_configured;
    }
    config_ = config;
    configured_ = true;
    fault_ = DispenseFault::none;
    return Status::ok;
}

Status DispensePlanner::start(const double target_mass_mg) noexcept {
    if (running()) {
        return Status::busy;
    }
    if (!configured_) {
        return Status::not_configured;
    }
    if (!std::isfinite(target_mass_mg) || target_mass_mg <= 0.0 ||
        target_mass_mg > config_.maximum_allowed_mass_mg) {
        return Status::invalid_argument;
    }
    if (!augers_.motion_permitted()) {
        state_ = DispenseState::estop;
        fault_ = DispenseFault::estop_active;
        return Status::fault_active;
    }

    const Status stop_status = augers_.stop_all();
    if (!is_ok(stop_status) || augers_.shutdown_fault_latched()) {
        fail(DispenseFault::motor_driver_fault);
        return is_ok(stop_status) ? Status::fault_active : stop_status;
    }
    target_mass_mg_ = target_mass_mg;
    final_mass_mg_ = 0.0;
    final_error_mg_ = 0.0;
    fault_ = DispenseFault::none;
    active_stage_ = kNoStage;
    settled_stage_ = kNoStage;
    request_profile(MeasurementProfileRequest::precision_settle, true);
    state_ = DispenseState::verify_empty;
    deadline_us_ = deadline_after(
        clock_.now_us(), config_.empty_verification_timeout_us);
    return Status::ok;
}

Status DispensePlanner::update(const DispenseSample& sample) noexcept {
    const Status service_status = augers_.service();
    if (running() &&
        (!is_ok(service_status) || augers_.shutdown_fault_latched())) {
        fail(DispenseFault::motor_driver_fault);
        return is_ok(service_status) ? Status::fault_active : service_status;
    }
    if (running() && !augers_.motion_permitted()) {
        static_cast<void>(augers_.stop_all());
        fault_ = DispenseFault::estop_active;
        state_ = DispenseState::estop;
        active_stage_ = kNoStage;
        request_profile(MeasurementProfileRequest::precision_settle);
        return Status::fault_active;
    }
    if (running() && configured_) {
        // Capacity protection is independent for the responsive and settled
        // paths. A validity failure in one path must not mask a finite,
        // over-capacity observation from the other path.
        const bool responsive_over_capacity =
            std::isfinite(sample.responsive_mass_mg) &&
            sample.responsive_mass_mg > config_.maximum_allowed_mass_mg;
        const bool settled_over_capacity =
            std::isfinite(sample.settled_mass_mg) &&
            sample.settled_mass_mg > config_.maximum_allowed_mass_mg;
        if (responsive_over_capacity || settled_over_capacity) {
            const double mass = responsive_over_capacity && settled_over_capacity
                                    ? std::max(
                                          sample.responsive_mass_mg,
                                          sample.settled_mass_mg)
                                    : (responsive_over_capacity
                                           ? sample.responsive_mass_mg
                                           : sample.settled_mass_mg);
            finish_capacity_exceeded(mass);
            return Status::fault_active;
        }
    }

    switch (state_) {
        case DispenseState::verify_empty:
            return update_verify_empty(sample);
        case DispenseState::dispense_stage_1:
        case DispenseState::dispense_stage_2:
        case DispenseState::dispense_stage_3:
        case DispenseState::dispense_stage_4:
            return update_stage(sample);
        case DispenseState::settle:
            return update_settle(sample);
        case DispenseState::validate:
            return update_validate(sample);
        case DispenseState::idle:
        case DispenseState::complete:
            return Status::ok;
        case DispenseState::underfill:
        case DispenseState::overfill:
        case DispenseState::fault:
        case DispenseState::estop:
            return Status::fault_active;
    }
    fail(DispenseFault::invalid_configuration);
    return Status::fault_active;
}

Status DispensePlanner::update_verify_empty(
    const DispenseSample& sample) noexcept {
    if (!accept_requested_profile(sample)) {
        if (deadline_expired()) {
            fail(DispenseFault::tare_timeout);
            return Status::timeout;
        }
        return Status::busy;
    }
    if (sample.valid &&
        (!std::isfinite(sample.responsive_mass_mg) ||
         !std::isfinite(sample.settled_mass_mg))) {
        fail(DispenseFault::measurement_invalid);
        return Status::invalid_argument;
    }
    if (sample.valid && sample.stable &&
        std::abs(sample.settled_mass_mg) <= config_.empty_tolerance_mg) {
        const std::uint8_t stage = first_enabled_stage();
        if (stage == kNoStage) {
            fail(DispenseFault::invalid_configuration);
            return Status::not_configured;
        }
        return enter_stage(stage);
    }
    if (deadline_expired()) {
        fail(DispenseFault::tare_timeout);
        return Status::timeout;
    }
    return Status::busy;
}

Status DispensePlanner::update_stage(const DispenseSample& sample) noexcept {
    if (active_stage_ == kNoStage ||
        static_cast<std::size_t>(active_stage_) >= config_.stages.size()) {
        fail(DispenseFault::invalid_configuration);
        return Status::not_configured;
    }
    const auto& stage = config_.stages[active_stage_];

    MotorDriverStatus driver_status{};
    const Status driver_result =
        augers_.read_driver_status(stage.motor_channel, driver_status);
    if (!is_ok(driver_result) || driver_status.critical_fault()) {
        fail(DispenseFault::motor_driver_fault);
        return is_ok(driver_result) ? Status::fault_active : driver_result;
    }
    if (!accept_requested_profile(sample)) {
        if (profile_acknowledged_ || augers_.any_active()) {
            fail(DispenseFault::measurement_invalid);
            return Status::verification_failed;
        }
        if (deadline_expired()) {
            fail(DispenseFault::motion_timeout);
            return Status::timeout;
        }
        return Status::busy;
    }
    if (!sample.valid || !std::isfinite(sample.responsive_mass_mg)) {
        fail(DispenseFault::measurement_invalid);
        return Status::invalid_argument;
    }

    final_mass_mg_ = sample.responsive_mass_mg;
    final_error_mg_ = target_mass_mg_ - final_mass_mg_;
    if (final_mass_mg_ > target_mass_mg_ + stage.maximum_overshoot_mg) {
        finish_overfill(final_mass_mg_);
        return Status::fault_active;
    }
    if (deadline_expired()) {
        fail(DispenseFault::motion_timeout);
        return Status::timeout;
    }

    const double stop_margin =
        stage.transition_error_mg + stage.predictive_margin_mg;
    if (final_error_mg_ <= stop_margin) {
        const Status stop_result = augers_.stop(stage.motor_channel);
        if (!is_ok(stop_result)) {
            fail(DispenseFault::motor_driver_fault);
            return stop_result;
        }
        return enter_settle();
    }

    if (augers_.any_active()) {
        if (augers_.active_channel() != stage.motor_channel) {
            fail(DispenseFault::interlock_violation);
            return Status::interlock_violation;
        }
        return Status::busy;
    }

    const std::uint64_t now = clock_.now_us();
    const std::uint64_t elapsed_us = now - last_speed_update_us_;
    const double elapsed_seconds =
        static_cast<double>(elapsed_us) / 1'000'000.0;
    current_speed_hz_ = std::min(
        static_cast<double>(stage.maximum_speed_hz),
        current_speed_hz_ +
            stage.acceleration_hz_per_second * elapsed_seconds);
    last_speed_update_us_ = now;
    const double rounded_frequency = current_speed_hz_ + 0.5;
    const auto frequency = rounded_frequency >=
                                   static_cast<double>(
                                       std::numeric_limits<std::uint32_t>::max())
                               ? std::numeric_limits<std::uint32_t>::max()
                               : static_cast<std::uint32_t>(rounded_frequency);

    const Status start_result = augers_.start_pulses(
        stage.motor_channel,
        stage.direction,
        frequency,
        stage.minimum_pulse_count);
    if (start_result == Status::busy) {
        return Status::busy;
    }
    if (start_result == Status::interlock_violation) {
        fail(DispenseFault::interlock_violation);
        return start_result;
    }
    if (start_result == Status::fault_active) {
        fault_ = DispenseFault::estop_active;
        state_ = DispenseState::estop;
        active_stage_ = kNoStage;
        request_profile(MeasurementProfileRequest::precision_settle);
        return start_result;
    }
    if (!is_ok(start_result)) {
        fail(DispenseFault::motor_driver_fault);
        return start_result;
    }
    return Status::ok;
}

Status DispensePlanner::update_settle(const DispenseSample& sample) noexcept {
    if (augers_.any_active()) {
        fail(DispenseFault::interlock_violation);
        return Status::interlock_violation;
    }
    if (!accept_requested_profile(sample)) {
        if (deadline_expired()) {
            fail(DispenseFault::stability_timeout);
            return Status::timeout;
        }
        return Status::busy;
    }
    if (sample.valid && !std::isfinite(sample.settled_mass_mg)) {
        fail(DispenseFault::measurement_invalid);
        return Status::invalid_argument;
    }
    if (!(sample.valid && sample.stable)) {
        if (deadline_expired()) {
            fail(DispenseFault::stability_timeout);
            return Status::timeout;
        }
        return Status::busy;
    }

    final_mass_mg_ = sample.settled_mass_mg;
    final_error_mg_ = target_mass_mg_ - final_mass_mg_;
    if (settled_stage_ == kNoStage ||
        static_cast<std::size_t>(settled_stage_) >= config_.stages.size()) {
        fail(DispenseFault::invalid_configuration);
        return Status::not_configured;
    }
    const auto& completed_stage = config_.stages[settled_stage_];
    if (final_mass_mg_ >
        target_mass_mg_ + completed_stage.maximum_overshoot_mg) {
        finish_overfill(final_mass_mg_);
        return Status::fault_active;
    }

    const std::uint8_t next = next_enabled_stage(settled_stage_);
    if (final_error_mg_ > config_.final_underfill_tolerance_mg &&
        next != kNoStage) {
        return enter_stage(next);
    }
    return enter_validate();
}

Status DispensePlanner::update_validate(const DispenseSample& sample) noexcept {
    if (!accept_requested_profile(sample)) {
        if (deadline_expired()) {
            fail(DispenseFault::stability_timeout);
            return Status::timeout;
        }
        return Status::busy;
    }
    if (sample.valid && !std::isfinite(sample.settled_mass_mg)) {
        fail(DispenseFault::measurement_invalid);
        return Status::invalid_argument;
    }
    if (!(sample.valid && sample.stable)) {
        if (deadline_expired()) {
            fail(DispenseFault::stability_timeout);
            return Status::timeout;
        }
        return Status::busy;
    }

    final_mass_mg_ = sample.settled_mass_mg;
    final_error_mg_ = target_mass_mg_ - final_mass_mg_;
    if (final_error_mg_ > config_.final_underfill_tolerance_mg) {
        finish_underfill(final_mass_mg_);
        return Status::fault_active;
    }
    if (-final_error_mg_ > config_.final_overfill_tolerance_mg) {
        finish_overfill(final_mass_mg_);
        return Status::fault_active;
    }

    state_ = DispenseState::complete;
    active_stage_ = kNoStage;
    fault_ = DispenseFault::none;
    request_profile(MeasurementProfileRequest::precision_settle);
    return Status::ok;
}

Status DispensePlanner::enter_stage(const std::uint8_t stage_index) noexcept {
    if (static_cast<std::size_t>(stage_index) >= config_.stages.size() ||
        !config_.stages[stage_index].enabled) {
        fail(DispenseFault::invalid_configuration);
        return Status::not_configured;
    }
    const auto& stage = config_.stages[stage_index];
    const Status config_status =
        augers_.configure_motor(stage.motor_channel, stage.motor);
    if (!is_ok(config_status)) {
        fail(DispenseFault::motor_driver_fault);
        return config_status;
    }

    active_stage_ = stage_index;
    settled_stage_ = kNoStage;
    state_ = state_for_stage(stage_index);
    request_profile(MeasurementProfileRequest::active_dispense);
    deadline_us_ = deadline_after(clock_.now_us(), stage.timeout_us);
    last_speed_update_us_ = clock_.now_us();
    current_speed_hz_ = static_cast<double>(stage.start_speed_hz);
    return Status::ok;
}

Status DispensePlanner::enter_settle() noexcept {
    const Status stop_status = augers_.stop_all();
    if (!is_ok(stop_status)) {
        fail(DispenseFault::motor_driver_fault);
        return stop_status;
    }
    settled_stage_ = active_stage_;
    active_stage_ = kNoStage;
    state_ = DispenseState::settle;
    request_profile(MeasurementProfileRequest::precision_settle);
    deadline_us_ = deadline_after(clock_.now_us(), config_.settling_timeout_us);
    return Status::ok;
}

Status DispensePlanner::enter_validate() noexcept {
    const Status stop_status = augers_.stop_all();
    if (!is_ok(stop_status)) {
        fail(DispenseFault::motor_driver_fault);
        return stop_status;
    }
    active_stage_ = kNoStage;
    state_ = DispenseState::validate;
    request_profile(MeasurementProfileRequest::precision_settle);
    deadline_us_ = deadline_after(clock_.now_us(), config_.validation_timeout_us);
    return Status::ok;
}

void DispensePlanner::finish_underfill(const double mass_mg) noexcept {
    static_cast<void>(augers_.stop_all());
    final_mass_mg_ = mass_mg;
    final_error_mg_ = target_mass_mg_ - mass_mg;
    state_ = DispenseState::underfill;
    active_stage_ = kNoStage;
    request_profile(MeasurementProfileRequest::precision_settle);
}

void DispensePlanner::finish_overfill(const double mass_mg) noexcept {
    static_cast<void>(augers_.stop_all());
    final_mass_mg_ = mass_mg;
    final_error_mg_ = target_mass_mg_ - mass_mg;
    state_ = DispenseState::overfill;
    active_stage_ = kNoStage;
    request_profile(MeasurementProfileRequest::precision_settle);
}

void DispensePlanner::finish_capacity_exceeded(const double mass_mg) noexcept {
    static_cast<void>(augers_.stop_all());
    final_mass_mg_ = mass_mg;
    final_error_mg_ = target_mass_mg_ - mass_mg;
    fault_ = DispenseFault::capacity_exceeded;
    state_ = DispenseState::overfill;
    active_stage_ = kNoStage;
    request_profile(MeasurementProfileRequest::precision_settle);
}

void DispensePlanner::fail(const DispenseFault fault) noexcept {
    static_cast<void>(augers_.stop_all());
    fault_ = fault;
    state_ = DispenseState::fault;
    active_stage_ = kNoStage;
    request_profile(MeasurementProfileRequest::precision_settle);
}

bool DispensePlanner::accept_requested_profile(
    const DispenseSample& sample) noexcept {
    if (sample.profile != requested_profile_ ||
        sample.profile_generation != requested_profile_generation_ ||
        !sample.profile_settled) {
        return false;
    }
    profile_acknowledged_ = true;
    return true;
}

void DispensePlanner::request_profile(
    const MeasurementProfileRequest profile,
    const bool force_new_generation) noexcept {
    if (!force_new_generation && profile == requested_profile_) {
        return;
    }
    requested_profile_ = profile;
    if (requested_profile_generation_ !=
        std::numeric_limits<std::uint64_t>::max()) {
        ++requested_profile_generation_;
    }
    profile_acknowledged_ = false;
}

std::uint8_t DispensePlanner::first_enabled_stage() const noexcept {
    return next_enabled_stage(kNoStage);
}

std::uint8_t DispensePlanner::next_enabled_stage(
    const std::uint8_t after_stage) const noexcept {
    const std::size_t beginning = after_stage == kNoStage
                                      ? 0U
                                      : static_cast<std::size_t>(after_stage) + 1U;
    for (std::size_t index = beginning; index < config_.stages.size(); ++index) {
        if (config_.stages[index].enabled) {
            return static_cast<std::uint8_t>(index);
        }
    }
    return kNoStage;
}

DispenseState DispensePlanner::state_for_stage(
    const std::uint8_t stage) noexcept {
    switch (stage) {
        case 0U:
            return DispenseState::dispense_stage_1;
        case 1U:
            return DispenseState::dispense_stage_2;
        case 2U:
            return DispenseState::dispense_stage_3;
        case 3U:
            return DispenseState::dispense_stage_4;
        default:
            return DispenseState::fault;
    }
}

std::uint64_t DispensePlanner::deadline_after(
    const std::uint64_t now,
    const std::uint64_t duration) noexcept {
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    if (duration > maximum - now) {
        return maximum;
    }
    return now + duration;
}

bool DispensePlanner::deadline_expired() const noexcept {
    return clock_.now_us() >= deadline_us_;
}

bool DispensePlanner::running() const noexcept {
    switch (state_) {
        case DispenseState::verify_empty:
        case DispenseState::dispense_stage_1:
        case DispenseState::dispense_stage_2:
        case DispenseState::dispense_stage_3:
        case DispenseState::dispense_stage_4:
        case DispenseState::settle:
        case DispenseState::validate:
            return true;
        case DispenseState::idle:
        case DispenseState::complete:
        case DispenseState::underfill:
        case DispenseState::overfill:
        case DispenseState::fault:
        case DispenseState::estop:
            return false;
    }
    return false;
}

Status DispensePlanner::abort() noexcept {
    const Status stop_status = augers_.stop_all();
    active_stage_ = kNoStage;
    settled_stage_ = kNoStage;
    request_profile(MeasurementProfileRequest::precision_settle);
    if (!is_ok(stop_status) || augers_.shutdown_fault_latched()) {
        state_ = DispenseState::fault;
        fault_ = DispenseFault::motor_driver_fault;
        return is_ok(stop_status) ? Status::fault_active : stop_status;
    }
    state_ = DispenseState::idle;
    fault_ = DispenseFault::none;
    return Status::ok;
}

}  // namespace gravimetra::motion
