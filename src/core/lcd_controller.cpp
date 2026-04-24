#include "vioavr/core/lcd_controller.hpp"
#include <algorithm>

namespace vioavr::core {

LcdController::LcdController(std::string_view name, const LcdDescriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    lcddr_.resize(desc_.lcddr_count, 0U);
    
    if (desc_.lcdcra_address != 0U) {
        ranges_.push_back({desc_.lcdcra_address, desc_.lcdccr_address});
    }
    if (desc_.lcddr_base_address != 0U) {
        ranges_.push_back({desc_.lcddr_base_address, static_cast<u16>(desc_.lcddr_base_address + desc_.lcddr_count - 1U)});
    }
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
}

void LcdController::tick(u64 elapsed_cycles) noexcept
{
    if ((lcdcra_ & kLcden) == 0U) {
        return;
    }

    // LCD Frame timing
    // Frame rate depends on LCDPS (prescaler) and LCDCD (divider) in LCDFRR
    // and clock source LCDCS in LCDCRB.
    // simplified for now: trigger Start of Frame interrupt periodically.
    cycle_accumulator_ += elapsed_cycles;
    
    // Placeholder: 32Hz @ 1MHz = 31250 cycles.
    // For 16MHz, that's 500,000 cycles per frame.
    // The exact timing should be: f_frame = f_clk / (K * N * D)
    // where K is bias/duty factor, N is prescaler, D is divider.
    if (cycle_accumulator_ >= 256000U) {
        cycle_accumulator_ = 0;
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
        // LCDIF is cleared by writing 1
        if (value & kLcdif) {
            lcdcra_ &= static_cast<u8>(~kLcdif);
            interrupt_pending_ = false;
        }
        // Other bits are standard RW, but preserve current LCDIF state if not being cleared
        lcdcra_ = (lcdcra_ & kLcdif) | (value & static_cast<u8>(~kLcdif));
    } else if (address == desc_.lcdcrb_address) {
        lcdcrb_ = value;
    } else if (address == desc_.lcdfrr_address) {
        lcdfrr_ = value;
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

} // namespace vioavr::core
