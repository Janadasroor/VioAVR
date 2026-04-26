#include "vioavr/core/twi8x.hpp"
#include "vioavr/core/port_mux.hpp"
#include <algorithm>
#include <vector>
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/logger.hpp"

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
    
    slave_phase_ = TwiSlavePhase::idle;
    slave_bits_left_ = 0;
    prev_scl_ = PinLevel::high;
    prev_sda_ = PinLevel::high;
    last_intended_sda_ = PinLevel::high;

    cycle_counter_ = 0;
    bit_duration_ = 20; // Default for MBAUD=0
    bits_left_ = 0;
    host_phase_ = TwiPhase::idle;
}

void Twi8x::tick(u64 elapsed_cycles) noexcept {
    if (!(sctrla_ & 0x01U) && !(mctrla_ & 0x01U)) return;

    for (u64 c = 0; c < elapsed_cycles; ++c) {
        PinLevel sda = port_mux_->get_twi_sda_level(desc_.index);
        PinLevel scl = port_mux_->get_twi_scl_level(desc_.index);
        
        // Signal driving (Slave & Master)
        PinLevel target_sda = PinLevel::high;
        bool do_ack = (sstatus_ & SSTATUS_AP) != 0;
        if (slave_phase_ == TwiSlavePhase::tx_data) {
            if (slave_bits_left_ > 0) {
                target_sda = (slave_shift_register_ >> (slave_bits_left_ - 1)) & 1 ? PinLevel::high : PinLevel::low;
            }
        } else if (slave_phase_ == TwiSlavePhase::ack_pulse || slave_phase_ == TwiSlavePhase::ack_setup) {
            if (do_ack) target_sda = (sctrlb_ & SCTRLB_ACKACT) ? PinLevel::high : PinLevel::low;
        }
        
        port_mux_->drive_twi_sda(desc_.index, target_sda, true);
        last_intended_sda_ = target_sda;

        // Slave Monitor
        if (sctrla_ & 0x01U) {
            if (scl == PinLevel::high && last_intended_sda_ == PinLevel::high) {
                if (prev_sda_ == PinLevel::high && sda == PinLevel::low) {
                    slave_phase_ = TwiSlavePhase::addr;
                    slave_bits_left_ = 8;
                    slave_shift_register_ = 0;
                    sstatus_ |= SSTATUS_AP;
                    sstatus_ &= ~(SSTATUS_DIF | SSTATUS_CLKHOLD | SSTATUS_RXACK | SSTATUS_DIR | SSTATUS_COLL | SSTATUS_BUSERR);
                    Logger::debug("TWI8X Slave START");
                }
                else if (prev_sda_ == PinLevel::low && sda == PinLevel::high) {
                    if (slave_phase_ != TwiSlavePhase::idle) {
                        sstatus_ |= SSTATUS_APIF;
                        sstatus_ &= ~SSTATUS_AP;
                        slave_phase_ = TwiSlavePhase::idle;
                        Logger::debug("TWI8X Slave STOP");
                    }
                }
            }

            // EDGE Detection
            if (prev_scl_ == PinLevel::low && scl == PinLevel::high) {
                // Rising: Sample bit
                if (slave_phase_ == TwiSlavePhase::addr || slave_phase_ == TwiSlavePhase::rx_data) {
                    slave_shift_register_ <<= 1;
                    if (sda == PinLevel::high) slave_shift_register_ |= 0x01;

                    if (slave_bits_left_ == 0) {
                        // All 8 bits inclusive of bit 0 just sampled.
                        if (slave_phase_ == TwiSlavePhase::addr) {
                            u8 addr = slave_shift_register_ >> 1;
                            u8 my_addr = saddr_ >> 1;
                            u8 mask = saddrmask_ >> 1;
                            if ((addr & ~mask) == (my_addr & ~mask)) {
                                sstatus_ |= (SSTATUS_APIF | SSTATUS_AP | SSTATUS_CLKHOLD);
                                if (slave_shift_register_ & 0x01) sstatus_ |= SSTATUS_DIR;
                                else sstatus_ &= ~SSTATUS_DIR;
                                slave_phase_ = TwiSlavePhase::ack_setup; // Transition now
                            } else {
                                slave_phase_ = TwiSlavePhase::idle; 
                                sstatus_ &= ~SSTATUS_AP;
                            }
                            } else if (slave_phase_ == TwiSlavePhase::rx_data) {
                                sdata_ = slave_shift_register_;
                                sstatus_ |= (SSTATUS_DIF | SSTATUS_CLKHOLD);
                            }
                    }
                }
 else if (slave_phase_ == TwiSlavePhase::ack_setup) {
                    slave_phase_ = TwiSlavePhase::ack_pulse;
                } else if (slave_phase_ == TwiSlavePhase::ack_pulse) {
                    if (sstatus_ & SSTATUS_DIR) {
                        if (sda == PinLevel::high) sstatus_ |= SSTATUS_RXACK;
                        else sstatus_ &= ~SSTATUS_RXACK;
                    }
                }
            } else if (prev_scl_ == PinLevel::high && scl == PinLevel::low) {
                // Falling: Prep/Decrement
                if (slave_phase_ == TwiSlavePhase::addr || slave_phase_ == TwiSlavePhase::rx_data || slave_phase_ == TwiSlavePhase::tx_data) {
                    if (slave_bits_left_ > 0) slave_bits_left_--;
                    else {
                        // Pulse 8 Falling
                        if (slave_phase_ == TwiSlavePhase::addr) {
                            // Already transitioned maybe, but just in case
                            slave_phase_ = TwiSlavePhase::ack_setup;
                        } else if (slave_phase_ == TwiSlavePhase::rx_data) {
                            if (sstatus_ & SSTATUS_DIF) {
                                slave_phase_ = TwiSlavePhase::ack_setup;
                            }
                        } else if (slave_phase_ == TwiSlavePhase::tx_data) {
                            slave_phase_ = TwiSlavePhase::ack_setup;
                        }
                    }
                }
                
                if (slave_phase_ == TwiSlavePhase::ack_pulse) {
                    if (sstatus_ & SSTATUS_DIR) {
                        if (sstatus_ & SSTATUS_RXACK) {
                             slave_phase_ = TwiSlavePhase::idle; 
                        } else {
                            slave_phase_ = TwiSlavePhase::tx_data;
                            slave_bits_left_ = 8;
                            slave_shift_register_ = sdata_;
                        }
                    } else {
                        slave_phase_ = TwiSlavePhase::rx_data;
                        slave_bits_left_ = 7;
                        slave_shift_register_ = 0;
                    }
                }
            }
        }

        // Master Core
        if (mctrla_ & 0x01U) tick_master_core();

        prev_scl_ = scl;
        prev_sda_ = sda;
    }
}

