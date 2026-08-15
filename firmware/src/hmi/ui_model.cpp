#include "gravimetra/hmi/ui_model.hpp"

namespace gravimetra::hmi {

Status UiRequestModel::bind(const UiBinding& binding) noexcept {
    if ((binding.request == UiRequestType::set_target &&
         !binding.value_from_next_numeric_response) ||
        (binding.value_from_next_numeric_response &&
         (binding.request != UiRequestType::set_target ||
          binding.maximum_value < binding.minimum_value))) {
        return Status::invalid_argument;
    }
    if (binding_count_ >= bindings_.size()) {
        return Status::queue_full;
    }
    for (std::size_t index = 0U; index < binding_count_; ++index) {
        const UiBinding& current = bindings_[index];
        if (current.page_id == binding.page_id &&
            current.component_id == binding.component_id &&
            current.edge == binding.edge) {
            return Status::invalid_argument;
        }
    }
    bindings_[binding_count_] = binding;
    ++binding_count_;
    return Status::ok;
}

Status UiRequestModel::accept(const NextionEvent& event) noexcept {
    if (event.type == NextionEventType::numeric_value) {
        if (!numeric_response_pending_) {
            return Status::not_configured;
        }
        const UiRequest pending = pending_numeric_request_;
        const bool in_range = event.numeric_value >= pending_minimum_value_ &&
            event.numeric_value <= pending_maximum_value_;
        cancel_pending_numeric_response();
        if (!in_range) {
            return Status::invalid_argument;
        }
        UiRequest completed = pending;
        completed.value = event.numeric_value;
        return request(completed);
    }
    if (event.type != NextionEventType::touch) {
        return Status::invalid_argument;
    }
    for (std::size_t index = 0U; index < binding_count_; ++index) {
        const UiBinding& binding = bindings_[index];
        const bool edge_matches = binding.edge == UiTouchEdge::either ||
            (event.pressed && binding.edge == UiTouchEdge::press) ||
            (!event.pressed && binding.edge == UiTouchEdge::release);
        if (binding.page_id == event.page_id &&
            binding.component_id == event.component_id && edge_matches) {
            if (binding.value_from_next_numeric_response) {
                if (numeric_response_pending_) {
                    return Status::busy;
                }
                pending_numeric_request_ = UiRequest{
                    binding.request,
                    event.page_id,
                    event.component_id,
                    0U};
                pending_minimum_value_ = binding.minimum_value;
                pending_maximum_value_ = binding.maximum_value;
                numeric_response_pending_ = true;
                return Status::ok;
            }
            return request(UiRequest{
                binding.request,
                event.page_id,
                event.component_id,
                event.numeric_value});
        }
    }
    return Status::not_configured;
}

Status UiRequestModel::request(const UiRequest& request_value) noexcept {
    return requests_.push(request_value) ? Status::ok : Status::queue_full;
}

bool UiRequestModel::pop(UiRequest& request_value) noexcept {
    return requests_.pop(request_value);
}

void UiRequestModel::clear_requests() noexcept {
    requests_.clear();
}

void UiRequestModel::cancel_pending_numeric_response() noexcept {
    pending_numeric_request_ = UiRequest{};
    pending_minimum_value_ = 0U;
    pending_maximum_value_ = 0U;
    numeric_response_pending_ = false;
}

std::size_t UiRequestModel::binding_count() const noexcept {
    return binding_count_;
}

std::size_t UiRequestModel::pending_requests() const noexcept {
    return requests_.size();
}

bool UiRequestModel::pending_numeric_request(
    UiRequest& request_value) const noexcept {
    if (!numeric_response_pending_) {
        return false;
    }
    request_value = pending_numeric_request_;
    return true;
}

}  // namespace gravimetra::hmi
