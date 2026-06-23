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
            static int dbg_cnt = 0;
            if (dbg_cnt++ < 30)
                std::fprintf(stderr, "[UEDATX-RD] bc_after=%d data=%02x flags=%02x\n",
                    (int)bank.byte_count, data, ep.interrupt_flags);
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
    if (address == desc_.uhwcon_address) {
        uhwcon_ = value;
        if (bus_) bus_->set_interrupts_dirty();
    }
    else if (address == desc_.usbcon_address) {
        static int dbg_cnt = 0;
        if (dbg_cnt++ < 20)
            std::fprintf(stderr, "[USBCON-WR] old=%02X new=%02X\n", usbcon_, value);
        usbcon_ = value;
        // ATmega32U4 hardware: DETACH bit (0x10) is auto-cleared when VBUS is present
        if (usbsta_ & 0x01U) {
            u8 old = usbcon_;
            usbcon_ &= ~0x10U;
            if (old != usbcon_) {
                std::fprintf(stderr, "[USBCON-AUTOCLR] DETACH cleared by VBUS\n");
            }
        }
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
        udint_ = 0; // ATmega32U4 (and others): any write clears all flags
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
        std::fprintf(stderr, "[UECONX-WR] ep=%d val=%02x\n", (int)uenum_, value);
        ep.control = value;
        if (value & 0x01U) { // EPEN
            ep.interrupt_flags |= desc_.ueintx_txini_mask; // TXINI - Ready to be loaded
            update_ueint();
            if (bus_) bus_->set_interrupts_dirty();
        }
    }
    else if (address == desc_.uecfg0x_address) {
        std::fprintf(stderr, "[CFG0-WR] ep=%d val=%02x (eptype=%d dir=%d)\n",
            (int)uenum_, value, (value >> 6) & 0x03, value & 0x01);
        ep.config0 = value;
    }
    else if (address == desc_.uecfg1x_address) {
        std::fprintf(stderr, "[CFG1-WR] ep=%d val=%02x epsize_idx=%d banks=%d\n",
            (int)uenum_, value, (value >> 4U) & 0x07U, ((value >> 2U) & 0x03U));
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
        static int dbg_ueintx = 0;
        if (dbg_ueintx++ < 10)
            std::fprintf(stderr, "[UEINTX-WR] ep=%d val=%02x old_fl=%02x bc=%d busy=%d\n",
                (int)uenum_, value, old_flags,
                (int)ep.banks[ep.cpu_bank].byte_count, (int)ep.banks[ep.cpu_bank].busy);
        // Write-1-to-clear for bits 0-4,6. Bits 5 (RWAL) and 7 (FIFOCON) handled separately.
        ep.interrupt_flags &= ~(value & ~0xA0U);
        u8 cleared_bits = old_flags & value;
        
        // Handle FIFOCON (Bit 7) clearing — write-1-to-clear
        // When RXSTPI is set (unprocessed SETUP packet), FIFOCON is read-only-0 (datasheet 21.6.6)
        if ((value & 0x80U) && !(ep.interrupt_flags & desc_.ueintx_rxstpi_mask)) {
            auto& current_bank = ep.banks[ep.cpu_bank];
            // Determine direction from state, not from write value:
            // - byte_count > 0: CPU wrote data → IN commit
            // - byte_count == 0 && RXOUTI pending: CPU finished reading → OUT release
            bool out_phase = (current_bank.byte_count == 0 && 
                             (old_flags & desc_.ueintx_rxouti_mask));
            if (!out_phase) {
                // IN direction: firmware committed data for host to read
                current_bank.busy = true;
                // Only clear TXINI if data was actually written (firmware finalizing IN data)
                if (current_bank.byte_count > 0) {
                    ep.interrupt_flags &= ~desc_.ueintx_txini_mask;
                }
                
                // Switch CPU to next bank if available and not busy
                if (ep.bank_count > 1) {
                    u8 next_bank = (ep.cpu_bank + 1) % ep.bank_count;
                    if (!ep.banks[next_bank].busy) {
                        ep.cpu_bank = next_bank;
                        ep.interrupt_flags |= desc_.ueintx_txini_mask; // Next bank is ready to be loaded
                    }
                }
            } else {
                // OUT direction or EP is OUT: firmware released the bank after reading data
                current_bank.busy = false;
                current_bank.byte_count = 0;
                current_bank.read_idx = 0;
                current_bank.write_idx = 0;
                ep.interrupt_flags &= ~desc_.ueintx_rxouti_mask; // Clear RXOUTI for current bank
                
                // Switch CPU to next bank if it has data
                if (ep.bank_count > 1) {
                    u8 next_bank = (ep.cpu_bank + 1) % ep.bank_count;
                    if (ep.banks[next_bank].busy && ep.banks[next_bank].byte_count > 0) {
                        ep.cpu_bank = next_bank;
                        ep.interrupt_flags |= desc_.ueintx_rxouti_mask; // Next bank has data
                    }
                }
            }
        }

        if (cleared_bits & desc_.ueintx_txini_mask) {
            ep.banks[ep.cpu_bank].busy = true;
            static int dbg_cnt = 0;
            if (dbg_cnt++ < 5 && ep_idx == 0)
                std::fprintf(stderr, "[USB-BUSY] ep0 TXINI cleared: busy=true\n");
        }
        if (cleared_bits & desc_.ueintx_rxstpi_mask) {
            ep.banks[ep.cpu_bank].busy = false;
            // After clearing RXSTPI, the bank is released. For IN endpoints with empty
            // bank, auto-assert TXINI so the CPU can load data for the data phase.
            if (is_in_endpoint(ep) && ep.banks[ep.cpu_bank].byte_count == 0) {
                ep.interrupt_flags |= desc_.ueintx_txini_mask;
            }
        }
        if (cleared_bits & desc_.ueintx_rxouti_mask) {
            ep.banks[ep.cpu_bank].busy = false;
        }
        
        update_ueint();
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
        static int dbg_cnt_w = 0;
        if (is_in_endpoint(ep) && (ep.interrupt_flags & desc_.ueintx_txini_mask)) {
            auto& bank = ep.banks[ep.cpu_bank];
            if (bank.byte_count < bank.fifo.size()) {
                bank.fifo[bank.write_idx] = value;
                bank.write_idx = (bank.write_idx + 1) % bank.fifo.size();
                bank.byte_count++;
                if (dbg_cnt_w++ < 30)
                    std::fprintf(stderr, "[UEDATX-WR] bc=%d data=%02x\n", (int)bank.byte_count, value);
            } else if (dbg_cnt_w++ < 5)
                std::fprintf(stderr, "[UEDATX-FULL] bc=%d\n", (int)bank.byte_count);
        } else if (dbg_cnt_w++ < 10) {
            std::fprintf(stderr, "[UEDATX-REJ] in=%d c0=%02x c1=%02x txini=%d busy=%d bc=%d flags=%02x\n",
                is_in_endpoint(ep), ep.config0, ep.config1,
                ep.interrupt_flags & desc_.ueintx_txini_mask,
                ep.banks[ep.cpu_bank].busy, ep.banks[ep.cpu_bank].byte_count, ep.interrupt_flags);
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
    if (!(usbcon_ & desc_.usbcon_usbe_mask)) {
        static int dbg_cnt = 0;
        if (dbg_cnt++ < 5)
            std::fprintf(stderr, "[USB-NOUSBE] usbcon=%02x udint=%02x udien=%02x usbint=%02x\n",
                usbcon_, udint_, udien_, usbint_);
        return false;
    }
    if (usbcon_ & desc_.usbcon_frzclk_mask) {
        static int dbg_cnt = 0;
        if (dbg_cnt++ < 5)
            std::fprintf(stderr, "[USB-FRZCLK] usbcon=%02x\n", usbcon_);
        return false;
    }

    // General Interrupt (USB_GEN): UDINT sources + VBUSTI (USBINT)
    // UDIEN bits may not map 1:1 to UDINT bits (e.g. ATmega32U4 EORSTE=bit2, EORSTI=bit4)
    u8 gen_check = 0;
    if (udint_ & desc_.udint_sofi_mask && udien_ & desc_.udien_sofe_mask)
        gen_check |= desc_.udint_sofi_mask;
    if (udint_ & desc_.udint_eorsti_mask && udien_ & desc_.udien_eorste_mask)
        gen_check |= desc_.udint_eorsti_mask;
    u8 vbusti_check = usbint_ & 0x03U;
    if (gen_check || vbusti_check == 0x03U) {
        static int dbg_cnt = 0;
        if (dbg_cnt++ < 20)
            std::fprintf(stderr, "[USB-PEND] udint=%02x udien=%02x gen=%02x usbint=%02x vector=%d\n",
                udint_, udien_, gen_check, usbint_, desc_.gen_vector_index);
        request.vector_index = desc_.gen_vector_index;
        return true;
    }

    // Communication Interrupt (USB_COM)
    if (ueint_) {
        static int dbg_cnt = 0;
        if (dbg_cnt++ < 20) {
            u8 ec0 = endpoints_[0].control;
            u8 fl0 = endpoints_[0].interrupt_flags;
            u8 en0 = endpoints_[0].interrupt_enable;
            u8 bc0 = endpoints_[0].banks[0].byte_count;
            bool bsy0 = endpoints_[0].banks[0].busy;
            std::fprintf(stderr, "[USB-COM] ueint=%02x ec0=%02x fl0=%02x en0=%02x bc0=%d bsy0=%d\n",
                ueint_, ec0, fl0, en0, bc0, bsy0);
        }
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
    ep.data_toggle = false; 
    update_ueint();
    u8 uei = endpoints_[0].interrupt_enable;
    std::fprintf(stderr, "[USB-SETUP] ep=0 sie_bank=%d bc=%d uei=%02x fifo=%02x%02x%02x%02x...\n",
        ep.sie_bank, ep.banks[ep.sie_bank].byte_count, uei,
        bank.fifo[0], bank.fifo[1], bank.fifo[2], bank.fifo[3]);
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
    if (ep.banks[target_bank].busy) {
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
        // Save transmitted data before clearing
        last_in_data_.resize(bank.byte_count);
        for (u16 i = 0; i < bank.byte_count; ++i) {
            last_in_data_[i] = bank.fifo[(bank.read_idx + i) % bank.fifo.size()];
        }
        
        // Data sent! Clear busy and trigger TXINI
        bank.read_idx = 0;
        bank.write_idx = 0;
        bank.byte_count = 0;
        bank.busy = false;
        
        ep.sie_bank = (ep.sie_bank + 1) % ep.bank_count;
        ep.data_toggle = !ep.data_toggle;
        
        ep.interrupt_flags |= desc_.ueintx_txini_mask; // TXINI (Handshake ACK)
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
    static int dbg = 0;
    if (dbg++ < 5)
        std::fprintf(stderr, "[IS-BUSY] ep=%d bc=%d busy=%d result=%d\n",
            ep_idx, (int)bank.byte_count, (int)bank.busy, bank.byte_count > 0 || bank.busy);
    return bank.byte_count > 0 || bank.busy;
}

} // namespace vioavr::core
