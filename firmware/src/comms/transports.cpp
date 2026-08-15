#include "gravimetra/comms/transports.hpp"

#include <limits>

namespace gravimetra::comms {

UsbCdcTransport::UsbCdcTransport(hal::Uart& port) noexcept : port_(port) {}

Status UsbCdcTransport::send(
    const std::uint8_t* const data,
    const std::size_t length) noexcept {
    if (data == nullptr || length == 0U) {
        return Status::invalid_argument;
    }
    return port_.write(data, length);
}

std::size_t UsbCdcTransport::receive(
    std::uint8_t* const destination,
    const std::size_t capacity) noexcept {
    if (destination == nullptr || capacity == 0U) {
        return 0U;
    }
    return port_.read(destination, capacity);
}

Rs485HalfDuplexTransport::Rs485HalfDuplexTransport(
    hal::Uart& port,
    hal::DigitalOutput& driver_enable) noexcept
    : port_(port), driver_enable_(driver_enable) {}

Status Rs485HalfDuplexTransport::initialize_receive_mode() noexcept {
    const Status status = driver_enable_.write(false);
    if (is_ok(status)) {
        transmitting_ = false;
        transmit_deadline_us_ = 0U;
        direction_state_known_ = true;
    } else {
        direction_state_known_ = false;
    }
    return status;
}

Status Rs485HalfDuplexTransport::configure_transmit_timeout(
    const hal::MonotonicClock& clock,
    const std::uint64_t transmit_complete_timeout_us) noexcept {
    if (transmit_complete_timeout_us == 0U || transmitting_) {
        return Status::invalid_argument;
    }
    clock_ = &clock;
    transmit_complete_timeout_us_ = transmit_complete_timeout_us;
    transmit_deadline_us_ = 0U;
    return Status::ok;
}

Status Rs485HalfDuplexTransport::send(
    const std::uint8_t* const data,
    const std::size_t length) noexcept {
    if (data == nullptr || length == 0U) {
        return Status::invalid_argument;
    }
    if (!direction_state_known_) {
        return Status::not_configured;
    }
    if (!timeout_configured()) {
        return Status::not_configured;
    }
    if (transmitting_) {
        return Status::busy;
    }
    Status status = driver_enable_.write(true);
    if (!is_ok(status)) {
        // A failed GPIO API call does not prove that the physical output
        // remained low. Block both transmit and receive until RX mode is
        // explicitly re-established.
        transmitting_ = true;
        direction_state_known_ = false;
        return status;
    }
    transmitting_ = true;
    direction_state_known_ = true;
    const std::uint64_t now = clock_->now_us();
    const std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max();
    transmit_deadline_us_ = transmit_complete_timeout_us_ > maximum - now
        ? maximum
        : now + transmit_complete_timeout_us_;
    status = port_.write(data, length);
    if (!is_ok(status)) {
        const Status disable_status = driver_enable_.write(false);
        if (is_ok(disable_status)) {
            transmitting_ = false;
            transmit_deadline_us_ = 0U;
            direction_state_known_ = true;
            return status;
        }
        // DE may still be asserted. Keep the transport blocked until an
        // explicit abort/initialization successfully establishes RX mode.
        transmitting_ = true;
        direction_state_known_ = false;
        return disable_status;
    }
    return Status::ok;
}

std::size_t Rs485HalfDuplexTransport::receive(
    std::uint8_t* const destination,
    const std::size_t capacity) noexcept {
    if (!direction_state_known_ || transmitting_ || destination == nullptr ||
        capacity == 0U) {
        return 0U;
    }
    return port_.read(destination, capacity);
}

Status Rs485HalfDuplexTransport::on_transmit_complete() noexcept {
    if (!transmitting_) {
        return Status::busy;
    }
    const Status status = driver_enable_.write(false);
    if (is_ok(status)) {
        transmitting_ = false;
        transmit_deadline_us_ = 0U;
        direction_state_known_ = true;
    } else {
        direction_state_known_ = false;
    }
    return status;
}

Status Rs485HalfDuplexTransport::service() noexcept {
    if (!timeout_configured()) {
        return Status::not_configured;
    }
    if (!transmitting_) {
        return Status::ok;
    }
    if (clock_->now_us() < transmit_deadline_us_) {
        return Status::busy;
    }

    const Status status = driver_enable_.write(false);
    if (!is_ok(status)) {
        direction_state_known_ = false;
        return status;
    }
    transmitting_ = false;
    transmit_deadline_us_ = 0U;
    direction_state_known_ = true;
    return Status::timeout;
}

Status Rs485HalfDuplexTransport::abort_transmit() noexcept {
    const Status status = driver_enable_.write(false);
    if (is_ok(status)) {
        transmitting_ = false;
        transmit_deadline_us_ = 0U;
        direction_state_known_ = true;
    } else {
        direction_state_known_ = false;
    }
    return status;
}

bool Rs485HalfDuplexTransport::transmitting() const noexcept {
    return transmitting_;
}

bool Rs485HalfDuplexTransport::direction_state_known() const noexcept {
    return direction_state_known_;
}

bool Rs485HalfDuplexTransport::timeout_configured() const noexcept {
    return clock_ != nullptr && transmit_complete_timeout_us_ != 0U;
}

}  // namespace gravimetra::comms
