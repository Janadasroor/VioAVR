#include "vioavr/core/lcd_controller.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <algorithm>

namespace vioavr::core {

LcdController::LcdController(std::string_view name, const LcdDescriptor& desc, PinMux& pin_mux) noexcept
    : name_(name), desc_(desc), pin_mux_(pin_mux)
{
    lcddr_.resize(desc_.lcddr_count, 0U);
    
    if (desc_.lcdcra_address != 0U) {
        ranges_.push_back({desc_.lcdcra_address, desc_.lcdccr_address});
    }
    if (desc_.lcddr_base_address != 0U) {
        ranges_.push_back({desc_.lcddr_base_address, static_cast<u16>(desc_.lcddr_base_address + desc_.lcddr_count - 1U)});
    }
    recalculate_timing();
}

std::string_view LcdController::name() const noexcept { return name_; }

std::span<const AddressRange> LcdController::mapped_ranges() const noexcept
{
    return {ranges_.data(), ranges_.size()};
}

void LcdController::reset() noexcept
{
    lcdcra_ = 0U;
    lcdcrb_ = 0U;
    lcdfrr_ = 0U;
    lcdccr_ = 0U;
    std::fill(lcddr_.begin(), lcddr_.end(), 0U);
    interrupt_pending_ = false;
    cycle_accumulator_ = 0U;
    recalculate_timing();
    update_pin_ownership();
}

void LcdController::tick(u64 elapsed_cycles) noexcept
{
    if ((lcdcra_ & kLcden) == 0U) {
        return;
    }

    cycle_accumulator_ += elapsed_cycles;
    
    if (cycle_accumulator_ >= cycles_per_frame_) {
        cycle_accumulator_ %= cycles_per_frame_;
        lcdcra_ |= kLcdif;
        if (lcdcra_ & kLcdie) {
            interrupt_pending_ = true;
        }
    }
}

u8 LcdController::read(u16 address) noexcept
{
    if (address == desc_.lcdcra_address) return lcdcra_;
    if (address == desc_.lcdcrb_address) return lcdcrb_;
    if (address == desc_.lcdfrr_address) return lcdfrr_;
    if (address == desc_.lcdccr_address) return lcdccr_;
    
    if (address >= desc_.lcddr_base_address && address < desc_.lcddr_base_address + desc_.lcddr_count) {
        return lcddr_[address - desc_.lcddr_base_address];
    }
    
    return 0U;
}

void LcdController::write(u16 address, u8 value) noexcept
{
    if (address == desc_.lcdcra_address) {
        if (value & kLcdif) {
            lcdcra_ &= static_cast<u8>(~kLcdif);
            interrupt_pending_ = false;
        }
        const u8 old_lcdcra = lcdcra_;
        lcdcra_ = (lcdcra_ & kLcdif) | (value & static_cast<u8>(~kLcdif));
        
        if ((old_lcdcra ^ lcdcra_) & kLcden) {
            update_pin_ownership();
        }
    } else if (address == desc_.lcdcrb_address) {
        const u8 old_lcdcrb = lcdcrb_;
        lcdcrb_ = value;
        // LCDPM bits 0-3 (0x0F) and LCDMUX bits 4-5 (0x30) affect ownership
        if ((old_lcdcrb ^ lcdcrb_) & 0x3FU) {
            update_pin_ownership();
        }
        if ((old_lcdcrb ^ lcdcrb_) & 0x80U) { // LCDCS
            recalculate_timing();
        }
        if ((old_lcdcrb ^ lcdcrb_) & kLcdmux) {
            recalculate_timing();
        }
    } else if (address == desc_.lcdfrr_address) {
        lcdfrr_ = value;
        recalculate_timing();
    } else if (address == desc_.lcdccr_address) {
        lcdccr_ = value;
    } else if (address >= desc_.lcddr_base_address && address < desc_.lcddr_base_address + desc_.lcddr_count) {
        lcddr_[address - desc_.lcddr_base_address] = value;
    }
}

bool LcdController::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if (interrupt_pending_) {
        request = {desc_.vector_index, 0U};
        return true;
    }
    return false;
}

