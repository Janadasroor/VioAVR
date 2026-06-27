#include "vioavr/core/usb.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <algorithm>

namespace vioavr::core {

Usb::Usb(std::string_view name, const UsbDescriptor& desc) noexcept
    : name_(name), desc_(desc) {
    size_t ri = 0;
    if (desc.uhwcon_address) ranges_[ri++] = {desc.uhwcon_address, desc.usbint_address};
    if (desc.udcon_address) ranges_[ri++] = {desc.udcon_address, desc.udmfn_address};
    if (desc.ueintx_address) ranges_[ri++] = {desc.ueintx_address, desc.ueint_address};
    num_ranges_ = ri;
    
    // Initialize FIFOs for endpoints
    for (auto& ep : endpoints_) {
        for (auto& bank : ep.banks) {
            bank.fifo.resize(64); // Small default size
        }
    }
}

std::span<const AddressRange> Usb::mapped_ranges() const noexcept {
    return {ranges_.data(), num_ranges_};
}

void Usb::reset() noexcept {
    uhwcon_ = 0;
    usbcon_ = 0;
    usbsta_ = 0;
    usbint_ = 0;
    udcon_ = 0;
    udint_ = 0;
    udien_ = 0;
    udaddr_ = 0;
    uenum_ = 0;
    uerst_ = 0;
    ueint_ = 0;
    cycle_accumulator_ = 0;
    frame_number_ = 0;
    
    update_pll_timing();

    for (auto& ep : endpoints_) {
        ep.control = 0;
        ep.config0 = 0;
        ep.config1 = 0;
        ep.status0 = 0;
        ep.status1 = 0;
        ep.interrupt_flags = 0;
        ep.interrupt_enable = 0;
        ep.cpu_bank = 0;
        ep.sie_bank = 0;
        ep.bank_count = 1;
        ep.data_toggle = false;
        for (auto& bank : ep.banks) {
            bank.read_idx = 0;
            bank.write_idx = 0;
            bank.byte_count = 0;
            bank.busy = false;
        }
    }
}

void Usb::update_pll_timing() noexcept {
    const u32 pll_freq = desc_.usb_pll_frequency_hz;
    pll_cycles_per_sof_ = pll_freq / 1000;
    if (bus_) {
        const u32 cpu_freq = bus_->device().cpu_frequency_hz;
        pll_scale_x256_ = static_cast<u32>((static_cast<u64>(pll_freq) * 256U) / cpu_freq);
    } else {
        pll_scale_x256_ = 256;
    }
}

void Usb::tick(u64 elapsed_cycles) noexcept {
    if (!(usbcon_ & desc_.usbcon_usbe_mask)) return; // USBE must be 1
    if (usbcon_ & desc_.usbcon_frzclk_mask) return; // FRZCLK must be 0
    
    // On many AVRs, USB clock requires PLL to be locked
    if (bus_ && desc_.pllcsr_address) {
        if (!(bus_->read_data(desc_.pllcsr_address) & 0x01U)) return; // PLOCK bit
    }

    cycle_accumulator_ += static_cast<u64>(elapsed_cycles) * pll_scale_x256_;
    if (pll_cycles_per_sof_ == 0) return;
    const u64 sof_threshold = static_cast<u64>(pll_cycles_per_sof_) * 256U;
    while (cycle_accumulator_ >= sof_threshold) {
        cycle_accumulator_ -= sof_threshold;
        
        frame_number_ = (frame_number_ + 1) & 0x7FFU;
        udint_ |= desc_.udint_sofi_mask; // SOFI
        if (bus_) bus_->set_interrupts_dirty();
    }
}

u8 Usb::read(u16 address) noexcept {
    if (address == desc_.uhwcon_address) return uhwcon_;
    if (address == desc_.usbcon_address) return usbcon_;
    if (address == desc_.usbsta_address) return usbsta_;
    if (address == desc_.usbint_address) return usbint_;
    if (address == desc_.udcon_address) return udcon_;
    if (address == desc_.udint_address) return udint_;
    if (address == desc_.udien_address) return udien_;
    if (address == desc_.udaddr_address) return udaddr_;
    if (address == desc_.udfnum_address) return static_cast<u8>(frame_number_ & 0xFFU);
    if (address == desc_.udfnum_address + 1) return static_cast<u8>((frame_number_ >> 8U) & 0x07U);
    if (address == desc_.uenum_address) return uenum_;
    if (address == desc_.uerst_address) return uerst_;
    if (address == desc_.ueint_address) return ueint_;

    u8 ep_idx = uenum_ & 0x07U;
    if (ep_idx >= endpoints_.size()) return 0;
    auto& ep = endpoints_[ep_idx];

    if (address == desc_.ueconx_address) return ep.control;
    if (address == desc_.uecfg0x_address) return ep.config0;
    if (address == desc_.uecfg1x_address) return ep.config1;
    if (address == desc_.uesta0x_address) return ep.status0;
    if (address == desc_.uesta1x_address) {
        u8 val = ep.status1 & ~0x07U; // Clear CURRBK and NBUSYBK
        val |= (ep.cpu_bank & 0x01U); // CURRBK
        u8 busy_count = 0;
        for (u8 i = 0; i < ep.bank_count; ++i) if (ep.banks[i].busy) busy_count++;
        val |= (busy_count << 1U); // NBUSYBK
        return val;
    }
    if (address == desc_.ueintx_address) {
        u8 flags = ep.interrupt_flags;
        auto& bank = ep.banks[ep.cpu_bank];
        // Calculate RWAL (Read/Write Allowed)
        bool is_in = is_in_endpoint(ep);
        bool rwal = false;
        if (is_in) {
            // IN: RWAL is 1 if current bank is not busy (CPU can write)
            rwal = !bank.busy;
        } else {
            // OUT: RWAL is 1 if current bank has data (CPU can read)
            rwal = (bank.byte_count > 0);
        }
        if (rwal) flags |= desc_.ueintx_rwal_mask;
        else flags &= ~desc_.ueintx_rwal_mask;

        return flags;
    }
    if (address == desc_.ueienx_address) return ep.interrupt_enable;
    if (address == desc_.uebclx_address) return static_cast<u8>(ep.banks[ep.cpu_bank].byte_count & 0xFFU);
    if (address == desc_.uebchx_address) return static_cast<u8>((ep.banks[ep.cpu_bank].byte_count >> 8U) & 0xFFU);
    
    if (address == desc_.uedatx_address) {
        auto& bank = ep.banks[ep.cpu_bank];
        if (bank.byte_count > 0) {
            u8 data = bank.fifo[bank.read_idx];
            bank.read_idx = (bank.read_idx + 1) % bank.fifo.size();
            bank.byte_count--;
            // ATmega32U4: RXSTPI auto-clears when last SETUP byte is read from UEDATX.
            // After RXSTPI clears on a CONTROL endpoint, the SIE sets TXINI to
            // signal the CPU that it can write IN data for the data phase.
            if (bank.byte_count == 0 && (ep.interrupt_flags & desc_.ueintx_rxstpi_mask)) {
                ep.interrupt_flags &= ~desc_.ueintx_rxstpi_mask;
                // Auto-assert TXINI for CONTROL endpoints (SIE behavior on ATmega32U4)
                if (is_in_endpoint(ep)) {
                    ep.interrupt_flags |= desc_.ueintx_txini_mask;
                }
                update_ueint();
                if (bus_) bus_->set_interrupts_dirty();
            }
            return data;
        }
        return 0;
    }

    return 0;
}

void Usb::write(u16 address, u8 value) noexcept {
    if (bus_) {
        std::fprintf(stderr, "[USB-WRITE] cyc=%lu addr=0x%02x val=0x%02x\n", (unsigned long)bus_->cpu_cycles(), address, value);
    }
    if (address == desc_.uhwcon_address) {
        uhwcon_ = value;
        if (bus_) bus_->set_interrupts_dirty();
    }
    else if (address == desc_.usbcon_address) {
        usbcon_ = value;
        // ATmega32U4 hardware: DETACH bit (0x10) is auto-cleared when VBUS is present
        if (usbsta_ & 0x01U)
            usbcon_ &= ~0x10U;
        if (bus_) bus_->set_interrupts_dirty();
    }
    else if (address == desc_.usbsta_address) {
        usbsta_ = value;
        if (bus_) bus_->set_interrupts_dirty();
    }
    else if (address == desc_.usbint_address) {
        // USBINT: bit 0 (VBUSTI) is W1C, bit 1 (VBUSTE) is normal R/W
        usbint_ = (usbint_ & ~0x02U) | (value & 0x02U);
        if (value & 0x01U) usbint_ &= ~0x01U;
        if (bus_) bus_->set_interrupts_dirty();
    }
    else if (address == desc_.udcon_address) {
        udcon_ = value;
        if (bus_) bus_->set_interrupts_dirty();
    }
    else if (address == desc_.udint_address) {
        bool had_sofi = (udint_ & desc_.udint_sofi_mask) != 0;
        udint_ = 0; // ATmega32U4 (and others): any write clears all flags
        // Reset SOF counter so CPU gets the full interval before next SOF
        if (had_sofi && pll_cycles_per_sof_ > 0) cycle_accumulator_ = 0;
    }
    else if (address == desc_.udien_address) {
        udien_ = value;
        if (bus_) bus_->set_interrupts_dirty();
    }
    else if (address == desc_.udaddr_address) udaddr_ = value;
    else if (address == desc_.uenum_address) uenum_ = value;
    else if (address == desc_.uerst_address) {
        uerst_ = value; // Set requested reset bits
        for (u8 i = 0; i < endpoints_.size(); ++i) {
            if (value & (1U << i)) {
                for (auto& bank : endpoints_[i].banks) {
                    bank.read_idx = 0;
                    bank.write_idx = 0;
                    bank.byte_count = 0;
                    bank.busy = false;
                }
                endpoints_[i].cpu_bank = 0;
                endpoints_[i].sie_bank = 0;
                uerst_ &= ~(1U << i); // Auto-clear after reset
            }
        }
    }
    
    u8 ep_idx = uenum_ & 0x07U;
    if (ep_idx >= endpoints_.size()) return;
    auto& ep = endpoints_[ep_idx];

    if (address == desc_.ueconx_address) {
        ep.control = value;
        if (value & 0x01U) { // EPEN
            ep.interrupt_flags |= desc_.ueintx_txini_mask; // TXINI - Ready to be loaded
            update_ueint();
            if (bus_) bus_->set_interrupts_dirty();
        }
    }
    else if (address == desc_.uecfg0x_address) {
        ep.config0 = value;
    }
    else if (address == desc_.uecfg1x_address) {
        ep.config1 = value;
        // EPSIZE bits [6:4]
        u8 size_idx = (value >> 4U) & 0x07U;
        u16 size = 8U << size_idx;
        if (size > 512) size = 512;
        
        // EPBK bits [3:2]
        u8 banks_idx = (value >> 2U) & 0x03U;
        ep.bank_count = (banks_idx == 0x01U) ? 2 : 1;

        for (auto& bank : ep.banks) {
            bank.fifo.resize(size);
            bank.read_idx = 0;
            bank.write_idx = 0;
            bank.byte_count = 0;
            bank.busy = false;
        }
        ep.cpu_bank = 0;
        ep.sie_bank = 0;
    }
    else if (address == desc_.ueintx_address) {
        u8 old_flags = ep.interrupt_flags;
        if (ep_idx == 0 && bus_) {
            std::fprintf(stderr, "[USB-WR-UEINTX0] cyc=%lu val=0x%02x old=0x%02x\n", (unsigned long)bus_->cpu_cycles(), value, old_flags);
        }
        // ATmega32U4 UEINTX is W0C: writing 0 clears a bit, writing 1 leaves it unchanged.
        // RWAL (bit 5) is read-only. FIFOCON (bit 7) has special bank-commit semantics.
        // Apply W0C to bits 0-4, 6 (TXINI, STALLEDI, RXOUTI, RXSTPI, NAKOUTI, NAKINI).
        constexpr u8 kW0cBits = 0x5FU; // bits 0,1,2,3,4,6 — all clearable except RWAL(5)
        ep.interrupt_flags &= (value | ~kW0cBits); // keep bit if value=1 or bit is non-W0C
        // Which flags were actually cleared (were set, now written as 0)?
        u8 cleared_bits = old_flags & ~value;

        // Handle FIFOCON (Bit 7) — W0C: writing 0 may trigger bank commit/release.
        // When RXSTPI is set (unprocessed SETUP packet), FIFOCON is read-only (ignored).
        bool fifocon_cleared = !(value & 0x80U) && !(ep.interrupt_flags & desc_.ueintx_rxstpi_mask);
        // On real ATmega32U4, clearing TXINI (bit 0) alone sends the IN packet for control
        // endpoints — FIFOCON is optional. Trigger bank commit on TXINI-clear too.
        bool txini_cleared = (cleared_bits & desc_.ueintx_txini_mask) != 0;
        bool in_commit = (fifocon_cleared || txini_cleared) &&
                         !(ep.interrupt_flags & desc_.ueintx_rxstpi_mask);
        if (in_commit) {
            auto& current_bank = ep.banks[ep.cpu_bank];
            // Direction: OUT if no data and RXOUTI was pending, else IN commit.
            bool out_phase = fifocon_cleared && (current_bank.byte_count == 0 &&
                             (old_flags & desc_.ueintx_rxouti_mask));
            if (!out_phase) {
                // IN direction: firmware committed data (including ZLP) for host to read
                current_bank.busy = true;
                if (ep_idx == 0 && bus_) {
                    std::fprintf(stderr, "[USB-IN-COMMIT] cyc=%lu busy=%d cpu_bank=%d\n",
                        (unsigned long)bus_->cpu_cycles(), current_bank.busy, ep.cpu_bank);
                }
                // Switch CPU to next bank if available
                if (ep.bank_count > 1) {
                    u8 next_bank = (ep.cpu_bank + 1) % ep.bank_count;
                    if (!ep.banks[next_bank].busy) {
                        ep.cpu_bank = next_bank;
                        ep.interrupt_flags |= desc_.ueintx_txini_mask;
                    }
                }
            } else {
                // OUT direction: firmware released the bank after reading data
                current_bank.busy = false;
                current_bank.byte_count = 0;
                current_bank.read_idx = 0;
                current_bank.write_idx = 0;
                ep.interrupt_flags &= ~desc_.ueintx_rxouti_mask;
                if (ep.bank_count > 1) {
                    u8 next_bank = (ep.cpu_bank + 1) % ep.bank_count;
                    if (ep.banks[next_bank].busy && ep.banks[next_bank].byte_count > 0) {
                        ep.cpu_bank = next_bank;
                        ep.interrupt_flags |= desc_.ueintx_rxouti_mask;
                    }
                }
            }
        }

        if (cleared_bits & desc_.ueintx_rxstpi_mask) {
            // Firmware explicitly cleared RXSTPI: release SETUP bank, set TXINI for response.
            // Only release/set if we didn't also commit (clear TXINI) in the same write.
            if (!txini_cleared) {
                ep.banks[ep.cpu_bank].busy = false;
                if (is_in_endpoint(ep) && ep.banks[ep.cpu_bank].byte_count == 0)
                    ep.interrupt_flags |= desc_.ueintx_txini_mask;
            }
        }
        if (cleared_bits & desc_.ueintx_rxouti_mask) {
            ep.banks[ep.cpu_bank].busy = false;
            if (ep_idx == 0 || ((ep.config0 >> 6) & 0x03U) == 0) {
                ep.interrupt_flags |= desc_.ueintx_txini_mask;
            }
        }

        update_ueint();
        if (ep_idx == 0 && bus_) {
            std::fprintf(stderr, "[USB-WR-UEINTX0-AFTER] cyc=%lu flags=0x%02x\n", (unsigned long)bus_->cpu_cycles(), ep.interrupt_flags);
        }
        if (bus_) bus_->set_interrupts_dirty();
    }
    else if (address == desc_.ueienx_address) {
        ep.interrupt_enable = value;
        update_ueint();
        if (bus_) bus_->set_interrupts_dirty();
    }
    else if (address == desc_.uesta0x_address) {
        // UESTA0X: write-0-to-clear STALLRQ (bit 5), write 1 sets CTRLDIR (bit 2)
        for (u8 b = 0; b < 8; ++b) {
            u8 bit = (1U << b);
            if (value & bit) {
                if (b == 2) ep.status0 |= bit;       // CTRLDIR set by writing 1
                else if (b == 5) {}                   // STALLRQ set by HW, writing 1 has no effect
            } else {
                if (b == 5) ep.status0 &= ~bit;      // STALLRQ cleared by writing 0
            }
        }
    }
    else if (address == desc_.uesta1x_address) {
        // UESTA1X: mostly read-only. Bits 0 (CFGOK) cleared by writing 1.
        ep.status1 &= ~value;
    }
    else if (address == desc_.uebclx_address) {
        // UEBCLX: lower byte of byte count — writable for test/debug
        u16 count = ep.banks[ep.cpu_bank].byte_count;
        ep.banks[ep.cpu_bank].byte_count = static_cast<u16>((count & 0xFF00U) | value);
    }
    else if (address == desc_.uebchx_address) {
        // UEBCHX: upper byte of byte count — writable for test/debug
        u16 count = ep.banks[ep.cpu_bank].byte_count;
        ep.banks[ep.cpu_bank].byte_count = static_cast<u16>((count & 0x00FFU) | (static_cast<u16>(value) << 8U));
    }
    else if (address == desc_.uedatx_address) {
        // Only allow writes for IN endpoints with TXINI set
        if (is_in_endpoint(ep) && (ep.interrupt_flags & desc_.ueintx_txini_mask)) {
            auto& bank = ep.banks[ep.cpu_bank];
            if (bank.byte_count < bank.fifo.size()) {
                bank.fifo[bank.write_idx] = value;
                bank.write_idx = (bank.write_idx + 1) % bank.fifo.size();
                bank.byte_count++;
            }
        }
    }
}

void Usb::update_ueint() noexcept {
    ueint_ = 0;
    for (size_t i = 0; i < endpoints_.size(); ++i) {
        if (endpoints_[i].interrupt_flags & endpoints_[i].interrupt_enable) {
            ueint_ |= (1U << i);
        }
    }
}

bool Usb::is_in_endpoint(const Endpoint& ep) const noexcept {
    // ATmega32U4 UECFG0X bit layout: EPTYPE=bits[7:6], EPDIR=bit[0]
    u8 eptype = (ep.config0 >> 6) & 0x03U;
    if (eptype == 0x01U) return true; // Control: bidirectional
    if (eptype == 0x00U) {
        // No type configured: EP0 is always control, others are disabled
        ptrdiff_t idx = &ep - endpoints_.data();
        if (idx >= 0 && static_cast<size_t>(idx) < endpoints_.size() && static_cast<size_t>(idx) == 0)
            return true;
        return false;
    }
    return (ep.config0 & 0x01U) != 0; // EPDIR = bit 0
}

bool Usb::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (!(usbcon_ & desc_.usbcon_usbe_mask))
        return false;
    if (usbcon_ & desc_.usbcon_frzclk_mask)
        return false;

