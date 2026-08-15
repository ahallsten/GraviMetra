#pragma once

#include "gravimetra/common/fixed_queue.hpp"
#include "gravimetra/common/status.hpp"
#include "gravimetra/hmi/nextion.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::hmi {

enum class UiPageRole : std::uint8_t {
    home_run = 0,
    target_setup,
    live_dispensing,
    calibration,
    internal_check,
    diagnostics,
    power_battery,
    fault_history,
    service_configuration,
};

enum class UiRequestType : std::uint8_t {
    navigate = 0,
    set_target,
    start_dispense,
    stop_dispense,
    tare,
    begin_calibration,
    run_internal_check,
    acknowledge_fault,
    enter_service,
};

enum class UiTouchEdge : std::uint8_t {
    press = 0,
    release,
    either,
};

struct UiRequest {
    UiRequestType type{UiRequestType::navigate};
    std::uint8_t page_id{0U};
    std::uint8_t component_id{0U};
    std::uint32_t value{0U};
};

struct UiBinding {
    std::uint8_t page_id{0U};
    std::uint8_t component_id{0U};
    UiTouchEdge edge{UiTouchEdge::release};
    UiRequestType request{UiRequestType::navigate};
    // Numeric responses do not carry a component identifier. At most one
    // binding may therefore await the next 0x71 response. Bounds come from
    // application configuration; this layer does not invent a target range.
    bool value_from_next_numeric_response{false};
    std::uint32_t minimum_value{0U};
    std::uint32_t maximum_value{0U};
};

constexpr std::size_t kUiBindingCapacity = 24U;
constexpr std::size_t kUiRequestCapacity = 16U;

// Converts configured component events into application requests. This class
// deliberately has no access to actuators, motion, calibration, or safety APIs.
class UiRequestModel {
public:
    [[nodiscard]] Status bind(const UiBinding& binding) noexcept;
    [[nodiscard]] Status accept(const NextionEvent& event) noexcept;
    [[nodiscard]] Status request(const UiRequest& request) noexcept;
    [[nodiscard]] bool pop(UiRequest& request) noexcept;
    void clear_requests() noexcept;
    void cancel_pending_numeric_response() noexcept;

    [[nodiscard]] std::size_t binding_count() const noexcept;
    [[nodiscard]] std::size_t pending_requests() const noexcept;
    [[nodiscard]] bool pending_numeric_request(
        UiRequest& request) const noexcept;

private:
    std::array<UiBinding, kUiBindingCapacity> bindings_{};
    std::size_t binding_count_{0U};
    FixedQueue<UiRequest, kUiRequestCapacity> requests_{};
    UiRequest pending_numeric_request_{};
    std::uint32_t pending_minimum_value_{0U};
    std::uint32_t pending_maximum_value_{0U};
    bool numeric_response_pending_{false};
};

}  // namespace gravimetra::hmi
