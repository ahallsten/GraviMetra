#pragma once

#include "gravimetra/common/status.hpp"
#include "gravimetra/hal/interfaces.hpp"

#include <cstddef>
#include <cstdint>

namespace gravimetra::system {

struct ConfigStoreLayout {
    std::size_t offset{0U};
    std::size_t slot_size{0U};
    // Every program operation issued by AtomicConfigStore is aligned to this
    // granularity. STM32G4 internal flash requires 8-byte doubleword writes.
    std::size_t program_alignment{0U};
    // Each slot must occupy its own complete erase unit. For STM32G4 internal
    // flash this is the configured flash-page size, never a sub-page slice.
    std::size_t erase_alignment{0U};
};

struct ConfigLoadInfo {
    std::uint32_t generation{0U};
    std::uint8_t slot_index{0U};
    bool recovered_from_redundant_copy{false};
};

[[nodiscard]] std::uint32_t crc32_ieee(
    const std::uint8_t* data,
    std::size_t length) noexcept;

// Stores an application-defined serialized payload. The application schema
// version is separate from this envelope's format version. Callers must use a
// stable explicit serialization; raw compiler-dependent structs are discouraged.
class AtomicConfigStore {
public:
    AtomicConfigStore(
        hal::NonvolatileStorage& storage,
        const ConfigStoreLayout& layout) noexcept;

    [[nodiscard]] Status load(
        std::uint32_t expected_schema_version,
        std::uint8_t* destination,
        std::size_t capacity,
        std::size_t& payload_length,
        ConfigLoadInfo* info = nullptr) noexcept;

    [[nodiscard]] Status save(
        std::uint32_t schema_version,
        const std::uint8_t* payload,
        std::size_t payload_length) noexcept;

    [[nodiscard]] std::size_t maximum_payload_size() const noexcept;

private:
    struct SlotInfo {
        bool present{false};
        bool integrity_valid{false};
        std::uint32_t schema_version{0U};
        std::uint32_t generation{0U};
        std::size_t payload_length{0U};
        std::uint32_t payload_crc{0U};
    };

    [[nodiscard]] bool layout_valid() const noexcept;
    [[nodiscard]] std::size_t slot_offset(std::uint8_t slot) const noexcept;
    [[nodiscard]] Status inspect_slot(
        std::uint8_t slot,
        SlotInfo& info) noexcept;
    [[nodiscard]] static bool generation_is_newer(
        std::uint32_t candidate,
        std::uint32_t reference) noexcept;

    hal::NonvolatileStorage& storage_;
    ConfigStoreLayout layout_{};
};

}  // namespace gravimetra::system