bool LcdController::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (interrupt_pending_) {
        request = {desc_.vector_index, 0U};
        interrupt_pending_ = false;
        lcdcra_ &= static_cast<u8>(~kLcdif);
        return true;
    }
    return false;
}

void LcdController::update_pin_ownership() noexcept
{
    const bool enabled = (lcdcra_ & kLcden) != 0U;
    
    // LCDPM bits 0-3 determine number of segments (Table 24-3)
    const u8 lcdpm = lcdcrb_ & 0x0FU;
    static constexpr u8 kSegmentLimits[] = {
        13, 15, 17, 19, 21, 23, 24, 25, 27, 29, 31, 33, 35, 37, 39, 40
    };
    const u8 active_segments = kSegmentLimits[lcdpm];

    // Handle Common pins
    for (u8 i = 0; i < 4; ++i) {
        const auto& pin = desc_.common_pins[i];
        if (pin.port_address != 0U) {
            if (enabled) {
                pin_mux_.claim_pin_by_address(pin.port_address, pin.bit_index, PinOwner::lcd);
            } else {
                pin_mux_.release_pin_by_address(pin.port_address, pin.bit_index, PinOwner::lcd);
            }
        }
    }

    // Handle Segment pins (Up to 40)
    for (u8 i = 0; i < 40; ++i) {
        const auto& pin = desc_.segment_pins[i];
        if (pin.port_address != 0U) {
            if (enabled && i < active_segments) {
                pin_mux_.claim_pin_by_address(pin.port_address, pin.bit_index, PinOwner::lcd);
            } else {
                pin_mux_.release_pin_by_address(pin.port_address, pin.bit_index, PinOwner::lcd);
            }
        }
    }
}

void LcdController::recalculate_timing() noexcept
{
    // Duty denominator D (LCDMUX bits 5:4 in LCDCRB)
    const u8 mux = (lcdcrb_ >> 4) & 0x03U;
    u8 D = 1;
    switch (mux) {
        case 0: D = 1; break; // Static
        case 1: D = 2; break; // 1/2 Duty
        case 2: D = 3; break; // 1/3 Duty
        case 3: D = 4; break; // 1/4 Duty
    }

    // Prescaler P (LCDPS bits 6:4 in LCDFRR)
    const u8 ps = (lcdfrr_ >> 4) & 0x07U;
    static constexpr u16 kPrescalers[] = {16, 64, 128, 256, 512, 1024, 2048, 4096};
    const u16 P = kPrescalers[ps];

    // Clock Divider K (LCDCD bits 2:0 in LCDFRR)
    const u8 K = (lcdfrr_ & 0x07U) + 1;

    // Frame frequency = f_clk / (P * K * D)
    // So cycles per frame (in LCD clock cycles) = P * K * D
    u64 lcd_cycles = static_cast<u64>(P) * K * D;

    if (lcdcrb_ & 0x80U) { // LCDCS: Asynchronous Clock Select (32.768 kHz)
        cycles_per_frame_ = static_cast<u64>(lcd_cycles * (cpu_frequency_ / 32768.0));
    } else {
        cycles_per_frame_ = lcd_cycles;
    }
}

u8 LcdController::duty_cycle() const noexcept
{
    const u8 mux = (lcdcrb_ >> 4) & 0x03U;
    switch (mux) {
        case 0: return 1; // Static
        case 1: return 2; // 1/2 Duty
        case 2: return 3; // 1/3 Duty
        case 3: return 4; // 1/4 Duty
        default: return 1;
    }
}

u8 LcdController::active_segments() const noexcept
{
    const u8 lcdpm = lcdcrb_ & 0x0FU;
    static constexpr u8 kSegmentLimits[] = {
        13, 15, 17, 19, 21, 23, 24, 25, 27, 29, 31, 33, 35, 37, 39, 40
    };
    return kSegmentLimits[lcdpm];
}

} // namespace vioavr::core
