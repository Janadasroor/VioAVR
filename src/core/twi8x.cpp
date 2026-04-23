#include "vioavr/core/twi8x.hpp"
#include "vioavr/core/port_mux.hpp"
#include <algorithm>
#include <vector>
#include "vioavr/core/evsys.hpp"

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
        for (u64 i = 0; i < elapsed_cycles; ++i) {
            if (bits_left_ > 0) {
                cycle_counter_++;
                
                u64 half = bit_duration_ / 2;
                bool scl = (cycle_counter_ >= half);
                
                PinLevel sda_level = PinLevel::high;
                bool sda_enabled = true;
                
                if (cycle_counter_ < half) {
                    // SCL is Low, SDA can change
                    if (bits_left_ > 1) {
                        // Data bits 7..0 (MSB first)
                        bool sda = (shift_register_ >> (bits_left_ - 2)) & 1;
                        sda_level = sda ? PinLevel::high : PinLevel::low;
                    } else {
                        // ACK bit (9th bit)
                        if (host_phase_ == TwiPhase::read_data) {
                            // Master drives ACK/NACK based on MCTRLB.ACKACT
                            bool nack = (mctrlb_ & 0x01U); // 0=ACK (Low), 1=NACK (High)
                            sda_level = nack ? PinLevel::high : PinLevel::low;
                        } else {
                            // Master releases SDA to read slave ACK
                            sda_enabled = false;
                        }
                    }
                } else if (cycle_counter_ == half + (half / 2)) {
                    // SCL is High, sample SDA
                    if (bits_left_ > 1) {
                        if (host_phase_ == TwiPhase::read_data) {
                            // Sample data bit from slave
                            if (port_mux_->get_twi_sda_level(desc_.index) == PinLevel::high) {
                                data_read_accumulator_ |= (1 << (bits_left_ - 2));
                            }
                        }
                    } else {
                        // Sample ACK/NACK
                        if (host_phase_ != TwiPhase::read_data) {
                            PinLevel sample = port_mux_->get_twi_sda_level(desc_.index);
                            if (sample == PinLevel::high) mstatus_ |= MSTATUS_RXACK; // NACK
                            else mstatus_ &= ~MSTATUS_RXACK; // ACK
                        }
                    }
                }
                
                port_mux_->drive_twi_scl(desc_.index, scl ? PinLevel::high : PinLevel::low);
                port_mux_->drive_twi_sda(desc_.index, sda_level, sda_enabled);
                

                if (cycle_counter_ >= bit_duration_) {
                    cycle_counter_ = 0;
                    bits_left_--;
                    if (bits_left_ == 0) {
                        mstatus_ |= MSTATUS_CLKHOLD;
                        
                        if (host_phase_ == TwiPhase::address) {
                            bool is_read = (maddr_ & 0x01U);
                            if (is_read) {
                                mstatus_ |= MSTATUS_RIF;
                                host_phase_ = TwiPhase::read_data;
                            } else {
                                mstatus_ |= MSTATUS_WIF;
                                host_phase_ = TwiPhase::write_data;
                            }
                        } else if (host_phase_ == TwiPhase::read_data) {
                            mstatus_ |= MSTATUS_RIF;
                            mdata_ = data_read_accumulator_;
                        } else {
                            mstatus_ |= MSTATUS_WIF;
                        }
                        break;
                    }
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
    if (address == desc_.mctrla_address) {
        u8 old = mctrla_;
        mctrla_ = value;
        if (port_mux_) {
            if (!(value & 0x01U)) { // ENABLE
                port_mux_->drive_twi_sda(desc_.index, PinLevel::high, false);
                port_mux_->drive_twi_scl(desc_.index, PinLevel::high, false);
            } else if (!(old & 0x01U)) {
                port_mux_->drive_twi_sda(desc_.index, PinLevel::high, true);
                port_mux_->drive_twi_scl(desc_.index, PinLevel::high, true);
            }
        }
    }
    else if (address == desc_.mctrlb_address) mctrlb_ = value;
    else if (address == desc_.mstatus_address) {
        // Clear WIF/RIF/ARBLOST/BUSERR on write 1
        mstatus_ &= static_cast<u8>(~(value & 0xCCU));
        // Manual BUSSTATE override (to IDLE)
        if ((value & 0x03U) == 0x01U) mstatus_ = (mstatus_ & ~0x03U) | 0x01U;
    }
    else if (address == desc_.mbaud_address) mbaud_ = value;
    else if (address == desc_.maddr_address) {
            u8 state = mstatus_ & 0x03U;
            if ((mctrla_ & 0x01U) && (state == 0x00 || state == 0x01 || state == 0x02)) { // ENABLED and (UNKNOWN or IDLE or OWNER)
                maddr_ = value;
                mstatus_ &= ~0x03U;
                mstatus_ |= 0x02U; // OWNER
                
                // Drive Start condition
                if (port_mux_) {
                    port_mux_->drive_twi_scl(desc_.index, PinLevel::high);
                    port_mux_->drive_twi_sda(desc_.index, PinLevel::low);
                }

            // Calculate bit duration: 2 * (10 + BAUD)
            bit_duration_ = 2U * (10U + mbaud_);
            cycle_counter_ = 0;
            bits_left_ = 9; // 8 data + 1 ACK
            shift_register_ = value;
            host_phase_ = TwiPhase::address;
        }
    }
    else if (address == desc_.mdata_address) {
        mdata_ = value;
        mstatus_ &= ~MSTATUS_CLKHOLD;
        
        if (host_phase_ == TwiPhase::read_data) {
             // Continue reading
             bits_left_ = 9;
             cycle_counter_ = 0;
             data_read_accumulator_ = 0;
        } else {
            // Write data
            shift_register_ = value;
            bits_left_ = 9;
            cycle_counter_ = 0;
            host_phase_ = TwiPhase::write_data;
        }
    }
    else if (address == desc_.mctrlb_address) {
        mctrlb_ = value;
        u8 cmd = (value >> 2) & 0x03U;
        if (cmd == 0x03) { // STOP
             if (port_mux_) {
                 port_mux_->drive_twi_scl(desc_.index, PinLevel::high);
                 port_mux_->drive_twi_sda(desc_.index, PinLevel::high);
             }
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

void Twi8x::inject_bus_start() noexcept {
    // Reset slave state for new transaction
}

void Twi8x::inject_bus_address(u8 address) noexcept {
    if (!(sctrla_ & 0x01U)) return; // Slave disabled

    u8 target = address >> 1;
    u8 my_addr = saddr_ >> 1;
    u8 mask = saddrmask_ >> 1;

    if ((target & ~mask) == (my_addr & ~mask)) {
        // MATCH!
        sstatus_ |= SSTATUS_APIF | SSTATUS_AP | SSTATUS_CLKHOLD;
        if (address & 0x01U) sstatus_ |= SSTATUS_DIR; // Read
        else sstatus_ &= ~SSTATUS_DIR; // Write
    }
}

void Twi8x::inject_bus_data(u8 data) noexcept {
    if (!(sstatus_ & SSTATUS_AP)) return; // Not addressed

    sdata_ = data;
    sstatus_ |= SSTATUS_DIF | SSTATUS_CLKHOLD;
}

void Twi8x::inject_bus_stop() noexcept {
    if (sstatus_ & SSTATUS_AP) {
        sstatus_ |= SSTATUS_APIF; // Stop interrupt
        sstatus_ &= ~SSTATUS_AP;
    }
}

    
void Twi8x::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
}

} // namespace vioavr::core
