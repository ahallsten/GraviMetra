#pragma once

#include "gravimetra/common/fixed_queue.hpp"
#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::comms {

constexpr std::uint8_t kServiceProtocolVersion = 1U;
constexpr std::uint8_t kServicePreambleFirst = 0xA5U;
constexpr std::uint8_t kServicePreambleSecond = 0x5AU;
constexpr std::size_t kServicePayloadCapacity = 128U;
constexpr std::size_t kServiceEncodedCapacity = kServicePayloadCapacity + 10U;
constexpr std::size_t kServiceReceiveQueueCapacity = 4U;

// These are generic service envelope classes, not machine/robot commands.
enum class ServiceMessageClass : std::uint8_t {
    request = 1U,
    response = 2U,
    event = 3U,
    telemetry = 4U,
};

struct ServiceMessage {
    ServiceMessageClass message_class{ServiceMessageClass::request};
    std::uint16_t sequence{0U};
    std::array<std::uint8_t, kServicePayloadCapacity> payload{};
    std::size_t payload_length{0U};
};

[[nodiscard]] std::uint16_t crc16_ccitt(
    const std::uint8_t* data,
    std::size_t length,
    std::uint16_t initial = 0xFFFFU) noexcept;

[[nodiscard]] Status encode_service_message(
    const ServiceMessage& message,
    std::uint8_t* destination,
    std::size_t capacity,
    std::size_t& written) noexcept;

class ServiceFrameParser {
public:
    [[nodiscard]] Status configure_inter_byte_timeout(
        const hal::MonotonicClock& clock,
        std::uint64_t timeout_us) noexcept;
    [[nodiscard]] Status ingest(std::uint8_t byte) noexcept;
    // Allows an idle communications loop to expire a truncated frame even
    // when no subsequent byte arrives.
    [[nodiscard]] Status service_timeout() noexcept;
    [[nodiscard]] bool pop(ServiceMessage& message) noexcept;
    // Drops only the partial wire frame; already-verified queued messages are
    // intentionally preserved.
    void discard_partial_frame() noexcept;
    void reset() noexcept;

    [[nodiscard]] std::size_t pending() const noexcept;
    [[nodiscard]] std::uint32_t crc_failures() const noexcept;
    [[nodiscard]] std::uint32_t malformed_frames() const noexcept;
    [[nodiscard]] std::uint32_t dropped_messages() const noexcept;
    [[nodiscard]] std::uint32_t timeout_resets() const noexcept;

private:
    enum class State : std::uint8_t {
        preamble_first = 0,
        preamble_second,
        fixed_header,
        payload,
        crc_low,
        crc_high,
    };

    void begin_frame() noexcept;
    [[nodiscard]] Status process_byte(std::uint8_t byte) noexcept;
    [[nodiscard]] Status finish_frame(std::uint8_t crc_high) noexcept;
    [[nodiscard]] bool partial_frame_active() const noexcept;
    // Replays the suffix following the last embedded preamble in a rejected,
    // bounded frame. This recovers a valid frame swallowed by a corrupted
    // length without dynamic allocation or an unbounded search.
    void resynchronize_from_bytes(
        const std::uint8_t* bytes,
        std::size_t length) noexcept;

    State state_{State::preamble_first};
    std::array<std::uint8_t, 6U> header_{};
    std::size_t header_length_{0U};
    ServiceMessage current_{};
    std::size_t payload_received_{0U};
    std::uint16_t calculated_crc_{0xFFFFU};
    std::uint8_t received_crc_low_{0U};
    const hal::MonotonicClock* clock_{nullptr};
    std::uint64_t inter_byte_timeout_us_{0U};
    std::uint64_t last_byte_time_us_{0U};
    bool have_last_byte_time_{false};
    FixedQueue<ServiceMessage, kServiceReceiveQueueCapacity> messages_{};
    std::uint32_t crc_failures_{0U};
    std::uint32_t malformed_frames_{0U};
    std::uint32_t dropped_messages_{0U};
    std::uint32_t timeout_resets_{0U};
};

}  // namespace gravimetra::comms
