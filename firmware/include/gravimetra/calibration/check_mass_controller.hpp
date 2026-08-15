#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"
#include "gravimetra/safety/estop_manager.hpp"

#include <cstdint>

namespace gravimetra::calibration {

// Exclusive mathematical safety ceiling. Operator/metrology policy must
// configure a much narrower value when automatic span correction is allowed.
inline constexpr double kMaximumSafeRelativeSpanAdjustment = 1.0;

class CheckMassActuator {
public:
    virtual ~CheckMassActuator() = default;
    [[nodiscard]] virtual Status apply() noexcept = 0;
    [[nodiscard]] virtual Status remove() noexcept = 0;
    [[nodiscard]] virtual Status disable() noexcept = 0;
};

class SpanCorrectionSink {
public:
    virtual ~SpanCorrectionSink() = default;
    [[nodiscard]] virtual Status apply_check_span_factor(
        double multiplicative_factor) noexcept = 0;
};

enum class CheckMassState : std::uint8_t {
    idle = 0,
    verify_empty,
    applying,
    settling_with_mass,
    removing,
    verify_zero_return,
    complete,
    failed,
    estop_inhibited,
};

enum class CheckMassOutcome : std::uint8_t {
    none = 0,
    success,
    empty_not_stable,
    actuator_timeout,
    actuator_sensor_fault,
    loaded_stability_timeout,
    check_failed,
    span_adjustment_rejected,
    span_adjustment_io_error,
    zero_return_failed,
    actuator_io_error,
    estop_interrupted,
    cancelled,
};

struct CheckMassSample {
    bool valid{false};
    bool stable{false};
    // The caller supplies the tared, compensated mass from the precision
    // measurement path; the controller never fabricates or recalibrates it.
    double mass_mg{0.0};
    // The acquisition coordinator must acknowledge that the precision
    // profile is applied and settled for the controller's current request
    // epoch. Stale active-dispense or prior-phase data cannot verify a check.
    std::uint64_t precision_profile_epoch{0U};
    bool precision_profile_ready{false};
};

struct CheckMassConfig {
    double certified_mass_mg{0.0};
    double check_tolerance_mg{0.0};
    double empty_tolerance_mg{0.0};
    double zero_return_tolerance_mg{0.0};
    std::uint64_t empty_stability_timeout_us{0U};
    std::uint64_t actuator_timeout_us{0U};
    std::uint64_t loaded_stability_timeout_us{0U};
    std::uint64_t zero_return_timeout_us{0U};
    bool allow_automatic_span_adjustment{false};
    double maximum_relative_span_adjustment{0.0};

    [[nodiscard]] bool valid(bool span_sink_available) const noexcept;
};

struct CheckMassResult {
    CheckMassOutcome outcome{CheckMassOutcome::none};
    double measured_mass_mg{0.0};
    double error_mg{0.0};
    double applied_span_factor{1.0};
    bool span_adjustment_applied{false};
};

class CheckMassController final : public safety::SafetyInhibitTarget {
public:
    CheckMassController(
        CheckMassActuator& actuator,
        hal::DigitalInput& home_sensor,
        hal::DigitalInput& applied_sensor,
        const hal::MonotonicClock& clock,
        SpanCorrectionSink* span_sink = nullptr) noexcept;

    [[nodiscard]] Status configure(const CheckMassConfig& config) noexcept;
    [[nodiscard]] Status start() noexcept;
    [[nodiscard]] Status update(const CheckMassSample& sample) noexcept;
    void cancel() noexcept;

    [[nodiscard]] Status inhibit_for_safety() noexcept override;
    [[nodiscard]] Status release_safety_inhibit() noexcept override;
    [[nodiscard]] Status clear_actuator_disable_fault() noexcept;

    [[nodiscard]] CheckMassState state() const noexcept {
        return state_;
    }
    [[nodiscard]] const CheckMassResult& result() const noexcept {
        return result_;
    }
    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] bool safety_permitted() const noexcept {
        return safety_permitted_;
    }
    [[nodiscard]] bool actuator_disable_fault_latched() const noexcept {
        return actuator_disable_fault_latched_;
    }
    [[nodiscard]] Status last_actuator_disable_status() const noexcept {
        return last_actuator_disable_status_;
    }
    [[nodiscard]] std::uint64_t required_precision_profile_epoch()
        const noexcept {
        return required_precision_profile_epoch_;
    }

private:
    [[nodiscard]] Status update_verify_empty(
        const CheckMassSample& sample) noexcept;
    [[nodiscard]] Status update_applying() noexcept;
    [[nodiscard]] Status update_settling(
        const CheckMassSample& sample) noexcept;
    [[nodiscard]] Status update_removing() noexcept;
    [[nodiscard]] Status update_verify_zero(
        const CheckMassSample& sample) noexcept;
    [[nodiscard]] Status read_sensors(bool& home, bool& applied) noexcept;
    [[nodiscard]] Status begin_removal() noexcept;
    [[nodiscard]] bool accepts_precision_sample(
        const CheckMassSample& sample) const noexcept;
    void request_fresh_precision_profile() noexcept;
    [[nodiscard]] Status disable_actuator() noexcept;
    void fail(CheckMassOutcome outcome) noexcept;
    [[nodiscard]] bool deadline_expired() const noexcept;
    [[nodiscard]] static std::uint64_t deadline_after(
        std::uint64_t now,
        std::uint64_t duration) noexcept;

    CheckMassActuator& actuator_;
    hal::DigitalInput& home_sensor_;
    hal::DigitalInput& applied_sensor_;
    const hal::MonotonicClock& clock_;
    SpanCorrectionSink* span_sink_{nullptr};
    CheckMassConfig config_{};
    CheckMassResult result_{};
    CheckMassState state_{CheckMassState::idle};
    std::uint64_t deadline_us_{0U};
    bool configured_{false};
    bool safety_permitted_{false};
    bool actuator_disable_fault_latched_{false};
    Status last_actuator_disable_status_{Status::ok};
    bool check_within_tolerance_{false};
    double pending_span_factor_{1.0};
    std::uint64_t required_precision_profile_epoch_{0U};
};

}  // namespace gravimetra::calibration
