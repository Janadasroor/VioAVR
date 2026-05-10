#include "vioavr/core/io_peripheral.hpp"

#include "vioavr/core/memory_bus.hpp"

namespace vioavr::core {

void IoPeripheral::set_interrupt_pending(bool pending) noexcept {
    if (interrupt_pending_ == pending) return;
    interrupt_pending_ = pending;
    if (bus_ != nullptr) {
        bus_->notify_interrupt_state_change(this, pending);
    }
}

}  // namespace vioavr::core