    // General Interrupt (USB_GEN): UDINT sources + VBUSTI (USBINT)
    // UDIEN bits may not map 1:1 to UDINT bits (e.g. ATmega32U4 EORSTE=bit2, EORSTI=bit4)
    u8 gen_check = 0;
    if (udint_ & desc_.udint_sofi_mask && udien_ & desc_.udien_sofe_mask)
        gen_check |= desc_.udint_sofi_mask;
    if (udint_ & desc_.udint_eorsti_mask && udien_ & desc_.udien_eorste_mask)
        gen_check |= desc_.udint_eorsti_mask;
    u8 vbusti_check = usbint_ & 0x03U;
    if (gen_check || vbusti_check == 0x03U) {
        request.vector_index = desc_.gen_vector_index;
        return true;
    }

    // Communication Interrupt (USB_COM)
    if (ueint_) {
        request.vector_index = desc_.com_vector_index;
        return true;
    }

    return false;
}

bool Usb::consume_interrupt_request(InterruptRequest& request) noexcept {
    return pending_interrupt_request(request);
}

void Usb::simulate_usb_reset() noexcept {
    udaddr_ = 0;
    uenum_ = 0;
    uerst_ = 0;
    udcon_ = 0;
    udint_ |= desc_.udint_eorsti_mask; // EORSTI
    if (bus_) bus_->set_interrupts_dirty();

    for (auto& ep : endpoints_) {
        ep.control = 0;
        ep.config0 = 0;
        ep.config1 = 0;
        ep.status0 = 0;
        ep.status1 = 0;
        ep.interrupt_flags = 0;
        ep.interrupt_enable = 0;
        ep.data_toggle = false;
        ep.cpu_bank = 0;
        ep.sie_bank = 0;
        ep.bank_count = 1;
        ep.data_toggle = false;
        for (auto& bank : ep.banks) {
            bank.read_idx = 0;
            bank.write_idx = 0;
            bank.byte_count = 0;
            bank.busy = false;
        }
    }
    update_ueint();
}

