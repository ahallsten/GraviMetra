#include "gravimetra/system/config_store.hpp"

#include <array>
#include <limits>

namespace gravimetra::system {
namespace {

constexpr std::uint32_t kRecordMagic = 0x474D4346U;  // "GMCF"
constexpr std::uint16_t kEnvelopeVersion = 2U;
// Both envelope boundaries are whole STM32G4 doublewords. The commit block is
// written last, after the padded payload, so interrupted writes remain invalid.
constexpr std::size_t kHeaderSize = 32U;
constexpr std::size_t kCommitSize = 8U;
constexpr std::uint32_t kCommitMarker = 0xC04D17EDU;
constexpr std::size_t kMaximumProgramAlignment = 8U;
constexpr std::size_t kProgramChunkSize = 32U;

[[nodiscard]] std::uint16_t read_u16(
    const std::uint8_t* const data) noexcept {
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(data[0U]) |
        static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(data[1U]) << 8U));
}

[[nodiscard]] std::uint32_t read_u32(
    const std::uint8_t* const data) noexcept {
    return static_cast<std::uint32_t>(data[0U]) |
        (static_cast<std::uint32_t>(data[1U]) << 8U) |
        (static_cast<std::uint32_t>(data[2U]) << 16U) |
        (static_cast<std::uint32_t>(data[3U]) << 24U);
}