void Twi8x::tick_master_core() noexcept {
    if (mstatus_ & MSTATUS_CLKHOLD) return;

    cycle_counter_++;
    u64 half = bit_duration_ / 2;
    bool scl_high = (cycle_counter_ >= half);
    
    PinLevel sda_level = PinLevel::high;
    bool sda_enabled = true;
    
    if (cycle_counter_ < half) {
        if (bits_left_ > 1) {
            bool sda_bit = (shift_register_ >> (bits_left_ - 2)) & 1;
            sda_level = sda_bit ? PinLevel::high : PinLevel::low;
        } else {
            if (host_phase_ == TwiPhase::read_data) {
                bool nack = (mctrlb_ & MCTRLB_ACKACT);
                sda_level = nack ? PinLevel::high : PinLevel::low;
            } else sda_enabled = false;
        }
    } else if (cycle_counter_ == half + (half / 2)) {
        if (bits_left_ > 1) {
            if (host_phase_ == TwiPhase::read_data) {
                if (port_mux_->get_twi_sda_level(desc_.index) == PinLevel::high) {
                    data_read_accumulator_ |= (1 << (bits_left_ - 2));
                }
            }
        } else {
            if (host_phase_ != TwiPhase::read_data) {
                if (port_mux_->get_twi_sda_level(desc_.index) == PinLevel::high) mstatus_ |= MSTATUS_RXACK;
                else mstatus_ &= ~MSTATUS_RXACK;
            }
        }
    }
    
    port_mux_->drive_twi_scl(desc_.index, scl_high ? PinLevel::high : PinLevel::low);
    port_mux_->drive_twi_sda(desc_.index, sda_level, sda_enabled);

    if (cycle_counter_ >= bit_duration_) {
        cycle_counter_ = 0;
        bits_left_--;
        if (bits_left_ == 0) {
            mstatus_ |= MSTATUS_CLKHOLD;
            if (host_phase_ == TwiPhase::address || host_phase_ == TwiPhase::write_data) {
                mstatus_ &= ~(MSTATUS_RIF | MSTATUS_WIF);
                mstatus_ |= (maddr_ & 0x01U) ? MSTATUS_RIF : MSTATUS_WIF;
                if (maddr_ & 0x01U) {
                    host_phase_ = TwiPhase::read_data;
                    bits_left_ = 8; // Prepare to receive 8 bits of first byte
                }
            } else if (host_phase_ == TwiPhase::read_data) {
                mdata_ = data_read_accumulator_;
                mstatus_ &= ~MSTATUS_WIF;
                mstatus_ |= MSTATUS_RIF;
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
            if (mctrla_ & MCTRLA_SMEN) {
                bits_left_ = 9; // ACK bit + 8 bits
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
            if (!(value & 0x01U)) {
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
                last_intended_sda_ = PinLevel::high;
            }
            mstatus_ = (mstatus_ & ~MSTATUS_BUSSTATE_MASK) | 0x01U;
            mstatus_ &= ~(MSTATUS_WIF | MSTATUS_RIF | MSTATUS_CLKHOLD);
        } else if (cmd == MCTRLB_MCMD_REPSTART) {
            if (port_mux_) {
                port_mux_->drive_twi_scl(desc_.index, PinLevel::high);
                port_mux_->drive_twi_sda(desc_.index, PinLevel::low); // Repeated Start
                last_intended_sda_ = PinLevel::low;
            }
            mstatus_ &= ~(MSTATUS_WIF | MSTATUS_RIF | MSTATUS_CLKHOLD);
        } else if (cmd == MCTRLB_MCMD_RECVTRANS) {
            mstatus_ &= ~(MSTATUS_RIF | MSTATUS_CLKHOLD);
            bits_left_ = 9;
            cycle_counter_ = 0;
            data_read_accumulator_ = 0;
            host_phase_ = TwiPhase::read_data;
        }
    }
    else if (address == desc_.mstatus_address) {
        // Only allow writing to BUSSTATE and flags
        mstatus_ = (mstatus_ & ~0x03U) | (value & 0x03U);
        if (value & 0x80U) mstatus_ &= ~0x80U; // Clear RIF
        if (value & 0x40U) mstatus_ &= ~0x40U; // Clear WIF
        if (value & 0x20U) mstatus_ &= ~0x20U; // Clear CLKHOLD? Wait, MSTATUS.CLKHOLD is read-only or depends on RIF/WIF?
        // Actually, in many AVRs, writing 1 to flag clears it. 
        // And CLKHOLD is usually cleared when flags are cleared.
        if (!(mstatus_ & (MSTATUS_RIF | MSTATUS_WIF))) mstatus_ &= ~MSTATUS_CLKHOLD;
    }
    else if (address == desc_.mbaud_address) {
        mbaud_ = value;
        bit_duration_ = 2 * (10 + (u64)value);
    }
    else if (address == desc_.maddr_address) {
        maddr_ = value;
        if (port_mux_) {
            port_mux_->drive_twi_scl(desc_.index, PinLevel::high);
            port_mux_->drive_twi_sda(desc_.index, PinLevel::low); // Start
            last_intended_sda_ = PinLevel::low;
        }
        mstatus_ &= ~(MSTATUS_BUSSTATE_MASK | MSTATUS_RIF | MSTATUS_WIF | MSTATUS_CLKHOLD);
        mstatus_ |= 0x02U;
        bits_left_ = 9;
        cycle_counter_ = 0;
        shift_register_ = value;
        host_phase_ = TwiPhase::address;
    }
    else if (address == desc_.mdata_address) {
        mdata_ = value;
        mstatus_ &= ~(MSTATUS_WIF | MSTATUS_RIF | MSTATUS_CLKHOLD);
        if (host_phase_ == TwiPhase::read_data) {
             bits_left_ = 9;
             cycle_counter_ = 0;
             data_read_accumulator_ = 0;
        } else {
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
    bool rif = (mstatus_ & MSTATUS_RIF) && (mctrla_ & 0x80U);
    bool wif = (mstatus_ & MSTATUS_WIF) && (mctrla_ & 0x40U);
    if (rif || wif) {
        request = {desc_.master_vector_index, 0U};
        return true;
    }
    bool s_drif = (sstatus_ & SSTATUS_DIF) && (sctrla_ & 0x80U);
    bool s_apif = (sstatus_ & SSTATUS_APIF) && (sctrla_ & 0x40U);
    if (s_drif || s_apif) {
        request = {desc_.slave_vector_index, 0U};
        return true;
    }
    return false;
}

bool Twi8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    return true;
}

void Twi8x::inject_bus_start() noexcept {
    slave_phase_ = TwiSlavePhase::addr;
    slave_bits_left_ = 8;
    slave_shift_register_ = 0;
}

void Twi8x::inject_bus_address(u8 address) noexcept {
    inject_bus_start();
    slave_shift_register_ = address;
    u8 addr = address >> 1;
    u8 my_addr = saddr_ >> 1;
    u8 mask = saddrmask_ >> 1;
    if ((addr & ~mask) == (my_addr & ~mask)) {
        sstatus_ |= (SSTATUS_APIF | SSTATUS_AP | SSTATUS_CLKHOLD);
        if (address & 0x01) sstatus_ |= SSTATUS_DIR;
        else sstatus_ &= ~SSTATUS_DIR;
        slave_phase_ = TwiSlavePhase::ack_setup;
    } else {
        slave_phase_ = TwiSlavePhase::idle;
        sstatus_ &= ~SSTATUS_AP;
    }
}

void Twi8x::inject_bus_data(u8 data) noexcept {
    sdata_ = data;
    slave_shift_register_ = data;
    sstatus_ |= (SSTATUS_DIF | SSTATUS_CLKHOLD);
    slave_phase_ = TwiSlavePhase::ack_setup;
}

void Twi8x::inject_bus_stop() noexcept {
    sstatus_ |= SSTATUS_APIF;
    sstatus_ &= ~SSTATUS_AP;
    slave_phase_ = TwiSlavePhase::idle;
}

void Twi8x::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
}

void Twi8x::set_port_mux(PortMux* port_mux) noexcept {
    port_mux_ = port_mux;
}

} // namespace vioavr::core
