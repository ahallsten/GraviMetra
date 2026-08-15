#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/motion/dispense_planner.hpp"

#include <cstdint>

namespace gravimetra::system {

enum class SystemState : std::uint8_t {
    BOOT = 0,
    SELF_TEST,
    WARMUP,
    ZERO,
    READY,
    DISPENSE_STAGE_1,
    DISPENSE_STAGE_2,
    DISPENSE_STAGE_3,
    DISPENSE_STAGE_4,
    SETTLE,
    VALIDATE,
    REPORT,
    UNLOAD_OR_CLEANUP,
    AUTO_CHECK_CAL,
    MANUAL_CALIBRATION,
    SERVICE,
    FAULT,
    ESTOP,
};

enum class StateTransitionReason : std::uint8_t {
    power_on = 0,
    normal_progress,
    operator_request,
    dispense_progress,
    operation_complete,
    operation_failed,
    fault_detected,
    estop_asserted,
    supervised_reset,
};

struct StateTransition {
    SystemState from{SystemState::BOOT};
    SystemState to{SystemState::BOOT};
    StateTransitionReason reason{StateTransitionReason::power_on};
    std::uint64_t sequence{0U};
};

// Top-level lifecycle authority. Detailed dispense and check-mass sequencing
// stays in their domain controllers; this class accepts only their reported
// state/result and cannot directly energize hardware.
class SystemStateMachine {
public:
    SystemStateMachine() noexcept = default;

    [[nodiscard]] Status boot_complete() noexcept;
    [[nodiscard]] Status self_test_complete(bool passed) noexcept;
    [[nodiscard]] Status warmup_complete() noexcept;
    [[nodiscard]] Status zero_complete(bool accepted) noexcept;

    [[nodiscard]] Status request_dispense() noexcept;
    [[nodiscard]] Status sync_dispense_state(
        motion::DispenseState state) noexcept;
    [[nodiscard]] Status report_complete() noexcept;
    [[nodiscard]] Status cleanup_complete() noexcept;

    [[nodiscard]] Status request_auto_check() noexcept;
    [[nodiscard]] Status auto_check_complete(bool passed) noexcept;
    [[nodiscard]] Status request_manual_calibration() noexcept;
    [[nodiscard]] Status manual_calibration_complete(bool accepted) noexcept;
    [[nodiscard]] Status request_service() noexcept;
    [[nodiscard]] Status exit_service() noexcept;

    void raise_fault() noexcept;
    void assert_estop() noexcept;
    [[nodiscard]] Status supervised_reset(
        bool estop_clear,
        bool blocking_faults_clear) noexcept;

    [[nodiscard]] SystemState state() const noexcept { return state_; }
    [[nodiscard]] const StateTransition& last_transition() const noexcept {
        return last_transition_;
    }
    [[nodiscard]] bool motion_state() const noexcept;

private:
    [[nodiscard]] Status require_and_transition(
        SystemState required,
        SystemState next,
        StateTransitionReason reason) noexcept;
    void transition(
        SystemState next,
        StateTransitionReason reason) noexcept;
    [[nodiscard]] bool dispense_sequence_state() const noexcept;

    SystemState state_{SystemState::BOOT};
    StateTransition last_transition_{};
    std::uint64_t transition_sequence_{0U};
    bool dispense_session_active_{false};
    std::uint8_t last_dispense_stage_{0xFFU};
};

}  // namespace gravimetra::system
