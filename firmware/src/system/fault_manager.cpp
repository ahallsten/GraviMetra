#include "gravimetra/system/fault_manager.hpp"

namespace gravimetra::system {

bool FaultManager::valid_code(const FaultCode code) noexcept {
    return static_cast<std::uint8_t>(code) <
        static_cast<std::uint8_t>(FaultCode::count);
}

std::uint64_t FaultManager::code_mask(const FaultCode code) noexcept {
    return std::uint64_t{1U} << static_cast<std::uint8_t>(code);
}

void FaultManager::append_record(
    const FaultCode code,
    const bool became_active,
    const FaultSnapshot& snapshot) noexcept {
    std::size_t write_index = 0U;
    if (history_size_ < history_.size()) {
        write_index = (history_start_ + history_size_) % history_.size();
        ++history_size_;
    } else {
        write_index = history_start_;
        history_start_ = (history_start_ + 1U) % history_.size();
    }
    history_[write_index] = FaultRecord{
        next_sequence_, code, became_active, snapshot};
    ++next_sequence_;
}

Status FaultManager::raise(
    const FaultCode code,
    const FaultSnapshot& snapshot) noexcept {
    if (!valid_code(code)) {
        return Status::invalid_argument;
    }
    const std::uint64_t mask = code_mask(code);
    if ((active_mask_ & mask) != 0U) {
        return Status::ok;
    }
    active_mask_ |= mask;
    append_record(code, true, snapshot);
    return Status::ok;
}

Status FaultManager::clear(
    const FaultCode code,
    const FaultSnapshot& snapshot) noexcept {
    if (!valid_code(code)) {
        return Status::invalid_argument;
    }
    const std::uint64_t mask = code_mask(code);
    if ((active_mask_ & mask) == 0U) {
        return Status::ok;
    }
    active_mask_ &= ~mask;
    append_record(code, false, snapshot);
    return Status::ok;
}

Status FaultManager::clear_all_active(
    const FaultSnapshot& snapshot) noexcept {
    for (std::uint8_t index = 0U;
         index < static_cast<std::uint8_t>(FaultCode::count);
         ++index) {
        const auto code = static_cast<FaultCode>(index);
        const std::uint64_t mask = code_mask(code);
        if ((active_mask_ & mask) != 0U) {
            active_mask_ &= ~mask;
            append_record(code, false, snapshot);
        }
    }
    return Status::ok;
}

bool FaultManager::active(const FaultCode code) const noexcept {
    return valid_code(code) && (active_mask_ & code_mask(code)) != 0U;
}

bool FaultManager::any_active() const noexcept {
    return active_mask_ != 0U;
}

std::uint64_t FaultManager::active_mask() const noexcept {
    return active_mask_;
}

std::size_t FaultManager::history_size() const noexcept {
    return history_size_;
}

const FaultRecord* FaultManager::history_at(const std::size_t index) const noexcept {
    if (index >= history_size_) {
        return nullptr;
    }
    return &history_[(history_start_ + index) % history_.size()];
}

void FaultManager::clear_history() noexcept {
    history_start_ = 0U;
    history_size_ = 0U;
}

}  // namespace gravimetra::system
