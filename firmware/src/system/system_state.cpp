#include "gravimetra/system/system_state.hpp"

#include <limits>

namespace gravimetra::system {

Status SystemStateMachine::boot_complete() noexcept {
    return require_and_transition(
        SystemState::BOOT,
        SystemState::SELF_TEST,
        StateTransitionReason::normal_progress);
}

Status SystemStateMachine::self_test_complete(const bool passed) noexcept {
    if (state_ != SystemState::SELF_TEST) {
        return Status::invalid_argument;
    }
    if (!passed) {
        transition(SystemState::FAULT, StateTransitionReason::operation_failed);
        return Status::verification_failed;
    }
    transition(SystemState::WARMUP, StateTransitionReason::normal_progress);
    return Status::ok;
}

Status SystemStateMachine::warmup_complete() noexcept {
    return require_and_transition(
        SystemState::WARMUP,
        SystemState::ZERO,
        StateTransitionReason::normal_progress);
}

Status SystemStateMachine::zero_complete(const bool accepted) noexcept {
    // Dispense also uses ZERO while its planner verifies an empty pan. Only
    // planner synchronization may leave that ZERO state.
    if (state_ != SystemState::ZERO || dispense_session_active_) {
        return Status::invalid_argument;
    }
    if (!accepted) {
        transition(SystemState::FAULT, StateTransitionReason::operation_failed);
        return Status::verification_failed;
    }
    transition(SystemState::READY, StateTransitionReason::normal_progress);
    return Status::ok;
}

Status SystemStateMachine::request_dispense() noexcept {
    // The planner begins with empty-pan verification, which corresponds to the
    // system ZERO state until it reports the first enabled stage.
    const Status status = require_and_transition(
        SystemState::READY,
        SystemState::ZERO,
        StateTransitionReason::operator_request);
    if (is_ok(status)) {
        dispense_session_active_ = true;
        last_dispense_stage_ = 0xFFU;
    }
    return status;
}

Status SystemStateMachine::sync_dispense_state(
    const motion::DispenseState state) noexcept {
    // E-stop has higher priority than every other top-level state, including
    // an already-latched ordinary fault.
    if (state == motion::DispenseState::estop) {
        assert_estop();
        return Status::fault_active;
    }
    // A stale planner update must never demote a latched fault or E-stop.
    if (state_ == SystemState::ESTOP || state_ == SystemState::FAULT) {
        return Status::fault_active;
    }
    if (!dispense_session_active_ || !dispense_sequence_state()) {
        return Status::invalid_argument;
    }

    SystemState next = state_;
    std::uint8_t incoming_stage = 0xFFU;
    switch (state) {
        case motion::DispenseState::idle:
        case motion::DispenseState::verify_empty:
            next = SystemState::ZERO;
            break;
        case motion::DispenseState::dispense_stage_1:
            next = SystemState::DISPENSE_STAGE_1;
            incoming_stage = 0U;
            break;
        case motion::DispenseState::dispense_stage_2:
            next = SystemState::DISPENSE_STAGE_2;
            incoming_stage = 1U;
            break;
        case motion::DispenseState::dispense_stage_3:
            next = SystemState::DISPENSE_STAGE_3;
            incoming_stage = 2U;
            break;
        case motion::DispenseState::dispense_stage_4:
            next = SystemState::DISPENSE_STAGE_4;
            incoming_stage = 3U;
            break;
        case motion::DispenseState::settle:
            next = SystemState::SETTLE;
            break;
        case motion::DispenseState::validate:
            next = SystemState::VALIDATE;
            break;
        case motion::DispenseState::complete:
            next = SystemState::REPORT;
            break;
        case motion::DispenseState::underfill:
        case motion::DispenseState::overfill:
        case motion::DispenseState::fault:
            dispense_session_active_ = false;
            transition(SystemState::FAULT, StateTransitionReason::operation_failed);
            return Status::fault_active;
        case motion::DispenseState::estop:
            // Handled before the active-session guard above.
            return Status::fault_active;
        default:
            return Status::invalid_argument;
    }

    // Planner reports are polled; repeated reports are idempotent and should
    // not create fictitious lifecycle transitions.
    if (next == state_) {
        return Status::ok;
    }

    bool allowed = false;
    if (next == SystemState::ZERO) {
        allowed = state_ == SystemState::ZERO;
    } else if (incoming_stage != 0xFFU) {
        const bool source_allows_stage =
            state_ == SystemState::ZERO || state_ == SystemState::SETTLE;
        const bool stage_is_forward = last_dispense_stage_ == 0xFFU ||
            incoming_stage > last_dispense_stage_;
        allowed = source_allows_stage && stage_is_forward;
    } else if (next == SystemState::SETTLE) {
        allowed = motion_state();
    } else if (next == SystemState::VALIDATE) {
        allowed = state_ == SystemState::SETTLE;
    } else if (next == SystemState::REPORT) {
        allowed = state_ == SystemState::VALIDATE;
    }
    if (!allowed) {
        return Status::invalid_argument;
    }
    if (incoming_stage != 0xFFU) {
        last_dispense_stage_ = incoming_stage;
    }
    transition(next, StateTransitionReason::dispense_progress);
    return Status::ok;
}

Status SystemStateMachine::report_complete() noexcept {
    const Status status = require_and_transition(
        SystemState::REPORT,
        SystemState::UNLOAD_OR_CLEANUP,
        StateTransitionReason::operation_complete);
    if (is_ok(status)) {
        dispense_session_active_ = false;
        last_dispense_stage_ = 0xFFU;
    }
    return status;
}

Status SystemStateMachine::cleanup_complete() noexcept {
    return require_and_transition(
        SystemState::UNLOAD_OR_CLEANUP,
        SystemState::READY,
        StateTransitionReason::operation_complete);
}

Status SystemStateMachine::request_auto_check() noexcept {
    return require_and_transition(
        SystemState::READY,
        SystemState::AUTO_CHECK_CAL,
        StateTransitionReason::operator_request);
}

Status SystemStateMachine::auto_check_complete(const bool passed) noexcept {
    if (state_ != SystemState::AUTO_CHECK_CAL) {
        return Status::invalid_argument;
    }
    transition(
        passed ? SystemState::READY : SystemState::FAULT,
        passed ? StateTransitionReason::operation_complete
               : StateTransitionReason::operation_failed);
    return passed ? Status::ok : Status::verification_failed;
}

Status SystemStateMachine::request_manual_calibration() noexcept {
    return require_and_transition(
        SystemState::READY,
        SystemState::MANUAL_CALIBRATION,
        StateTransitionReason::operator_request);
}

Status SystemStateMachine::manual_calibration_complete(
    const bool accepted) noexcept {
    if (state_ != SystemState::MANUAL_CALIBRATION) {
        return Status::invalid_argument;
    }
    transition(
        accepted ? SystemState::READY : SystemState::FAULT,
        accepted ? StateTransitionReason::operation_complete
                 : StateTransitionReason::operation_failed);
    return accepted ? Status::ok : Status::verification_failed;
}

Status SystemStateMachine::request_service() noexcept {
    return require_and_transition(
        SystemState::READY,
        SystemState::SERVICE,
        StateTransitionReason::operator_request);
}

Status SystemStateMachine::exit_service() noexcept {
    return require_and_transition(
        SystemState::SERVICE,
        SystemState::SELF_TEST,
        StateTransitionReason::supervised_reset);
}

void SystemStateMachine::raise_fault() noexcept {
    dispense_session_active_ = false;
    last_dispense_stage_ = 0xFFU;
    if (state_ != SystemState::ESTOP && state_ != SystemState::FAULT) {
        transition(SystemState::FAULT, StateTransitionReason::fault_detected);
    }
}

void SystemStateMachine::assert_estop() noexcept {
    dispense_session_active_ = false;
    last_dispense_stage_ = 0xFFU;
    if (state_ != SystemState::ESTOP) {
        transition(SystemState::ESTOP, StateTransitionReason::estop_asserted);
    }
}

Status SystemStateMachine::supervised_reset(
    const bool estop_clear,
    const bool blocking_faults_clear) noexcept {
    if (state_ != SystemState::FAULT && state_ != SystemState::ESTOP) {
        return Status::invalid_argument;
    }
    if (!estop_clear || !blocking_faults_clear) {
        return Status::fault_active;
    }
    dispense_session_active_ = false;
    last_dispense_stage_ = 0xFFU;
    transition(SystemState::SELF_TEST, StateTransitionReason::supervised_reset);
    return Status::ok;
}

bool SystemStateMachine::motion_state() const noexcept {
    switch (state_) {
        case SystemState::DISPENSE_STAGE_1:
        case SystemState::DISPENSE_STAGE_2:
        case SystemState::DISPENSE_STAGE_3:
        case SystemState::DISPENSE_STAGE_4:
            return true;
        default:
            return false;
    }
}

bool SystemStateMachine::dispense_sequence_state() const noexcept {
    switch (state_) {
        case SystemState::ZERO:
        case SystemState::DISPENSE_STAGE_1:
        case SystemState::DISPENSE_STAGE_2:
        case SystemState::DISPENSE_STAGE_3:
        case SystemState::DISPENSE_STAGE_4:
        case SystemState::SETTLE:
        case SystemState::VALIDATE:
        case SystemState::REPORT:
            return true;
        default:
            return false;
    }
}

Status SystemStateMachine::require_and_transition(
    const SystemState required,
    const SystemState next,
    const StateTransitionReason reason) noexcept {
    if (state_ != required) {
        return Status::invalid_argument;
    }
    transition(next, reason);
    return Status::ok;
}

void SystemStateMachine::transition(
    const SystemState next,
    const StateTransitionReason reason) noexcept {
    const auto previous = state_;
    state_ = next;
    if (transition_sequence_ != std::numeric_limits<std::uint64_t>::max()) {
        ++transition_sequence_;
    }
    last_transition_ = StateTransition{
        previous,
        next,
        reason,
        transition_sequence_,
    };
}

}  // namespace gravimetra::system
