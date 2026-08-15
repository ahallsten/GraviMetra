#include "gravimetra/hmi/nextion.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace gravimetra::hmi {
namespace {

constexpr std::uint8_t kTerminator = 0xFFU;

// Zero denotes a delimiter-terminated variable-length response. All other
// supported response types have a protocol-defined body length, so 0xFF bytes
// inside their payload must not be mistaken for the three-byte terminator.
[[nodiscard]] std::size_t fixed_frame_length(
    const std::uint8_t response_code) noexcept {
    switch (response_code) {
        case 0x65U:
            return 4U;
        case 0x66U:
            return 2U;
        case 0x70U:
            return 0U;
        case 0x71U:
            return 5U;
        default:
            return 1U;
    }
}

[[nodiscard]] bool is_error_response(const std::uint8_t code) noexcept {
    switch (code) {
        case 0x00U:
        case 0x02U:
        case 0x03U:
        case 0x04U:
        case 0x05U:
        case 0x11U:
        case 0x12U:
        case 0x1AU:
        case 0x1BU:
        case 0x1CU:
        case 0x1DU:
        case 0x1EU:
        case 0x1FU:
        case 0x20U:
        case 0x23U:
        case 0x24U:
            return true;
        default:
            return false;
    }
}

}  // namespace

Status NextionParser::append(const std::uint8_t byte) noexcept {
    if (frame_length_ >= frame_.size()) {
        discarding_ = true;
        frame_length_ = 0U;
        terminator_bytes_ = 0U;
        ++malformed_frames_;
        return Status::protocol_error;
    }
    frame_[frame_length_] = byte;
    ++frame_length_;
    return Status::ok;
}

Status NextionParser::ingest(const std::uint8_t byte) noexcept {
    if (discarding_) {
        if (byte == kTerminator) {
            ++terminator_bytes_;
            if (terminator_bytes_ == 3U) {
                discarding_ = false;
                terminator_bytes_ = 0U;
            }
        } else {
            terminator_bytes_ = 0U;
        }
        return Status::ok;
    }

    if (frame_length_ != 0U) {
        const std::size_t expected = fixed_frame_length(frame_[0U]);
        if (expected != 0U && frame_length_ < expected) {
            // Fixed-width payload bytes are opaque and may legitimately be
            // 0xFF, including three consecutive bytes in a numeric response.
            return append(byte);
        }
        if (expected != 0U) {
            if (byte == kTerminator) {
                ++terminator_bytes_;
                if (terminator_bytes_ == 3U) {
                    terminator_bytes_ = 0U;
                    return finish_frame();
                }
                return Status::ok;
            }

            // A fixed-width response must be followed immediately by exactly
            // three terminator bytes. Discard the damaged frame through its
            // next terminator so payload bytes cannot become UI requests.
            discarding_ = true;
            frame_length_ = 0U;
            terminator_bytes_ = 0U;
            ++malformed_frames_;
            return Status::protocol_error;
        }
    }

    if (byte == kTerminator) {
        ++terminator_bytes_;
        if (terminator_bytes_ == 3U) {
            terminator_bytes_ = 0U;
            return finish_frame();
        }
        return Status::ok;
    }

    Status result = Status::ok;
    while (terminator_bytes_ > 0U) {
        const Status append_status = append(kTerminator);
        if (!is_ok(append_status)) {
            result = append_status;
            break;
        }
        --terminator_bytes_;
    }
    terminator_bytes_ = 0U;
    if (is_ok(result)) {
        result = append(byte);
    }
    return result;
}

Status NextionParser::poll(
    hal::Uart& uart,
    const std::size_t byte_budget,
    std::size_t& bytes_read) noexcept {
    bytes_read = 0U;
    Status result = Status::ok;
    std::array<std::uint8_t, 16U> input{};

    while (bytes_read < byte_budget) {
        const std::size_t requested =
            std::min(input.size(), byte_budget - bytes_read);
        const std::size_t received = uart.read(input.data(), requested);
        if (received == 0U) {
            break;
        }
        if (received > requested) {
            ++malformed_frames_;
            return Status::io_error;
        }
        bytes_read += received;
        for (std::size_t index = 0U; index < received; ++index) {
            const Status byte_status = ingest(input[index]);
            if (is_ok(result) && !is_ok(byte_status)) {
                result = byte_status;
            }
        }
    }
    return result;
}

Status NextionParser::finish_frame() noexcept {
    if (frame_length_ == 0U) {
        ++malformed_frames_;
        return Status::protocol_error;
    }

    NextionEvent event{};
    event.response_code = frame_[0U];
    bool valid = true;

    switch (frame_[0U]) {
        case 0x01U:
            event.type = NextionEventType::acknowledgement;
            valid = frame_length_ == 1U;
            break;
        case 0x65U:
            event.type = NextionEventType::touch;
            valid = frame_length_ == 4U && frame_[3U] <= 1U;
            if (valid) {
                event.page_id = frame_[1U];
                event.component_id = frame_[2U];
                event.pressed = frame_[3U] != 0U;
            }
            break;
        case 0x66U:
            event.type = NextionEventType::page;
            valid = frame_length_ == 2U;
            if (valid) {
                event.page_id = frame_[1U];
            }
            break;
        case 0x70U:
            event.type = NextionEventType::string_value;
            valid = (frame_length_ - 1U) < event.text.size();
            if (valid) {
                event.text_length = frame_length_ - 1U;
                for (std::size_t index = 0U; index < event.text_length; ++index) {
                    event.text[index] = static_cast<char>(frame_[index + 1U]);
                }
                event.text[event.text_length] = '\0';
            }
            break;
        case 0x71U:
            event.type = NextionEventType::numeric_value;
            valid = frame_length_ == 5U;
            if (valid) {
                event.numeric_value =
                    static_cast<std::uint32_t>(frame_[1U]) |
                    (static_cast<std::uint32_t>(frame_[2U]) << 8U) |
                    (static_cast<std::uint32_t>(frame_[3U]) << 16U) |
                    (static_cast<std::uint32_t>(frame_[4U]) << 24U);
            }
            break;
        case 0x88U:
            event.type = NextionEventType::ready;
            valid = frame_length_ == 1U;
            break;
        default:
            event.type = is_error_response(frame_[0U])
                ? NextionEventType::error
                : NextionEventType::unknown;
            valid = frame_length_ == 1U;
            break;
    }

    frame_length_ = 0U;
    if (!valid) {
        ++malformed_frames_;
        return Status::protocol_error;
    }
    if (!events_.push(event)) {
        ++dropped_events_;
        return Status::queue_full;
    }
    return Status::ok;
}

