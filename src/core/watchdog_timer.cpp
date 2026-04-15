#include "vioavr/core/watchdog_timer.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include <algorithm>

namespace vioavr::core {

namespace {
constexpr u8 kWdif = 0x80U;
constexpr u8 kWdie = 0x40U;
constexpr u8 kWdp3 = 0x20U;
constexpr u8 kWdce = 0x10U;
constexpr u8 kWde  = 0x08U;
constexpr u8 kWdpMask = 0x07U;

// Watchdog timeout is approximately based on a 128kHz oscillator.
// We'll assume a 16MHz CPU for cycle calculations: 125 CPU cycles per WDT cycle.
constexpr u32 kCpuCyclesPerWdtCycle = 125U;

const u32 kWdtTicks[] = {
    2048U,    // 0: 16ms
    4096U,    // 1: 32ms
    8192U,    // 2: 64ms
    16384U,   // 3: 0.125s
    32768U,   // 4: 0.25s
    65536U,   // 5: 0.5s
    131072U,  // 6: 1.0s
    262144U,  // 7: 2.0s
    524288U,  // 8: 4.0s
    1048576U  // 9: 8.0s
};
}

WatchdogTimer::WatchdogTimer(std::string_view name, const WdtDescriptor& desc, AvrCpu& cpu) noexcept
    : name_(name), desc_(desc), cpu_(cpu), ranges_{{{desc_.wdtcsr_address, desc_.wdtcsr_address}}}
{
}

std::string_view WatchdogTimer::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> WatchdogTimer::mapped_ranges() const noexcept
{
    return ranges_;
}

void WatchdogTimer::reset() noexcept
{
    wdtcsr_ = 0U;
    cycles_left_ = 0U;
    interrupt_pending_ = false;
}

void WatchdogTimer::tick(const u64 elapsed_cycles) noexcept
{
    if (timed_sequence_active_) {
        if (elapsed_cycles >= timed_sequence_cycles_left_) {
            timed_sequence_active_ = false;
            timed_sequence_cycles_left_ = 0;
            wdtcsr_ &= static_cast<u8>(~kWdce);
        } else {
            timed_sequence_cycles_left_ -= static_cast<u8>(elapsed_cycles);
        }
    }

    if ((wdtcsr_ & (desc_.wde_mask | desc_.wdie_mask)) == 0U) {
        return;
    }

    if (elapsed_cycles >= cycles_left_) {
        complete_timeout();
    } else {
        cycles_left_ -= static_cast<u32>(elapsed_cycles);
    }
}

u8 WatchdogTimer::read(const u16 address) noexcept
{
    if (address == desc_.wdtcsr_address) {
        return static_cast<u8>((wdtcsr_ & 0x7FU) | (interrupt_pending_ ? kWdif : 0U));
    }
    return 0U;
}

void WatchdogTimer::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.wdtcsr_address) {
        // Timed sequence start: WDCE and WDE must be set to 1
        if ((value & kWdce) != 0U && (value & desc_.wde_mask) != 0U) {
            timed_sequence_active_ = true;
            timed_sequence_cycles_left_ = 16; // 4 cycles in silicon, but we use a larger buffer for simulation stability
            wdtcsr_ |= kWdce;
            return;
        }

        if (timed_sequence_active_) {
            // Update WDE and WDP bits, WDCE is cleared
            wdtcsr_ = static_cast<u8>(value & static_cast<u8>(~kWdce));
            timed_sequence_active_ = false;
            timed_sequence_cycles_left_ = 0;
            reset_watchdog();
        } else {
            // Safety level 1/2 quirks: 
            // - WDIE can be changed anytime
            // - WDE can be SET anytime, but CLEARED only via timed sequence
            u8 new_wdtcsr = wdtcsr_;
            
            // Allow changing WDIE
            if (value & desc_.wdie_mask) new_wdtcsr |= desc_.wdie_mask;
            else new_wdtcsr &= static_cast<u8>(~desc_.wdie_mask);

            // Allow setting WDE
            if (value & desc_.wde_mask) new_wdtcsr |= desc_.wde_mask;
            
            // Allow clearing WDIF by writing 1
            if (value & kWdif) interrupt_pending_ = false;

            wdtcsr_ = new_wdtcsr;
            reset_watchdog();
        }
    }
}

bool WatchdogTimer::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if (interrupt_pending_ && (wdtcsr_ & desc_.wdie_mask) != 0U) {
        request = {desc_.vector_index, 0U};
        return true;
    }
    return false;
}

bool WatchdogTimer::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (pending_interrupt_request(request)) {
        // WDIF is NOT cleared by hardware according to datasheet, 
        // it is cleared by writing 1 to it.
        // Actually, most AVR flags ARE cleared when ISR is entered.
        // Let's re-check ATmega328P datasheet: 
        // "WDIF is cleared by hardware when executing the corresponding interrupt handling vector. 
        // Alternatively, WDIF is cleared by writing a logic one to the flag."
        // So my code was right to clear it, but maybe I cleared it and THEN checked in test?
        interrupt_pending_ = false;
        return true;
    }
    return false;
}

void WatchdogTimer::reset_watchdog() noexcept
{
    cycles_left_ = get_timeout_cycles();
}

void WatchdogTimer::complete_timeout() noexcept
{
    if ((wdtcsr_ & desc_.wdie_mask) != 0U) {
        interrupt_pending_ = true;
        // In Interrupt and Reset Mode, WDIE is cleared after interrupt.
        if ((wdtcsr_ & desc_.wde_mask) != 0U) {
            wdtcsr_ &= static_cast<u8>(~desc_.wdie_mask);
        }
    } else if ((wdtcsr_ & desc_.wde_mask) != 0U) {
        cpu_.reset();
    }
    reset_watchdog();
}

u32 WatchdogTimer::get_timeout_cycles() const noexcept
{
    u8 p = static_cast<u8>(((wdtcsr_ & kWdp3) >> 2U) | (wdtcsr_ & kWdpMask));
    if (p > 9U) p = 9U;
    return kWdtTicks[p] * kCpuCyclesPerWdtCycle;
}

}  // namespace vioavr::core
