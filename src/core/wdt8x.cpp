#include "vioavr/core/wdt8x.hpp"
#include "vioavr/core/avr_cpu.hpp"

namespace vioavr::core {

Wdt8x::Wdt8x(const Wdt8xDescriptor& descriptor, AvrCpu& cpu) noexcept
    : desc_(descriptor), cpu_(cpu)
{
    u16 max_addr = desc_.ctrla_address;
    if (desc_.winctrla_address > max_addr) max_addr = desc_.winctrla_address;
    if (desc_.intctrl_address > max_addr) max_addr = desc_.intctrl_address;
    if (desc_.status_address > max_addr) max_addr = desc_.status_address;
    
    ranges_[0] = AddressRange{desc_.ctrla_address, max_addr};
}

std::span<const AddressRange> Wdt8x::mapped_ranges() const noexcept {
    return ranges_;
}

void Wdt8x::reset() noexcept {
    ctrla_ = 0x00U;
    winctrla_ = 0x00U;
    intctrl_ = 0x00U;
    status_ = 0x00U;
    enabled_ = false;
    elapsed_cycles_ = 0;
}

void Wdt8x::tick(u64 elapsed_cycles) noexcept {
    if (!enabled_) return;

    u64 prev_elapsed = elapsed_cycles_;
    elapsed_cycles_ += elapsed_cycles;

    // Check Early Warning
    // Simplification: EW occurs at 50% of the timeout for now.
    if (prev_elapsed < (timeout_cycles_ / 2) && elapsed_cycles_ >= (timeout_cycles_ / 2)) {
        status_ |= STATUS_EWIF;
    }

    if (elapsed_cycles_ >= timeout_cycles_) {
        cpu_.reset(ResetCause::watchdog);
        reset();
    }
}

u8 Wdt8x::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.winctrla_address) return winctrla_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.status_address) return status_;
    return 0U;
}

void Wdt8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
        update_timeout();
    }
    else if (address == desc_.winctrla_address) {
        winctrla_ = value;
        update_timeout();
    }
    else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    }
    else if (address == desc_.status_address) {
        // Clear EWIF by writing 1
        if (value & STATUS_EWIF) status_ &= ~STATUS_EWIF;
    }
}

void Wdt8x::reset_timer() noexcept {
    u8 window = (desc_.winctrla_address != 0) ? (winctrla_ & 0x0FU) : ((ctrla_ >> 4U) & 0x0FU);

    // Window mode check
    if (enabled_ && window != 0) {
        if (elapsed_cycles_ < window_cycles_) {
            // "WDR executed before the open window period" -> Immediate Reset
            cpu_.reset(ResetCause::watchdog);
            reset();
            return;
        }
    }
    elapsed_cycles_ = 0;
}

bool Wdt8x::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((intctrl_ & INTCTRL_EWIE) && (status_ & STATUS_EWIF)) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Wdt8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (request.vector_index == desc_.vector_index) {
        // EWIF is usually NOT cleared by hardware vectoring in AVR8X (Manual clear required)
        // But we return true to acknowledge it.
        return true;
    }
    return false;
}

void Wdt8x::update_timeout() noexcept {
    u8 period = ctrla_ & 0x0FU;
    u8 window = 0;

    if (desc_.winctrla_address != 0) {
        window = winctrla_ & 0x0FU;
    } else {
        // Window bits are 7:4 of CTRLA in Mega-0 series
        window = (ctrla_ >> 4U) & 0x0FU;
    }

    if (period == 0) {
        enabled_ = false;
        return;
    }
    
    enabled_ = true;
    // Base frequency is approx 32kHz WDTOSC.
    // period bits: 0=OFF, 1=8ms, 2=16ms, 3=32ms, 4=64ms, ... 11=8s
    // Cycles at 16MHz: 8ms = 128,000 cycles.
    // 2^0 * 128000? No, mapping is usually 2^(period-1) * 8ms.
    timeout_cycles_ = (1ULL << (period - 1)) * 128000ULL;
    
    if (window != 0) {
        // Window cycles are same mapping but for the CLOSED period.
        window_cycles_ = (1ULL << (window - 1)) * 128000ULL;
    } else {
        window_cycles_ = 0;
    }
}

} // namespace vioavr::core
