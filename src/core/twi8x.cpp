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
                            bool nack = (mctrlb_ & MCTRLB_ACKACT); // 0=ACK (Low), 4=NACK (High)
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
                        
                        if (host_phase_ == TwiPhase::address || host_phase_ == TwiPhase::write_data) {
                        // Sample ACK/NACK
                        bool nack = port_mux_->get_twi_sda_level(desc_.index) == PinLevel::high;
                        if (nack) mstatus_ |= MSTATUS_RXACK;
                        else      mstatus_ &= ~MSTATUS_RXACK;

                        if (host_phase_ == TwiPhase::address) {
                            if (maddr_ & 0x01U) { // Read
                                mstatus_ |= MSTATUS_RIF;
                                host_phase_ = TwiPhase::read_data;
                            } else { // Write
                                mstatus_ |= MSTATUS_WIF;
                            }
                        } else {
                            mstatus_ |= MSTATUS_WIF;
                        }
                        mstatus_ |= MSTATUS_CLKHOLD;
                    } else if (host_phase_ == TwiPhase::read_data) {
                        mdata_ = data_read_accumulator_;
                        mstatus_ |= MSTATUS_RIF;
                        mstatus_ |= MSTATUS_CLKHOLD;
                    }
                    bits_left_ = 0; // Waiting for software action
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
    if (address == desc_.mdata_address) {
        if (mstatus_ & MSTATUS_RIF) {
            mstatus_ &= ~MSTATUS_RIF;
            mstatus_ &= ~MSTATUS_CLKHOLD;
            
            // Smart Mode: Writing/Reading MDATA can trigger commands
            if (mctrla_ & MCTRLA_SMEN) {
                // If SMEN is 1, reading MDATA triggers a RECVTRANS (0x02)
                bits_left_ = 9;
                cycle_counter_ = 0;
                data_read_accumulator_ = 0;
                host_phase_ = TwiPhase::read_data;
            }
        }
        return mdata_;
    }
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
    else if (address == desc_.mctrlb_address) {
        mctrlb_ = value;
        u8 cmd = value & MCTRLB_MCMD_MASK;
        if (cmd == MCTRLB_MCMD_STOP) {
            if (port_mux_) {
                port_mux_->drive_twi_scl(desc_.index, PinLevel::high);
                port_mux_->drive_twi_sda(desc_.index, PinLevel::high);
            }
            mstatus_ = (mstatus_ & ~MSTATUS_BUSSTATE_MASK) | 0x01U; // Back to IDLE
            mstatus_ &= ~MSTATUS_CLKHOLD;
            host_phase_ = TwiPhase::idle;
        } else if (cmd == MCTRLB_MCMD_RECVTRANS) {
             mstatus_ &= ~(MSTATUS_RIF | MSTATUS_CLKHOLD);
             bits_left_ = 9;
             cycle_counter_ = 0;
             data_read_accumulator_ = 0;
             host_phase_ = TwiPhase::read_data;
        } else if (cmd == MCTRLB_MCMD_REPSTART) {
            // Issuing a repeated start
            if (port_mux_) {
                port_mux_->drive_twi_scl(desc_.index, PinLevel::high);
                port_mux_->drive_twi_sda(desc_.index, PinLevel::low);
            }
            mstatus_ &= ~MSTATUS_CLKHOLD;
            // Wait for MADDR write to actually start the address phase?
            // Actually, usually you write MADDR *after* REPSTART if you manually triggered it,
            // or MADDR write *is* the trigger.
        }
    }
    else if (address == desc_.mstatus_address) {
        // Clear WIF/RIF/ARBLOST/BUSERR on write 1
        mstatus_ &= static_cast<u8>(~(value & 0xCCU));
        // Manual BUSSTATE override (to IDLE)
        if ((value & 0x03U) == 0x01U) mstatus_ = (mstatus_ & ~0x03U) | 0x01U;
    }
    else if (address == desc_.mbaud_address) mbaud_ = value;
    else if (address == desc_.maddr_address) {
            if (mctrla_ & 0x01U) { // ENABLED
                u8 bus_state = mstatus_ & 0x03U;
                
                mstatus_ &= ~(MSTATUS_WIF | MSTATUS_RIF | MSTATUS_CLKHOLD);
                mstatus_ &= ~0x03U;
                mstatus_ |= 0x02U; // OWNER
                
                // Drive Start or Repeated Start condition
                if (port_mux_) {
                    if (bus_state == 0x02) { // Already OWNER, so this is a Repeated Start
                        // To ensure a valid Repeated Start, we must ensure SDA is High before driving it Low while SCL is High.
                        port_mux_->drive_twi_sda(desc_.index, PinLevel::high);
                        port_mux_->drive_twi_scl(desc_.index, PinLevel::high);
                        // Small delay effectively handled by next tick, but we can force state here.
                        port_mux_->drive_twi_sda(desc_.index, PinLevel::low);
                    } else {
                        // Regular Start from IDLE/UNKNOWN
                        port_mux_->drive_twi_scl(desc_.index, PinLevel::high);
                        port_mux_->drive_twi_sda(desc_.index, PinLevel::low);
                    }
                }

                maddr_ = value;
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
        mstatus_ &= ~(MSTATUS_WIF | MSTATUS_RIF | MSTATUS_CLKHOLD);
        
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
    bool rif = (mstatus_ & MSTATUS_RIF) && (mctrla_ & 0x80U); // RIEN = bit 7
    bool wif = (mstatus_ & MSTATUS_WIF) && (mctrla_ & 0x40U); // WIEN = bit 6

    if (rif || wif) {
        request = {desc_.master_vector_index, 0U};
        return true;
    }

    // Check Client interrupts
    bool s_drif = (sstatus_ & SSTATUS_DIF) && (sctrla_ & 0x80U); // DIEN = bit 7
    bool s_apif = (sstatus_ & SSTATUS_APIF) && (sctrla_ & 0x40U); // APIEN = bit 6

    if (s_drif || s_apif) {
        request = {desc_.slave_vector_index, 0U};
        return true;
    }
    return false;
}

bool Twi8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (request.vector_index == desc_.master_vector_index) {
        // TWI Host interrupts are usually cleared by hardware actions (writing MADDR/MDATA/MCTRLB)
        // but can be explicitly cleared here if needed.
        return true;
    }
    if (request.vector_index == desc_.slave_vector_index) {
        return true;
    }
    return false;
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
