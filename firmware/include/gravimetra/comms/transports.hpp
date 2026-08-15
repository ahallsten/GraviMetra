#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gravimetra::comms {

class ByteTransport {
public:
    virtual ~ByteTransport() = default;
    // On Status::ok the transport owns a complete copy of the supplied bytes;
    // callers never retain an asynchronous buffer-lifetime obligation.
    [[nodiscard]] virtual Status send(
        const std::uint8_t* data,
        std::size_t length) noexcept = 0;
    [[nodiscard]] virtual std::size_t receive(
        std::uint8_t* destination,
        std::size_t capacity) noexcept = 0;
};

// A CDC implementation can expose its nonblocking byte stream through the
// existing UART-shaped HAL contract without coupling service messages to USB.
class UsbCdcTransport final : public ByteTransport {
public:
    explicit UsbCdcTransport(hal::Uart& port) noexcept;
    [[nodiscard]] Status send(
        const std::uint8_t* data,
        std::size_t length) noexcept override;
    [[nodiscard]] std::size_t receive(
        std::uint8_t* destination,
        std::size_t capacity) noexcept override;

private:
    hal::Uart& port_;
};

constexpr std::size_t kCanFdPayloadCapacity = 64U;

struct CanFrame {
    std::uint32_t identifier{0U};
    bool extended_identifier{false};
    bool flexible_data_rate{false};
    std::array<std::uint8_t, kCanFdPayloadCapacity> data{};
    std::size_t length{0U};
};

// Raw CAN bring-up abstraction only. Arbitration IDs, fragmentation, and the
// final robot protocol intentionally remain outside this module.
class CanPort {
public:
    virtual ~CanPort() = default;
    [[nodiscard]] virtual Status transmit(const CanFrame& frame) noexcept = 0;
    [[nodiscard]] virtual bool receive(CanFrame& frame) noexcept = 0;
};

class Rs485HalfDuplexTransport final : public ByteTransport {
public:
    Rs485HalfDuplexTransport(
        hal::Uart& port,
        hal::DigitalOutput& driver_enable) noexcept;

    [[nodiscard]] Status initialize_receive_mode() noexcept;
    // A timeout is deliberately not invented. Transmission remains disabled
    // until a hardware-derived nonzero deadline and monotonic clock are
    // configured by the application.
    [[nodiscard]] Status configure_transmit_timeout(
        const hal::MonotonicClock& clock,
        std::uint64_t transmit_complete_timeout_us) noexcept;
    [[nodiscard]] Status send(
        const std::uint8_t* data,
        std::size_t length) noexcept override;
    [[nodiscard]] std::size_t receive(
        std::uint8_t* destination,
        std::size_t capacity) noexcept override;

    // Call only from the target UART transmission-complete event. Lowering DE
    // immediately after a queued/asynchronous write can truncate the frame.
    [[nodiscard]] Status on_transmit_complete() noexcept;
    // Call from the communications-rate loop. An expired transmission makes a
    // best effort to return to receive mode and reports Status::timeout when
    // that deassertion succeeds.
    [[nodiscard]] Status service() noexcept;
    [[nodiscard]] Status abort_transmit() noexcept;
    [[nodiscard]] bool transmitting() const noexcept;
    [[nodiscard]] bool direction_state_known() const noexcept;
    [[nodiscard]] bool timeout_configured() const noexcept;

private:
    hal::Uart& port_;
    hal::DigitalOutput& driver_enable_;
    const hal::MonotonicClock* clock_{nullptr};
    std::uint64_t transmit_complete_timeout_us_{0U};
    std::uint64_t transmit_deadline_us_{0U};
    bool transmitting_{false};
    bool direction_state_known_{false};
};

}  // namespace gravimetra::comms
