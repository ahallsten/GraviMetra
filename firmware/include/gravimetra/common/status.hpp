#pragma once

#include <cstdint>

namespace gravimetra {

enum class Status : std::uint8_t {
    ok = 0,
    busy,
    timeout,
    invalid_argument,
    not_configured,
    io_error,
    protocol_error,
    verification_failed,
    interlock_violation,
    fault_active,
    queue_full,
};

[[nodiscard]] constexpr bool is_ok(const Status status) noexcept {
    return status == Status::ok;
}

}  // namespace gravimetra

