#include "vioavr/core/spi8x.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

Spi8x::Spi8x(const Spi8xDescriptor& descriptor) noexcept
    : desc_(descriptor)
{
    const std::array<u16, 5> addrs = {
        desc_.ctrla_address, desc_.ctrlb_address, desc_.intctrl_address,
        desc_.intflags_address, desc_.data_address
    };
    
    std::vector<u16> sorted;
    for (auto a : addrs) if (a != 0) sorted.push_back(a);
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());

    size_t ri = 0;
    ranges_.fill({0, 0});
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (ri > 0 && sorted[i] == ranges_[ri-1].end + 1) {
            ranges_[ri-1].end = sorted[i];
        } else {
            ranges_[ri++] = AddressRange{sorted[i], sorted[i]};
        }
    }
}

std::span<const AddressRange> Spi8x::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void Spi8x::reset() noexcept {
    ctrla_ = 0x00U;
    ctrlb_ = 0x00U;
    intctrl_ = 0x00U;
    intflags_ = 0x00U;
    data_ = 0x00U;
    transfer_in_progress_ = false;
    transfer_cycles_elapsed_ = 0;
}

void Spi8x::tick(u64 elapsed_cycles) noexcept {
    if (transfer_in_progress_) {
        transfer_cycles_elapsed_ += elapsed_cycles;
        if (transfer_cycles_elapsed_ >= transfer_duration_) {
            transfer_in_progress_ = false;
            intflags_ |= INTFLAGS_IF;
        }
    }
}

u8 Spi8x::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.data_address) {
        intflags_ &= ~INTFLAGS_IF;
        return data_;
    }
    return 0U;
}

void Spi8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) ctrla_ = value;
    else if (address == desc_.ctrlb_address) ctrlb_ = value;
    else if (address == desc_.intctrl_address) intctrl_ = value;
    else if (address == desc_.intflags_address) {
        if (value & INTFLAGS_IF) intflags_ &= ~INTFLAGS_IF;
    }
    else if (address == desc_.data_address) {
        data_ = value;
        if (ctrla_ & CTRLA_ENABLE) {
            intflags_ &= ~INTFLAGS_IF;
            transfer_in_progress_ = true;
            transfer_cycles_elapsed_ = 0;
            
            // Simplified duration based on CTRLA.PRESC and CLK2X
            // PRESC is bits 1-3. CLK2X is bit 7.
            // Mode: /2, /4, /8, /16, /32, /64, /128
            u32 divisor = 2; // Default
            u8 presc = (ctrla_ >> 1) & 0x07U;
            switch(presc) {
                case 0: divisor = 4; break;
                case 1: divisor = 16; break;
                case 2: divisor = 64; break;
                case 3: divisor = 128; break;
                // others are similar
            }
            if (ctrla_ & 0x80U) divisor /= 2;
            transfer_duration_ = divisor * 8;
        }
    }
}

bool Spi8x::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if ((intflags_ & INTFLAGS_IF) && (intctrl_ & INTCTRL_IE)) {
        request = {desc_.vector_index, 0U};
        return true;
    }
    return false;
}

bool Spi8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (!pending_interrupt_request(request)) return false;
    intflags_ &= ~INTFLAGS_IF;
    return true;
}

} // namespace vioavr::core
