#include "gravimetra/board/board_config.hpp"
#include "gravimetra/common/fixed_queue.hpp"
#include "gravimetra/common/status.hpp"

#include <unity.h>

namespace {

void test_unreleased_board_is_motion_safe() {
    TEST_ASSERT_FALSE(gravimetra::board::kRevisionAUnreleased.pin_map_released);
    TEST_ASSERT_FALSE(
        gravimetra::board::motion_mapping_complete(
            gravimetra::board::kRevisionAUnreleased));
}

void test_motion_mapping_requires_unique_uart_timer_and_gpio_resources() {
    using namespace gravimetra::board;
    BoardConfig config{};
    config.pin_map_released = true;
    const std::array<Peripheral, 4U> uarts{
        Peripheral::usart1,
        Peripheral::usart2,
        Peripheral::usart3,
        Peripheral::uart4,
    };
    const std::array<TimerPeripheral, 4U> timers{
        TimerPeripheral::tim1,
        TimerPeripheral::tim2,
        TimerPeripheral::tim3,
        TimerPeripheral::tim4,
    };
    for (std::size_t index = 0U; index < config.motors.size(); ++index) {
        config.motors[index].step =
            Pin{Port::a, static_cast<std::uint8_t>(index), false};
        config.motors[index].step_timer = TimerChannel{timers[index], 1U};
        config.motors[index].direction = Pin{
            Port::b, static_cast<std::uint8_t>(index), false};
        config.motors[index].enable =
            Pin{Port::c, static_cast<std::uint8_t>(index), true};
        config.motors[index].uart = uarts[index];
    }
    config.estop_monitor = Pin{Port::d, 0U, true};
    TEST_ASSERT_TRUE(motion_mapping_complete(config));

    config.motors[3U].uart = config.motors[0U].uart;
    TEST_ASSERT_FALSE(motion_mapping_complete(config));
    config.motors[3U].uart = uarts[3U];
    config.motors[3U].step_timer = config.motors[0U].step_timer;
    TEST_ASSERT_FALSE(motion_mapping_complete(config));
    config.motors[3U].step_timer = TimerChannel{timers[3U], 1U};
    config.estop_monitor = config.motors[1U].enable;
    TEST_ASSERT_FALSE(motion_mapping_complete(config));
}

void test_fixed_queue_is_bounded_fifo() {
    gravimetra::FixedQueue<int, 2U> queue;
    TEST_ASSERT_TRUE(queue.push(11));
    TEST_ASSERT_TRUE(queue.push(22));
    TEST_ASSERT_FALSE(queue.push(33));

    int value = 0;
    TEST_ASSERT_TRUE(queue.pop(value));
    TEST_ASSERT_EQUAL_INT(11, value);
    TEST_ASSERT_TRUE(queue.pop(value));
    TEST_ASSERT_EQUAL_INT(22, value);
    TEST_ASSERT_FALSE(queue.pop(value));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_unreleased_board_is_motion_safe);
    RUN_TEST(test_motion_mapping_requires_unique_uart_timer_and_gpio_resources);
    RUN_TEST(test_fixed_queue_is_bounded_fifo);
    return UNITY_END();
}