void Usb::simulate_vbus_event(bool high) noexcept {
    if (high) {
        usbsta_ |= 0x01U; // VBUS bit
        // Real ATmega32U4 hardware: VBUS detection auto-clears DETACH (USBCON bit 4)
        usbcon_ &= ~0x10U; // Clear DETACH
    } else {
        usbsta_ &= ~0x01U;
    }
    usbint_ |= 0x01U; // VBUSTI
    if (bus_) bus_->set_interrupts_dirty();
}

void Usb::simulate_setup_packet(const SetupPacket& setup) noexcept {
    auto& ep = endpoints_[0];
    auto& bank = ep.banks[0]; // Setup always bank 0
    bank.fifo.resize(64);
    bank.fifo[0] = setup.bmRequestType;
    bank.fifo[1] = setup.bRequest;
    bank.fifo[2] = static_cast<u8>(setup.wValue & 0xFFU);
    bank.fifo[3] = static_cast<u8>((setup.wValue >> 8U) & 0xFFU);
    bank.fifo[4] = static_cast<u8>(setup.wIndex & 0xFFU);
    bank.fifo[5] = static_cast<u8>((setup.wIndex >> 8U) & 0xFFU);
    bank.fifo[6] = static_cast<u8>(setup.wLength & 0xFFU);
    bank.fifo[7] = static_cast<u8>((setup.wLength >> 8U) & 0xFFU);
    bank.read_idx = 0;
    bank.write_idx = 8;
    bank.byte_count = 8;
    bank.busy = false; // Don't set busy here — SIE→CPU direction. busy=true means CPU has data for SIE.
    ep.interrupt_flags |= desc_.ueintx_rxstpi_mask; // RXSTPI
    ep.interrupt_flags &= ~desc_.ueintx_txini_mask;  // cleared by hardware on SETUP
    ep.interrupt_flags &= ~desc_.ueintx_rxouti_mask; // cleared by hardware on SETUP
    ep.data_toggle = false;
    update_ueint();
    if (bus_) bus_->set_interrupts_dirty();
}

