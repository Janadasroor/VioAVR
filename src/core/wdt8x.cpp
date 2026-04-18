#include "vioavr/core/wdt8x.hpp"
#include "vioavr/core/avr_cpu.hpp"

namespace vioavr::core {

Wdt8x::Wdt8x(const Wdt8xDescriptor& descriptor, AvrCpu& cpu) noexcept
    : desc_(descriptor), cpu_(cpu)
{
    ranges_[0] = AddressRange{desc_.ctrla_address, desc_.status_address};
}

std::span<const AddressRange> Wdt8x::mapped_ranges() const noexcept {
    return ranges_;
}

void Wdt8x::reset() noexcept {
    ctrla_ = 0x00U;
    status_ = 0x00U;
    enabled_ = false;
    elapsed_cycles_ = 0;
}

void Wdt8x::tick(u64 elapsed_cycles) noexcept {
    if (!enabled_) return;

    elapsed_cycles_ += elapsed_cycles;
    if (elapsed_cycles_ >= timeout_cycles_) {
        cpu_.reset(ResetCause::watchdog);
        reset();
    }
}

u8 Wdt8x::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.status_address) return status_;
    return 0U;
}

void Wdt8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
        update_timeout();
    }
}

void Wdt8x::reset_timer() noexcept {
    elapsed_cycles_ = 0;
}

void Wdt8x::update_timeout() noexcept {
    u8 period = ctrla_ & 0x0FU;
    if (period == 0) {
        enabled_ = false;
        return;
    }
    
    enabled_ = true;
    // Timeout is approx 8ms * 2^period at 1MHz? 
    // In AVR8X, it's cycles of WDT OSC (typically 32kHz).
    // For simplicity, we scale to I/O cycles.
    timeout_cycles_ = (1ULL << (period + 3)) * 100; // Rough scaling
}

} // namespace vioavr::core
