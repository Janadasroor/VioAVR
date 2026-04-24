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
    
    // Initialize FIFOs for endpoints
    for (auto& ep : endpoints_) {
        for (auto& bank : ep.banks) {
            bank.fifo.resize(64); // Small default size
        }
    }
}

std::span<const AddressRange> Usb::mapped_ranges() const noexcept {
    return {ranges_.data(), ranges_.size()};
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
    frame_cycles_ = 16000; // Default to 16MHz -> 1ms = 16k cycles
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

void Usb::tick(u64 elapsed_cycles) noexcept {
    if (!(usbcon_ & desc_.usbcon_usbe_mask)) return; // USBE must be 1
    if (usbcon_ & desc_.usbcon_frzclk_mask) return; // FRZCLK must be 0
    
    // On many AVRs, USB clock requires PLL to be locked
    if (bus_ && desc_.pllcsr_address) {
        if (!(bus_->read_data(desc_.pllcsr_address) & 0x01U)) return; // PLOCK bit
    }

    cycle_accumulator_ += elapsed_cycles;
    while (cycle_accumulator_ >= frame_cycles_) {
        cycle_accumulator_ -= frame_cycles_;
        
        frame_number_ = (frame_number_ + 1) & 0x7FFU;
        udint_ |= desc_.udint_sofi_mask; // SOFI
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
        bool is_in = (ep.config0 & 0x80U) != 0;
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
            return data;
        }
        return 0;
    }

    return 0;
}

void Usb::write(u16 address, u8 value) noexcept {
    if (address == desc_.uhwcon_address) uhwcon_ = value;
    else if (address == desc_.usbcon_address) usbcon_ = value;
    else if (address == desc_.usbsta_address) usbsta_ = value;
    else if (address == desc_.usbint_address) usbint_ = value;
    else if (address == desc_.udcon_address) udcon_ = value;
    else if (address == desc_.udint_address) udint_ &= value; // Clear on write 0
    else if (address == desc_.udien_address) udien_ = value;
    else if (address == desc_.udaddr_address) udaddr_ = value;
    else if (address == desc_.uenum_address) uenum_ = value;
    else if (address == desc_.uerst_address) {
        uerst_ = value;
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
                // Note: bit is not automatically cleared by hardware
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
        }
    }
    else if (address == desc_.uecfg0x_address) ep.config0 = value;
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
        ep.interrupt_flags &= (value | 0xA0U); // Only allow clearing bits 0-4, 6. Bit 5 (RWAL) and 7 (FIFOCON) handled separately.
        u8 cleared_bits = old_flags & (~value);
        
        // Handle FIFOCON (Bit 7) clearing
        if (!(value & 0x80U)) {
            bool is_in = (ep.config0 & 0x80U) != 0;
            auto& current_bank = ep.banks[ep.cpu_bank];
            if (is_in) {
                // IN: FIFOCON cleared -> Current bank is full, ready to send
                current_bank.busy = true;
                ep.interrupt_flags &= ~desc_.ueintx_txini_mask; // Clear TXINI for current bank
                
                // Switch CPU to next bank if available and not busy
                if (ep.bank_count > 1) {
                    u8 next_bank = (ep.cpu_bank + 1) % ep.bank_count;
                    if (!ep.banks[next_bank].busy) {
                        ep.cpu_bank = next_bank;
                        ep.interrupt_flags |= desc_.ueintx_txini_mask; // Next bank is ready to be loaded
                    }
                }
            } else {
                // OUT: FIFOCON cleared -> Current bank is empty, ready to receive next
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
        }
        if (cleared_bits & desc_.ueintx_rxstpi_mask) {
            ep.banks[ep.cpu_bank].busy = false; 
        }
        if (cleared_bits & desc_.ueintx_rxouti_mask) {
            ep.banks[ep.cpu_bank].busy = false;
        }
        
        update_ueint();
    }
    else if (address == desc_.ueienx_address) {
        ep.interrupt_enable = value;
        update_ueint();
    }
    else if (address == desc_.uedatx_address) {
        auto& bank = ep.banks[ep.cpu_bank];
        if (bank.byte_count < bank.fifo.size()) {
            bank.fifo[bank.write_idx] = value;
            bank.write_idx = (bank.write_idx + 1) % bank.fifo.size();
            bank.byte_count++;
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

bool Usb::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (!(usbcon_ & desc_.usbcon_usbe_mask)) return false; 
    if (usbcon_ & desc_.usbcon_frzclk_mask) return false;   

    // General Interrupt (USB_GEN)
    if (udint_ & udien_) {
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
    udint_ |= 0x08U; // EORSTI
}

void Usb::simulate_vbus_event(bool high) noexcept {
    if (high) {
        usbsta_ |= 0x01U; // VBUS bit
    } else {
        usbsta_ &= ~0x01U;
    }
    usbint_ |= 0x01U; // VBUSTI
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
    bank.busy = true; 
    ep.interrupt_flags |= desc_.ueintx_rxstpi_mask; // RXSTPI
    ep.data_toggle = false; 
    update_ueint();
}

void Usb::simulate_out_packet(u8 ep_idx, std::span<const u8> data) noexcept {
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
    ep.data_toggle = !ep.data_toggle;
    update_ueint();
}

void Usb::simulate_in_token(u8 ep_idx) noexcept {
    if (ep_idx >= endpoints_.size()) return;
    auto& ep = endpoints_[ep_idx];
    
    if (ep.control & desc_.ueconx_stallrq_mask) {
        ep.interrupt_flags |= desc_.ueintx_stalledi_mask;
        update_ueint();
        return;
    }

    // Check if SIE's current bank is busy (loaded by CPU and ready to send)
    auto& bank = ep.banks[ep.sie_bank];
    if (bank.busy) {
        // Data sent! Clear busy and trigger TXINI
        bank.read_idx = 0;
        bank.write_idx = 0;
        bank.byte_count = 0;
        bank.busy = false;
        
        ep.sie_bank = (ep.sie_bank + 1) % ep.bank_count;
        ep.data_toggle = !ep.data_toggle;
        
        // If CPU was waiting for a free bank (TXINI was 0), set it now
        bool is_in = (ep.config0 & 0x80U) != 0;
        if (is_in && !(ep.interrupt_flags & desc_.ueintx_txini_mask)) {
            // Check if the bank we just freed is the one the CPU should use next
            // In double banking, if TXINI is 0, it means all banks are busy.
            // Freed bank becomes the next available.
            ep.interrupt_flags |= desc_.ueintx_txini_mask;
        }

        ep.interrupt_flags |= desc_.ueintx_txini_mask; // TXINI (Handshake ACK)
        update_ueint();
    }
}

std::vector<u8> Usb::get_endpoint_data(u8 ep_idx) const noexcept {
    if (ep_idx >= endpoints_.size()) return {};
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

} // namespace vioavr::core
