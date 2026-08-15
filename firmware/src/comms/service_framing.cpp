#include "gravimetra/comms/service_framing.hpp"

namespace gravimetra::comms {
namespace {

[[nodiscard]] std::uint16_t crc16_byte(
    std::uint16_t crc,
    const std::uint8_t byte) noexcept {
    const auto shifted_byte = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(byte) << 8U);
    crc = static_cast<std::uint16_t>(crc ^ shifted_byte);
    for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
        crc = (crc & 0x8000U) != 0U
            ? static_cast<std::uint16_t>(
                  (static_cast<std::uint32_t>(crc) << 1U) ^
                  std::uint32_t{0x1021U})
            : static_cast<std::uint16_t>(
                  static_cast<std::uint32_t>(crc) << 1U);
    }
    return crc;
}

[[nodiscard]] bool valid_message_class(const std::uint8_t value) noexcept {
    return value >= static_cast<std::uint8_t>(ServiceMessageClass::request) &&
        value <= static_cast<std::uint8_t>(ServiceMessageClass::telemetry);
}

}  // namespace

std::uint16_t crc16_ccitt(
    const std::uint8_t* const data,
    const std::size_t length,
    const std::uint16_t initial) noexcept {
    if (data == nullptr && length != 0U) {
        return initial;
    }
    std::uint16_t crc = initial;
    for (std::size_t index = 0U; index < length; ++index) {
        crc = crc16_byte(crc, data[index]);
    }
    return crc;
}

Status encode_service_message(
    const ServiceMessage& message,
    std::uint8_t* const destination,
    const std::size_t capacity,
    std::size_t& written) noexcept {
    written = 0U;
    if (destination == nullptr ||
        message.payload_length > message.payload.size() ||
        message.payload_length > 0xFFFFU ||
        !valid_message_class(static_cast<std::uint8_t>(message.message_class))) {
        return Status::invalid_argument;
    }
    const std::size_t required = message.payload_length + 10U;
    if (capacity < required) {
        return Status::queue_full;
    }

    destination[0U] = kServicePreambleFirst;
    destination[1U] = kServicePreambleSecond;
    destination[2U] = kServiceProtocolVersion;
    destination[3U] = static_cast<std::uint8_t>(message.message_class);
    destination[4U] = static_cast<std::uint8_t>(message.sequence & 0xFFU);
    destination[5U] = static_cast<std::uint8_t>((message.sequence >> 8U) & 0xFFU);
    const auto payload_length = static_cast<std::uint16_t>(message.payload_length);
    destination[6U] = static_cast<std::uint8_t>(payload_length & 0xFFU);
    destination[7U] = static_cast<std::uint8_t>((payload_length >> 8U) & 0xFFU);
    for (std::size_t index = 0U; index < message.payload_length; ++index) {
        destination[8U + index] = message.payload[index];
    }
    const std::uint16_t crc =
        crc16_ccitt(&destination[2U], 6U + message.payload_length);
    destination[8U + message.payload_length] =
        static_cast<std::uint8_t>(crc & 0xFFU);
    destination[9U + message.payload_length] =
        static_cast<std::uint8_t>((crc >> 8U) & 0xFFU);
    written = required;
    return Status::ok;
}

void ServiceFrameParser::begin_frame() noexcept {
    state_ = State::fixed_header;
    header_length_ = 0U;
    payload_received_ = 0U;
    calculated_crc_ = 0xFFFFU;
    current_ = ServiceMessage{};
}

Status ServiceFrameParser::configure_inter_byte_timeout(
    const hal::MonotonicClock& clock,
    const std::uint64_t timeout_us) noexcept {
    if (timeout_us == 0U) {
        return Status::invalid_argument;
    }
    clock_ = &clock;
    inter_byte_timeout_us_ = timeout_us;
    have_last_byte_time_ = partial_frame_active();
    if (have_last_byte_time_) {
        last_byte_time_us_ = clock.now_us();
    }
    return Status::ok;
}

Status ServiceFrameParser::ingest(const std::uint8_t byte) noexcept {
    Status timeout_status = Status::ok;
    std::uint64_t now = 0U;
    if (clock_ != nullptr && inter_byte_timeout_us_ != 0U) {
        now = clock_->now_us();
        if (partial_frame_active() && have_last_byte_time_ &&
            now - last_byte_time_us_ >= inter_byte_timeout_us_) {
            discard_partial_frame();
            ++timeout_resets_;
            timeout_status = Status::timeout;
        }
    }

    const Status byte_status = process_byte(byte);
    if (clock_ != nullptr && inter_byte_timeout_us_ != 0U) {
        have_last_byte_time_ = partial_frame_active();
        if (have_last_byte_time_) {
            last_byte_time_us_ = now;
        }
    }
    return !is_ok(byte_status) ? byte_status : timeout_status;
}