bool NextionParser::pop(NextionEvent& event) noexcept {
    return events_.pop(event);
}

void NextionParser::reset() noexcept {
    frame_length_ = 0U;
    terminator_bytes_ = 0U;
    discarding_ = false;
    events_.clear();
}

std::size_t NextionParser::pending_events() const noexcept {
    return events_.size();
}

std::uint32_t NextionParser::malformed_frames() const noexcept {
    return malformed_frames_;
}

std::uint32_t NextionParser::dropped_events() const noexcept {
    return dropped_events_;
}

Status NextionCommandQueue::enqueue(
    const char* const command,
    const std::size_t length) noexcept {
    if (command == nullptr || length == 0U || length > kNextionCommandCapacity) {
        return Status::invalid_argument;
    }

    NextionCommand next{};
    for (std::size_t index = 0U; index < length; ++index) {
        const auto byte = static_cast<std::uint8_t>(command[index]);
        if (byte == kTerminator || byte < 0x20U || byte > 0x7EU) {
            return Status::invalid_argument;
        }
        next.bytes[index] = byte;
    }
    next.length = length;
    return commands_.push(next) ? Status::ok : Status::queue_full;
}

Status NextionCommandQueue::enqueue_page(const std::uint8_t page_id) noexcept {
    std::array<char, 16U> command{};
    const int count = std::snprintf(
        command.data(), command.size(), "page %u", static_cast<unsigned>(page_id));
    if (count <= 0 || static_cast<std::size_t>(count) >= command.size()) {
        return Status::invalid_argument;
    }
    return enqueue(command.data(), static_cast<std::size_t>(count));
}

bool NextionCommandQueue::valid_identifier(const char* const value) noexcept {
    if (value == nullptr || value[0U] == '\0') {
        return false;
    }
    for (std::size_t index = 0U; value[index] != '\0'; ++index) {
        const char character = value[index];
        const bool alpha_numeric =
            (character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9');
        if (!alpha_numeric && character != '_' && character != '.') {
            return false;
        }
    }
    return true;
}

bool NextionCommandQueue::valid_text(const char* const value) noexcept {
    if (value == nullptr) {
        return false;
    }
    for (std::size_t index = 0U; value[index] != '\0'; ++index) {
        const auto character = static_cast<unsigned char>(value[index]);
        if (character < 0x20U || character > 0x7EU || character == '"' ||
            character == '\\') {
            return false;
        }
    }
    return true;
}

Status NextionCommandQueue::enqueue_numeric(
    const char* const component,
    const std::uint32_t value) noexcept {
    if (!valid_identifier(component)) {
        return Status::invalid_argument;
    }
    std::array<char, kNextionCommandCapacity + 1U> command{};
    const int count = std::snprintf(
        command.data(),
        command.size(),
        "%s.val=%lu",
        component,
        static_cast<unsigned long>(value));
    if (count <= 0 || static_cast<std::size_t>(count) > kNextionCommandCapacity) {
        return Status::invalid_argument;
    }
    return enqueue(command.data(), static_cast<std::size_t>(count));
}

Status NextionCommandQueue::enqueue_text(
    const char* const component,
    const char* const text) noexcept {
    if (!valid_identifier(component) || !valid_text(text)) {
        return Status::invalid_argument;
    }
    std::array<char, kNextionCommandCapacity + 1U> command{};
    const int count = std::snprintf(
        command.data(), command.size(), "%s.txt=\"%s\"", component, text);
    if (count <= 0 || static_cast<std::size_t>(count) > kNextionCommandCapacity) {
        return Status::invalid_argument;
    }
    return enqueue(command.data(), static_cast<std::size_t>(count));
}

Status NextionCommandQueue::send_next(hal::Uart& uart) noexcept {
    if (!has_in_flight_) {
        if (!commands_.pop(in_flight_)) {
            return Status::busy;
        }
        has_in_flight_ = true;
    }

    std::array<std::uint8_t, kNextionCommandCapacity + 3U> encoded{};
    std::copy_n(in_flight_.bytes.begin(), in_flight_.length, encoded.begin());
    encoded[in_flight_.length] = kTerminator;
    encoded[in_flight_.length + 1U] = kTerminator;
    encoded[in_flight_.length + 2U] = kTerminator;
    const Status status =
        uart.write(encoded.data(), in_flight_.length + 3U);
    if (is_ok(status)) {
        has_in_flight_ = false;
    }
    return status;
}

void NextionCommandQueue::clear() noexcept {
    commands_.clear();
    has_in_flight_ = false;
}

std::size_t NextionCommandQueue::pending() const noexcept {
    return commands_.size() + (has_in_flight_ ? 1U : 0U);
}

}  // namespace gravimetra::hmi