void write_u16(std::uint8_t* const data, const std::uint16_t value) noexcept {
    data[0U] = static_cast<std::uint8_t>(value & 0xFFU);
    data[1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

void write_u32(std::uint8_t* const data, const std::uint32_t value) noexcept {
    data[0U] = static_cast<std::uint8_t>(value & 0xFFU);
    data[1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[2U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[3U] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

[[nodiscard]] std::uint32_t crc32_update(
    std::uint32_t crc,
    const std::uint8_t* const data,
    const std::size_t length) noexcept {
    for (std::size_t index = 0U; index < length; ++index) {
        crc ^= data[index];
        for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
            const std::uint32_t mask =
                (crc & 1U) != 0U ? 0xFFFFFFFFU : 0U;
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return crc;
}

[[nodiscard]] bool all_erased(
    const std::uint8_t* const data,
    const std::size_t length) noexcept {
    for (std::size_t index = 0U; index < length; ++index) {
        if (data[index] != 0xFFU) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] constexpr bool is_power_of_two(
    const std::size_t value) noexcept {
    return value != 0U && (value & (value - 1U)) == 0U;
}

[[nodiscard]] constexpr std::size_t round_up(
    const std::size_t value,
    const std::size_t alignment) noexcept {
    return (value + alignment - 1U) & ~(alignment - 1U);
}

}  // namespace

std::uint32_t crc32_ieee(
    const std::uint8_t* const data,
    const std::size_t length) noexcept {
    if (data == nullptr && length != 0U) {
        return 0U;
    }
    return ~crc32_update(0xFFFFFFFFU, data, length);
}

AtomicConfigStore::AtomicConfigStore(
    hal::NonvolatileStorage& storage,
    const ConfigStoreLayout& layout) noexcept
    : storage_(storage), layout_(layout) {}

bool AtomicConfigStore::layout_valid() const noexcept {
    if (!is_power_of_two(layout_.program_alignment) ||
        layout_.program_alignment > kMaximumProgramAlignment ||
        !is_power_of_two(layout_.erase_alignment) ||
        layout_.erase_alignment < layout_.program_alignment ||
        (layout_.erase_alignment % layout_.program_alignment) != 0U ||
        layout_.slot_size < (kHeaderSize + kCommitSize) ||
        (layout_.offset % layout_.erase_alignment) != 0U ||
        (layout_.slot_size % layout_.erase_alignment) != 0U ||
        (layout_.offset % layout_.program_alignment) != 0U ||
        (layout_.slot_size % layout_.program_alignment) != 0U ||
        (kHeaderSize % layout_.program_alignment) != 0U ||
        (kCommitSize % layout_.program_alignment) != 0U) {
        return false;
    }
    const std::size_t storage_size = storage_.size();
    if (layout_.offset > storage_size) {
        return false;
    }
    const std::size_t available = storage_size - layout_.offset;
    return layout_.slot_size <= (available / 2U);
}

std::size_t AtomicConfigStore::slot_offset(const std::uint8_t slot) const noexcept {
    return layout_.offset + (static_cast<std::size_t>(slot) * layout_.slot_size);
}

std::size_t AtomicConfigStore::maximum_payload_size() const noexcept {
    return layout_valid()
        ? layout_.slot_size - kHeaderSize - kCommitSize
        : 0U;
}

bool AtomicConfigStore::generation_is_newer(
    const std::uint32_t candidate,
    const std::uint32_t reference) noexcept {
    const std::uint32_t distance = candidate - reference;
    return distance != 0U && distance < 0x80000000U;
}

Status AtomicConfigStore::inspect_slot(
    const std::uint8_t slot,
    SlotInfo& info) noexcept {
    info = SlotInfo{};
    if (!layout_valid() || slot > 1U) {
        return Status::invalid_argument;
    }

    std::array<std::uint8_t, kHeaderSize> header{};
    std::array<std::uint8_t, kCommitSize> marker{};
    const std::size_t base = slot_offset(slot);
    Status status = storage_.read(base, header.data(), header.size());
    if (!is_ok(status)) {
        return status;
    }
    status = storage_.read(
        base + layout_.slot_size - marker.size(), marker.data(), marker.size());
    if (!is_ok(status)) {
        return status;
    }

    if (all_erased(header.data(), header.size()) &&
        all_erased(marker.data(), marker.size())) {
        return Status::ok;
    }
    info.present = true;
    if (read_u32(marker.data()) != kCommitMarker ||
        read_u32(&marker[4U]) != ~kCommitMarker ||
        read_u32(&header[0U]) != kRecordMagic ||
        read_u16(&header[4U]) != kEnvelopeVersion ||
        read_u16(&header[6U]) != kHeaderSize ||
        read_u32(&header[24U]) != crc32_ieee(header.data(), 24U) ||
        read_u32(&header[28U]) != ~read_u32(&header[24U])) {
        return Status::ok;
    }

    info.schema_version = read_u32(&header[8U]);
    info.generation = read_u32(&header[12U]);
    info.payload_length = read_u32(&header[16U]);
    info.payload_crc = read_u32(&header[20U]);
    if (info.payload_length > maximum_payload_size()) {
        return Status::ok;
    }

    std::array<std::uint8_t, 32U> chunk{};
    std::uint32_t crc = 0xFFFFFFFFU;
    std::size_t checked = 0U;
    while (checked < info.payload_length) {
        const std::size_t remaining = info.payload_length - checked;
        const std::size_t count = remaining < chunk.size()
            ? remaining
            : chunk.size();
        status = storage_.read(
            base + kHeaderSize + checked, chunk.data(), count);
        if (!is_ok(status)) {
            return status;
        }
        crc = crc32_update(crc, chunk.data(), count);
        checked += count;
    }
    info.integrity_valid = (~crc == info.payload_crc);
    return Status::ok;
}

Status AtomicConfigStore::load(
    const std::uint32_t expected_schema_version,
    std::uint8_t* const destination,
    const std::size_t capacity,
    std::size_t& payload_length,
    ConfigLoadInfo* const info) noexcept {
    payload_length = 0U;
    if (info != nullptr) {
        *info = ConfigLoadInfo{};
    }
    if (!layout_valid()) {
        return Status::invalid_argument;
    }

    std::array<SlotInfo, 2U> slots{};
    for (std::uint8_t index = 0U; index < 2U; ++index) {
        const Status status = inspect_slot(index, slots[index]);
        if (!is_ok(status)) {
            return status;
        }
    }

    const bool valid_zero = slots[0U].integrity_valid;
    const bool valid_one = slots[1U].integrity_valid;
    if (!valid_zero && !valid_one) {
        const bool corruption =
            (slots[0U].present && !slots[0U].integrity_valid) ||
            (slots[1U].present && !slots[1U].integrity_valid);
        return corruption ? Status::verification_failed : Status::not_configured;
    }

    std::uint8_t selected = 0U;
    if (!valid_zero || (valid_one && generation_is_newer(
            slots[1U].generation, slots[0U].generation))) {
        selected = 1U;
    }
    const SlotInfo& chosen = slots[selected];
    // Never silently roll back to an older configuration merely because its
    // schema happens to match. Migration policy belongs above this envelope.
    if (chosen.schema_version != expected_schema_version) {
        return Status::not_configured;
    }
    if ((chosen.payload_length != 0U && destination == nullptr) ||
        capacity < chosen.payload_length) {
        return Status::invalid_argument;
    }

    if (chosen.payload_length != 0U) {
        const Status status = storage_.read(
            slot_offset(selected) + kHeaderSize,
            destination,
            chosen.payload_length);
        if (!is_ok(status)) {
            return status;
        }
        if (crc32_ieee(destination, chosen.payload_length) != chosen.payload_crc) {
            return Status::verification_failed;
        }
    }
    payload_length = chosen.payload_length;
    if (info != nullptr) {
        const std::uint8_t other = selected == 0U ? 1U : 0U;
        info->generation = chosen.generation;
        info->slot_index = selected;
        info->recovered_from_redundant_copy =
            slots[other].present && !slots[other].integrity_valid;
    }
    return Status::ok;
}

Status AtomicConfigStore::save(
    const std::uint32_t schema_version,
    const std::uint8_t* const payload,
    const std::size_t payload_length) noexcept {
    if (!layout_valid() ||
        payload_length > maximum_payload_size() ||
        payload_length > std::numeric_limits<std::uint32_t>::max() ||
        (payload_length != 0U && payload == nullptr)) {
        return Status::invalid_argument;
    }

    std::array<SlotInfo, 2U> slots{};
    for (std::uint8_t index = 0U; index < 2U; ++index) {
        const Status status = inspect_slot(index, slots[index]);
        if (!is_ok(status)) {
            return status;
        }
    }

    bool have_newest = false;
    std::uint8_t newest = 0U;
    if (slots[0U].integrity_valid) {
        have_newest = true;
    }
    if (slots[1U].integrity_valid &&
        (!have_newest || generation_is_newer(
            slots[1U].generation, slots[0U].generation))) {
        newest = 1U;
        have_newest = true;
    }
    const std::uint8_t target = have_newest
        ? static_cast<std::uint8_t>(newest == 0U ? 1U : 0U)
        : 0U;
    const std::uint32_t generation = have_newest
        ? slots[newest].generation + 1U
        : 1U;

    std::array<std::uint8_t, kHeaderSize> header{};
    write_u32(&header[0U], kRecordMagic);
    write_u16(&header[4U], kEnvelopeVersion);
    write_u16(&header[6U], static_cast<std::uint16_t>(kHeaderSize));
    write_u32(&header[8U], schema_version);
    write_u32(&header[12U], generation);
    write_u32(&header[16U], static_cast<std::uint32_t>(payload_length));
    write_u32(&header[20U], crc32_ieee(payload, payload_length));
    write_u32(&header[24U], crc32_ieee(header.data(), 24U));
    write_u32(&header[28U], ~read_u32(&header[24U]));

    std::array<std::uint8_t, kCommitSize> marker{};
    write_u32(marker.data(), kCommitMarker);
    write_u32(&marker[4U], ~kCommitMarker);
    const std::size_t base = slot_offset(target);
    Status status = storage_.erase(base, layout_.slot_size);
    if (!is_ok(status)) {
        return status;
    }
    status = storage_.write(base, header.data(), header.size());
    if (!is_ok(status)) {
        return status;
    }
    std::array<std::uint8_t, kProgramChunkSize> chunk{};
    std::size_t written = 0U;
    while (written < payload_length) {
        chunk.fill(0xFFU);
        const std::size_t remaining = payload_length - written;
        const std::size_t count = remaining < chunk.size()
            ? remaining
            : chunk.size();
        for (std::size_t index = 0U; index < count; ++index) {
            chunk[index] = payload[written + index];
        }
        const std::size_t programmed = round_up(
            count, layout_.program_alignment);
        status = storage_.write(
            base + kHeaderSize + written, chunk.data(), programmed);
        if (!is_ok(status)) {
            return status;
        }
        written += count;
    }
    status = storage_.write(
        base + layout_.slot_size - marker.size(), marker.data(), marker.size());
    if (!is_ok(status)) {
        return status;
    }

    SlotInfo verified{};
    status = inspect_slot(target, verified);
    if (!is_ok(status)) {
        return status;
    }
    return verified.integrity_valid && verified.schema_version == schema_version &&
            verified.generation == generation &&
            verified.payload_length == payload_length
        ? Status::ok
        : Status::verification_failed;
}

}  // namespace gravimetra::system
