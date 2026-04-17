#include "vioavr/core/pll.hpp"
#include <algorithm>

namespace vioavr::core {

PllController::PllController(u16 address) : address_(address) {
    ranges_[0] = {address, address};
}

std::span<const AddressRange> PllController::mapped_ranges() const noexcept {
    return ranges_;
}

void PllController::reset() noexcept {
    pllcsr_ = 0;
    lock_delay_remaining_ = 0;
}

void PllController::tick(u64 elapsed_cycles) noexcept {
    if (lock_delay_remaining_ > 0) {
        if (elapsed_cycles >= lock_delay_remaining_) {
            lock_delay_remaining_ = 0;
            pllcsr_ |= 0x01U; // PLOCK set
        } else {
            lock_delay_remaining_ -= elapsed_cycles;
        }
    }
}

u8 PllController::read(u16 address) noexcept {
    if (address == address_) return pllcsr_;
    return 0;
}

void PllController::write(u16 address, u8 value) noexcept {
    if (address == address_) {
        bool was_enabled = pllcsr_ & 0x02U;
        bool is_enabled = value & 0x02U;
        
        // Update R/W bits (PLLE=bit 1, PINDIV=bit 4)
        pllcsr_ = (value & 0x12U);
        
        if (!was_enabled && is_enabled) {
            // Started PLL
            lock_delay_remaining_ = LOCK_DELAY_CYCLES;
            pllcsr_ &= ~0x01U; // Ensure PLOCK is clear while locking
        } else if (!is_enabled) {
            // Disabled PLL
            lock_delay_remaining_ = 0;
            pllcsr_ &= ~0x01U; // Clear PLOCK
        }
    }
}

} // namespace vioavr::core
