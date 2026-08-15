#pragma once

#include "gravimetra/common/fixed_queue.hpp"
#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::hmi {

constexpr std::size_t kNextionFrameCapacity = 64U;
constexpr std::size_t kNextionTextCapacity = 48U;
constexpr std::size_t kNextionEventQueueCapacity = 8U;
constexpr std::size_t kNextionCommandCapacity = 96U;
constexpr std::size_t kNextionCommandQueueCapacity = 8U;

enum class NextionEventType : std::uint8_t {
    acknowledgement = 0,
    touch,
    page,
    numeric_value,
    string_value,
    ready,
    error,
    unknown,
};

struct NextionEvent {
    NextionEventType type{NextionEventType::unknown};
    std::uint8_t response_code{0U};
    std::uint8_t page_id{0U};
    std::uint8_t component_id{0U};
    bool pressed{false};
    std::uint32_t numeric_value{0U};
    std::array<char, kNextionTextCapacity> text{};
    std::size_t text_length{0U};
};

// Consumes bytes incrementally. A complete Nextion response ends in three 0xFF
// bytes. No caller is required to wait for a response or terminator.
class NextionParser {
public:
    [[nodiscard]] Status ingest(std::uint8_t byte) noexcept;
    [[nodiscard]] Status poll(
        hal::Uart& uart,
        std::size_t byte_budget,
        std::size_t& bytes_read) noexcept;
    [[nodiscard]] bool pop(NextionEvent& event) noexcept;

    void reset() noexcept;
    [[nodiscard]] std::size_t pending_events() const noexcept;
    [[nodiscard]] std::uint32_t malformed_frames() const noexcept;
    [[nodiscard]] std::uint32_t dropped_events() const noexcept;

private:
    [[nodiscard]] Status finish_frame() noexcept;
    [[nodiscard]] Status append(std::uint8_t byte) noexcept;

    std::array<std::uint8_t, kNextionFrameCapacity> frame_{};
    std::size_t frame_length_{0U};
    std::uint8_t terminator_bytes_{0U};
    bool discarding_{false};
    FixedQueue<NextionEvent, kNextionEventQueueCapacity> events_{};
    std::uint32_t malformed_frames_{0U};
    std::uint32_t dropped_events_{0U};
};

struct NextionCommand {
    std::array<std::uint8_t, kNextionCommandCapacity> bytes{};
    std::size_t length{0U};
};

// Commands are sent without waiting for an acknowledgement. A failed UART
// write retains the in-flight command for an explicit retry.
class NextionCommandQueue {
public:
    [[nodiscard]] Status enqueue(const char* command, std::size_t length) noexcept;
    [[nodiscard]] Status enqueue_page(std::uint8_t page_id) noexcept;
    [[nodiscard]] Status enqueue_numeric(
        const char* component,
        std::uint32_t value) noexcept;
    [[nodiscard]] Status enqueue_text(
        const char* component,
        const char* text) noexcept;
    [[nodiscard]] Status send_next(hal::Uart& uart) noexcept;

    void clear() noexcept;
    [[nodiscard]] std::size_t pending() const noexcept;

private:
    [[nodiscard]] static bool valid_identifier(const char* value) noexcept;
    [[nodiscard]] static bool valid_text(const char* value) noexcept;

    FixedQueue<NextionCommand, kNextionCommandQueueCapacity> commands_{};
    NextionCommand in_flight_{};
    bool has_in_flight_{false};
};

}  // namespace gravimetra::hmi
