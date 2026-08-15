#pragma once

#include "gravimetra/common/status.hpp"

#include <cstddef>
#include <cstdint>

namespace gravimetra::hal {

class MonotonicClock {
public:
    virtual ~MonotonicClock() = default;
    [[nodiscard]] virtual std::uint64_t now_us() const noexcept = 0;
};

class DigitalInput {
public:
    virtual ~DigitalInput() = default;
    [[nodiscard]] virtual Status read(bool& asserted) noexcept = 0;
};

class DigitalOutput {
public:
    virtual ~DigitalOutput() = default;
    [[nodiscard]] virtual Status write(bool asserted) noexcept = 0;
};

class SpiBus {
public:
    virtual ~SpiBus() = default;
    [[nodiscard]] virtual Status transfer(
        const std::uint8_t* transmit,
        std::uint8_t* receive,
        std::size_t length,
        std::uint32_t timeout_us) noexcept = 0;
};

class I2cBus {
public:
    virtual ~I2cBus() = default;
    [[nodiscard]] virtual Status write(
        std::uint8_t address,
        const std::uint8_t* data,
        std::size_t length,
        std::uint32_t timeout_us) noexcept = 0;
    [[nodiscard]] virtual Status write_read(
        std::uint8_t address,
        const std::uint8_t* transmit,
        std::size_t transmit_length,
        std::uint8_t* receive,
        std::size_t receive_length,
        std::uint32_t timeout_us) noexcept = 0;
};

class Uart {
public:
    virtual ~Uart() = default;
    // On Status::ok the implementation has accepted the complete byte range
    // and copied it into storage owned by the UART/HAL layer. The caller may
    // immediately reuse or destroy the source buffer even when physical
    // transmission continues asynchronously. Implementations that cannot
    // guarantee that ownership transfer must return a non-ok status.
    [[nodiscard]] virtual Status write(
        const std::uint8_t* data,
        std::size_t length) noexcept = 0;
    [[nodiscard]] virtual std::size_t read(
        std::uint8_t* data,
        std::size_t capacity) noexcept = 0;
};

class StepPulseTimer {
public:
    virtual ~StepPulseTimer() = default;
    [[nodiscard]] virtual Status start(
        std::uint8_t channel,
        std::uint32_t frequency_hz,
        std::uint32_t pulse_count) noexcept = 0;
    virtual void stop(std::uint8_t channel) noexcept = 0;
    [[nodiscard]] virtual bool active(std::uint8_t channel) const noexcept = 0;
};

class NonvolatileStorage {
public:
    virtual ~NonvolatileStorage() = default;
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
    [[nodiscard]] virtual Status read(
        std::size_t offset,
        std::uint8_t* destination,
        std::size_t length) noexcept = 0;
    // write() and erase() are ordered durability barriers: Status::ok means
    // the requested bytes have reached nonvolatile media before a subsequent
    // operation begins. Backends must not acknowledge a RAM/cache-only write.
    // AtomicConfigStore additionally supplies geometry-aligned ranges.
    [[nodiscard]] virtual Status write(
        std::size_t offset,
        const std::uint8_t* source,
        std::size_t length) noexcept = 0;
    [[nodiscard]] virtual Status erase(
        std::size_t offset,
        std::size_t length) noexcept = 0;
};

class Watchdog {
public:
    virtual ~Watchdog() = default;
    virtual void refresh() noexcept = 0;
};

}  // namespace gravimetra::hal