Status ServiceFrameParser::process_byte(const std::uint8_t byte) noexcept {
    switch (state_) {
        case State::preamble_first:
            if (byte == kServicePreambleFirst) {
                state_ = State::preamble_second;
            }
            return Status::ok;
        case State::preamble_second:
            if (byte == kServicePreambleSecond) {
                begin_frame();
            } else if (byte != kServicePreambleFirst) {
                state_ = State::preamble_first;
            }
            return Status::ok;
        case State::fixed_header:
            header_[header_length_] = byte;
            ++header_length_;
            calculated_crc_ = crc16_byte(calculated_crc_, byte);
            if (header_length_ < header_.size()) {
                return Status::ok;
            }
            if (header_[0U] != kServiceProtocolVersion ||
                !valid_message_class(header_[1U])) {
                ++malformed_frames_;
                resynchronize_from_bytes(header_.data(), header_length_);
                return Status::protocol_error;
            }
            current_.message_class =
                static_cast<ServiceMessageClass>(header_[1U]);
            current_.sequence = static_cast<std::uint16_t>(
                static_cast<std::uint16_t>(header_[2U]) |
                static_cast<std::uint16_t>(
                    static_cast<std::uint16_t>(header_[3U]) << 8U));
            current_.payload_length = static_cast<std::size_t>(
                static_cast<std::uint16_t>(
                    static_cast<std::uint16_t>(header_[4U]) |
                    static_cast<std::uint16_t>(
                        static_cast<std::uint16_t>(header_[5U]) << 8U)));
            if (current_.payload_length > current_.payload.size()) {
                ++malformed_frames_;
                resynchronize_from_bytes(header_.data(), header_length_);
                return Status::protocol_error;
            }
            state_ = current_.payload_length == 0U
                ? State::crc_low
                : State::payload;
            return Status::ok;
        case State::payload:
            current_.payload[payload_received_] = byte;
            ++payload_received_;
            calculated_crc_ = crc16_byte(calculated_crc_, byte);
            if (payload_received_ == current_.payload_length) {
                state_ = State::crc_low;
            }
            return Status::ok;
        case State::crc_low:
            received_crc_low_ = byte;
            state_ = State::crc_high;
            return Status::ok;
        case State::crc_high:
            return finish_frame(byte);
    }
    ++malformed_frames_;
    reset();
    return Status::protocol_error;
}

Status ServiceFrameParser::finish_frame(const std::uint8_t crc_high) noexcept {
    const auto received = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(received_crc_low_) |
        static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(crc_high) << 8U));
    if (received != calculated_crc_) {
        ++crc_failures_;
        std::array<std::uint8_t, kServiceEncodedCapacity> rejected{};
        std::size_t rejected_length = 0U;
        for (std::size_t index = 0U; index < header_length_; ++index) {
            rejected[rejected_length++] = header_[index];
        }
        for (std::size_t index = 0U; index < payload_received_; ++index) {
            rejected[rejected_length++] = current_.payload[index];
        }
        rejected[rejected_length++] = received_crc_low_;
        rejected[rejected_length++] = crc_high;
        resynchronize_from_bytes(rejected.data(), rejected_length);
        return Status::verification_failed;
    }
    const ServiceMessage completed = current_;
    discard_partial_frame();
    if (!messages_.push(completed)) {
        ++dropped_messages_;
        return Status::queue_full;
    }
    return Status::ok;
}

Status ServiceFrameParser::service_timeout() noexcept {
    if (clock_ == nullptr || inter_byte_timeout_us_ == 0U) {
        return Status::not_configured;
    }
    if (!partial_frame_active() || !have_last_byte_time_) {
        return Status::busy;
    }
    if (clock_->now_us() - last_byte_time_us_ < inter_byte_timeout_us_) {
        return Status::busy;
    }
    discard_partial_frame();
    ++timeout_resets_;
    return Status::timeout;
}

bool ServiceFrameParser::pop(ServiceMessage& message) noexcept {
    return messages_.pop(message);
}

void ServiceFrameParser::discard_partial_frame() noexcept {
    state_ = State::preamble_first;
    header_length_ = 0U;
    payload_received_ = 0U;
    calculated_crc_ = 0xFFFFU;
    received_crc_low_ = 0U;
    current_ = ServiceMessage{};
    have_last_byte_time_ = false;
}

void ServiceFrameParser::reset() noexcept {
    discard_partial_frame();
    messages_.clear();
}

bool ServiceFrameParser::partial_frame_active() const noexcept {
    return state_ != State::preamble_first;
}

void ServiceFrameParser::resynchronize_from_bytes(
    const std::uint8_t* const bytes,
    const std::size_t length) noexcept {
    if (bytes == nullptr || length == 0U) {
        discard_partial_frame();
        return;
    }

    std::size_t preamble_index = length;
    if (length >= 2U) {
        for (std::size_t index = length - 1U; index != 0U; --index) {
            if (bytes[index - 1U] == kServicePreambleFirst &&
                bytes[index] == kServicePreambleSecond) {
                preamble_index = index - 1U;
                break;
            }
        }
    }

    const bool trailing_first = bytes[length - 1U] == kServicePreambleFirst;
    if (preamble_index == length) {
        discard_partial_frame();
        if (trailing_first) {
            state_ = State::preamble_second;
        }
        return;
    }

    std::array<std::uint8_t, kServiceEncodedCapacity> suffix{};
    const std::size_t suffix_length = length - preamble_index - 2U;
    for (std::size_t index = 0U; index < suffix_length; ++index) {
        suffix[index] = bytes[preamble_index + 2U + index];
    }

    discard_partial_frame();
    begin_frame();
    for (std::size_t index = 0U; index < suffix_length; ++index) {
        // Any nested rejection searches a strictly shorter suffix because the
        // last preamble was selected above; recursion therefore remains
        // bounded by one additional parser frame.
        static_cast<void>(process_byte(suffix[index]));
    }
}

std::size_t ServiceFrameParser::pending() const noexcept {
    return messages_.size();
}

std::uint32_t ServiceFrameParser::crc_failures() const noexcept {
    return crc_failures_;
}

std::uint32_t ServiceFrameParser::malformed_frames() const noexcept {
    return malformed_frames_;
}

std::uint32_t ServiceFrameParser::dropped_messages() const noexcept {
    return dropped_messages_;
}

std::uint32_t ServiceFrameParser::timeout_resets() const noexcept {
    return timeout_resets_;
}

}  // namespace gravimetra::comms
