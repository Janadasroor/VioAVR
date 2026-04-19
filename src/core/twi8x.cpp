#include "vioavr/core/twi8x.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

Twi8x::Twi8x(const Twi8xDescriptor& descriptor) noexcept
    : desc_(descriptor)
{
    const std::array<u16, 13> addrs = {
        desc_.mctrla_address, desc_.mctrlb_address, desc_.mstatus_address,
        desc_.mbaud_address, desc_.maddr_address, desc_.mdata_address,
        desc_.sctrla_address, desc_.sctrlb_address, desc_.sstatus_address,
        desc_.saddr_address, desc_.sdata_address, desc_.saddrmask_address,
        desc_.dbgctrl_address
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

std::span<const AddressRange> Twi8x::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

void Twi8x::reset() noexcept {
    mctrla_ = 0x00U;
    mctrlb_ = 0x00U;
    mstatus_ = 0x00U;
    mbaud_ = 0x00U;
    maddr_ = 0x00U;
    mdata_ = 0x00U;
    sctrla_ = 0x00U;
    sctrlb_ = 0x00U;
    sstatus_ = 0x00U;
    saddr_ = 0x00U;
    sdata_ = 0x00U;
    saddrmask_ = 0x00U;
    dbgctrl_ = 0x00U;
}

void Twi8x::tick(u64 elapsed_cycles) noexcept {
    if (mstatus_ & MSTATUS_CLKHOLD) return;

    const u8 state = mstatus_ & MSTATUS_BUSSTATE_MASK;
    if (state == 0x02U) { // OWNER
        if (bits_left_ > 0) {
            cycle_counter_ += elapsed_cycles;
            if (cycle_counter_ >= bit_duration_) {
                cycle_counter_ = 0;
                bits_left_--;
                if (bits_left_ == 0) {
                    mstatus_ |= MSTATUS_CLKHOLD;
                    if (maddr_ & 0x01U) mstatus_ |= MSTATUS_RIF;
                    else mstatus_ |= MSTATUS_WIF;
                }
            }
        }
    }
}

u8 Twi8x::read(u16 address) noexcept {
    if (address == desc_.mctrla_address) return mctrla_;
    if (address == desc_.mctrlb_address) return mctrlb_;
    if (address == desc_.mstatus_address) return mstatus_;
    if (address == desc_.mbaud_address) return mbaud_;
    if (address == desc_.maddr_address) return maddr_;
    if (address == desc_.mdata_address) return mdata_;
    if (address == desc_.sctrla_address) return sctrla_;
    if (address == desc_.sctrlb_address) return sctrlb_;
    if (address == desc_.sstatus_address) return sstatus_;
    if (address == desc_.saddr_address) return saddr_;
    if (address == desc_.sdata_address) return sdata_;
    if (address == desc_.saddrmask_address) return saddrmask_;
    if (address == desc_.dbgctrl_address) return dbgctrl_;
    return 0U;
}

void Twi8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.mctrla_address) mctrla_ = value;
    else if (address == desc_.mctrlb_address) mctrlb_ = value;
    else if (address == desc_.mstatus_address) {
        // Clear WIF/RIF/ARBLOST/BUSERR on write 1
        mstatus_ &= static_cast<u8>(~(value & 0xCCU));
        // Manual BUSSTATE override (to IDLE)
        if ((value & 0x03U) == 0x01U) mstatus_ = (mstatus_ & ~0x03U) | 0x01U;
    }
    else if (address == desc_.mbaud_address) mbaud_ = value;
    else if (address == desc_.maddr_address) {
        maddr_ = value;
        const u8 state = mstatus_ & MSTATUS_BUSSTATE_MASK;
        
        if ((mctrla_ & 0x01U) && (state == 0x00 || state == 0x01 || state == 0x02)) { // ENABLED and (UNKNOWN or IDLE or OWNER)
            mstatus_ = (mstatus_ & ~MSTATUS_BUSSTATE_MASK) | 0x02U; // OWNER
            mstatus_ |= MSTATUS_CLKHOLD;
            
            // Calculate bit duration: 2 * (10 + BAUD)
            bit_duration_ = 2U * (10U + mbaud_);
            cycle_counter_ = 0;
            bits_left_ = 9; // 8 data + 1 ACK
            
            // Immediate response for simplified simulation
            if (maddr_ & 0x01U) mstatus_ |= MSTATUS_RIF;
            else mstatus_ |= MSTATUS_WIF;
        }
    }
    else if (address == desc_.mdata_address) {
        mdata_ = value;
        mstatus_ &= ~MSTATUS_CLKHOLD;
        
        // Start "transfer" of 8 bits
        cycle_counter_ = 0;
        bits_left_ = 9;
        
        // Set WIF immediately for synchronous test support
        if (!(maddr_ & 0x01U)) mstatus_ |= MSTATUS_WIF;
    }
    else if (address == desc_.mctrlb_address) {
        mctrlb_ = value;
        u8 cmd = (value >> 2) & 0x03U;
        if (cmd == 0x03) { // STOP
             mstatus_ = (mstatus_ & ~MSTATUS_BUSSTATE_MASK) | 0x01U; // Back to IDLE
             mstatus_ &= ~MSTATUS_CLKHOLD;
        }
    }
    else if (address == desc_.sctrla_address) sctrla_ = value;
    else if (address == desc_.sctrlb_address) sctrlb_ = value;
    else if (address == desc_.sstatus_address) {
        if (value & 0x80U) sstatus_ &= ~0x80U;
        if (value & 0x40U) sstatus_ &= ~0x40U;
    }
    else if (address == desc_.saddr_address) saddr_ = value;
    else if (address == desc_.sdata_address) sdata_ = value;
    else if (address == desc_.saddrmask_address) saddrmask_ = value;
    else if (address == desc_.dbgctrl_address) dbgctrl_ = value;
}

bool Twi8x::pending_interrupt_request(InterruptRequest& request) const noexcept {
    // Check Host interrupts
    if (((mstatus_ & 0xC0U) != 0) && (mctrla_ & 0xC0U)) {
        request = {desc_.master_vector_index, 0U};
        return true;
    }
    // Check Client interrupts
    if (((sstatus_ & 0xC0U) != 0) && (sctrla_ & 0xE0U)) {
        request = {desc_.slave_vector_index, 0U};
        return true;
    }
    return false;
}

bool Twi8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (!pending_interrupt_request(request)) return false;
    return true; // Flags cleared by write action in firmware
}

} // namespace vioavr::core