void Usb::simulate_out_packet(u8 ep_idx, bool data1_pid, std::span<const u8> data) noexcept {
    if (ep_idx >= endpoints_.size()) return;
    auto& ep = endpoints_[ep_idx];
    
    if (ep.control & desc_.ueconx_stallrq_mask) {
        ep.interrupt_flags |= desc_.ueintx_stalledi_mask;
        update_ueint();
        return;
    }

    // Find next free bank for SIE
    u8 target_bank = ep.sie_bank;
    if (ep.banks[target_bank].busy && ep_idx != 0) {
        // Current bank busy, check next if double banked
        if (ep.bank_count > 1) {
            u8 next = (target_bank + 1) % ep.bank_count;
            if (!ep.banks[next].busy) target_bank = next;
            else return; // Both busy
        } else return; // Single bank busy
    }

    auto& bank = ep.banks[target_bank];
    size_t to_copy = std::min(data.size(), bank.fifo.size());
    for (size_t i = 0; i < to_copy; ++i) {
        bank.fifo[i] = data[i];
    }
    bank.write_idx = to_copy;
    bank.read_idx = 0;
    bank.byte_count = static_cast<u16>(to_copy);
    bank.busy = true;
    
    ep.sie_bank = (target_bank + 1) % ep.bank_count;
    ep.interrupt_flags |= desc_.ueintx_rxouti_mask; // RXOUTI
    if (data1_pid == ep.data_toggle) {
        ep.data_toggle = !ep.data_toggle;
    }
    update_ueint();
    if (bus_) bus_->set_interrupts_dirty();
}

