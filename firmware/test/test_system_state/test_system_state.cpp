#include "gravimetra/system/system_state.hpp"

#include <unity.h>

namespace {

using gravimetra::Status;
using gravimetra::motion::DispenseState;
using gravimetra::system::SystemState;
using gravimetra::system::SystemStateMachine;

void advance_to_ready(SystemStateMachine& machine) {
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.boot_complete()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.self_test_complete(true)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.warmup_complete()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.zero_complete(true)));
}

void test_lifecycle_and_dispense_mapping() {
    SystemStateMachine machine;
    advance_to_ready(machine);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::READY),
        static_cast<int>(machine.state()));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.request_dispense()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.sync_dispense_state(
            DispenseState::dispense_stage_1)));
    TEST_ASSERT_TRUE(machine.motion_state());

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.sync_dispense_state(DispenseState::settle)));
    // Stage 3 may follow the required settle when stage 2 is disabled.
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.sync_dispense_state(
            DispenseState::dispense_stage_3)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::DISPENSE_STAGE_3),
        static_cast<int>(machine.state()));

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.sync_dispense_state(DispenseState::settle)));
    TEST_ASSERT_FALSE(machine.motion_state());
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.sync_dispense_state(DispenseState::validate)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.sync_dispense_state(DispenseState::complete)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.report_complete()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.cleanup_complete()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::READY),
        static_cast<int>(machine.state()));
}

void test_estop_is_latched_until_supervised_reset() {
    SystemStateMachine machine;
    advance_to_ready(machine);
    machine.assert_estop();
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::ESTOP),
        static_cast<int>(machine.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(machine.supervised_reset(false, true)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::ESTOP),
        static_cast<int>(machine.state()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.supervised_reset(true, true)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::SELF_TEST),
        static_cast<int>(machine.state()));
}

void test_failed_check_faults_without_silent_recovery() {
    SystemStateMachine machine;
    advance_to_ready(machine);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.request_auto_check()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::verification_failed),
        static_cast<int>(machine.auto_check_complete(false)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::FAULT),
        static_cast<int>(machine.state()));
}

void test_stale_planner_updates_cannot_bypass_boot_or_demote_estop() {
    SystemStateMachine machine;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(machine.sync_dispense_state(
            DispenseState::dispense_stage_1)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::BOOT),
        static_cast<int>(machine.state()));

    machine.assert_estop();
    const auto estop_sequence = machine.last_transition().sequence;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(machine.sync_dispense_state(DispenseState::fault)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::ESTOP),
        static_cast<int>(machine.state()));
    TEST_ASSERT_EQUAL_UINT64(estop_sequence, machine.last_transition().sequence);
}

void test_dispense_zero_is_owned_by_the_planner_and_estop_outranks_fault() {
    SystemStateMachine machine;
    advance_to_ready(machine);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.request_dispense()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(machine.zero_complete(true)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::ZERO),
        static_cast<int>(machine.state()));

    machine.raise_fault();
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::fault_active),
        static_cast<int>(machine.sync_dispense_state(DispenseState::estop)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(SystemState::ESTOP),
        static_cast<int>(machine.state()));
}

void test_dispense_lifecycle_rejects_skips_regressions_and_unknown_states() {
    SystemStateMachine machine;
    advance_to_ready(machine);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.request_dispense()));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(machine.sync_dispense_state(DispenseState::complete)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(machine.sync_dispense_state(
            static_cast<DispenseState>(0xFFU))));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.sync_dispense_state(
            DispenseState::dispense_stage_2)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(machine.sync_dispense_state(
            DispenseState::dispense_stage_1)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::ok),
        static_cast<int>(machine.sync_dispense_state(DispenseState::settle)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(Status::invalid_argument),
        static_cast<int>(machine.sync_dispense_state(DispenseState::complete)));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_lifecycle_and_dispense_mapping);
    RUN_TEST(test_estop_is_latched_until_supervised_reset);
    RUN_TEST(test_failed_check_faults_without_silent_recovery);
    RUN_TEST(test_stale_planner_updates_cannot_bypass_boot_or_demote_estop);
    RUN_TEST(test_dispense_zero_is_owned_by_the_planner_and_estop_outranks_fault);
    RUN_TEST(test_dispense_lifecycle_rejects_skips_regressions_and_unknown_states);
    return UNITY_END();
}
