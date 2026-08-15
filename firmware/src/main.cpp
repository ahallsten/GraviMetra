#include "gravimetra/board/board_config.hpp"

#if defined(GRAVIMETRA_TARGET_STM32G491)
#include "stm32g4xx_hal.h"
#endif

namespace {

[[noreturn]] void safe_unconfigured_loop() noexcept {
    for (;;) {
#if defined(GRAVIMETRA_TARGET_STM32G491)
        __WFI();
#endif
    }
}

}  // namespace

int main() {
#if defined(GRAVIMETRA_TARGET_STM32G491)
    HAL_Init();
#endif

    // No production GPIO is touched until the pin map is explicitly released.
    // This makes an accidental flash inert while retaining a compilable target.
    if (!gravimetra::board::kRevisionAUnreleased.pin_map_released) {
        safe_unconfigured_loop();
    }

    safe_unconfigured_loop();
}