void Usb::simulate_in_token(u8 ep_idx) noexcept {
    if (ep_idx >= endpoints_.size()) return;
    auto& ep = endpoints_[ep_idx];
    
    if (ep.control & desc_.ueconx_stallrq_mask) {
        ep.interrupt_flags |= desc_.ueintx_stalledi_mask;
        update_ueint();
        return;
    }

    auto& bank = ep.banks[ep.sie_bank];
    if (bank.byte_count > 0 || bank.busy) {
        // Data or explicit commit: save data, clear bank, advance, set TXINI
        last_in_data_.resize(bank.byte_count);
        for (u16 i = 0; i < bank.byte_count; ++i) {
            last_in_data_[i] = bank.fifo[(bank.read_idx + i) % bank.fifo.size()];
        }
        
        bank.read_idx = 0;
        bank.write_idx = 0;
        bank.byte_count = 0;
        bank.busy = false;
        
        ep.sie_bank = (ep.sie_bank + 1) % ep.bank_count;
        ep.data_toggle = !ep.data_toggle;
        
        ep.interrupt_flags |= desc_.ueintx_txini_mask; // TXINI (Handshake ACK)
        update_ueint();
        if (bus_) bus_->set_interrupts_dirty();
    } else if (ep.interrupt_flags & desc_.ueintx_txini_mask) {
        // ZLP case: TXINI=1 but no data committed. Host sends IN, SIE delivers ZLP.
        // Clear TXINI to signal the ZLP was consumed, then immediately re-set it
        // (SIE re-asserts TXINI after ZLP ACK, just like real hardware).
        // This prevents stale TXINI=1 from falsely triggering is_ep_response_ready
        // for subsequent control transfers.
        last_in_data_.clear(); // ZLP — no data
        ep.interrupt_flags &= ~desc_.ueintx_txini_mask; // consumed
        ep.interrupt_flags |= desc_.ueintx_txini_mask;  // SIE re-asserts (instant)
        // Note: the re-assertion is intentional. The firmware needs TXINI=1 to load
        // the next response. But the host state machine transitions away after this call,
        // so the "stale TXINI" problem is solved by the state machine not re-checking.
        update_ueint();
        if (bus_) bus_->set_interrupts_dirty();
    }
}

std::vector<u8> Usb::get_endpoint_data(u8 ep_idx) const noexcept {
    if (ep_idx >= endpoints_.size()) return {};
    if (ep_idx == 0) return last_in_data_; // Return data captured during IN token
    
    const auto& ep = endpoints_[ep_idx];
    const auto& bank = ep.banks[ep.sie_bank];
    if (bank.byte_count == 0) return {};
    std::vector<u8> data;
    data.reserve(bank.byte_count);
    size_t idx = bank.read_idx;
    for (u16 i = 0; i < bank.byte_count; ++i) {
        data.push_back(bank.fifo[idx]);
        idx = (idx + 1) % bank.fifo.size();
    }
    return data;
}

bool Usb::is_endpoint_busy(u8 ep_idx) const noexcept {
    if (ep_idx >= endpoints_.size()) return false;
    auto& ep = endpoints_[ep_idx];
    auto& bank = ep.banks[ep.sie_bank];
    // Only report busy when the bank has been explicitly committed via FIFOCON (busy=true).
    // Do NOT use byte_count > 0 — that fires while firmware is still loading UEDATX bytes,
    // before the Endpoint_ClearIN() FIFOCON commit that signals the bank is ready for the host.
    return bank.busy;
}

void Usb::consume_out_status_zlp(u8 ep_idx) noexcept {
    // Simulate the SIE autonomously ACKing the status-stage OUT ZLP for IN control transfers.
    // For GET_DESCRIPTOR (and similar), LUFA/Caterina firmware does NOT explicitly handle
    // the status OUT ZLP — the USB SIE handles it transparently. This function clears
    // the simulated RXOUTI bank state so the next SETUP packet can be injected cleanly.
    if (ep_idx >= endpoints_.size()) return;
    auto& ep = endpoints_[ep_idx];
    // simulate_out_packet puts data in banks[target_bank] where target_bank=sie_bank before advance.
    // For single-bank EP0: simulate_out_packet uses bank 0 (sie_bank before advance was 0).
    // Clear all banks to be safe.
    for (auto& bank : ep.banks) {
        bank.busy = false;
        bank.byte_count = 0;
        bank.read_idx = 0;
        bank.write_idx = 0;
    }
    // Clear RXOUTI flag — SIE consumed the ZLP without firmware involvement
    ep.interrupt_flags &= ~desc_.ueintx_rxouti_mask;
    update_ueint();
    if (bus_) bus_->set_interrupts_dirty();
}

} // namespace vioavr::core
