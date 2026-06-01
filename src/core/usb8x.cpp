#include "vioavr/core/usb8x.hpp"

namespace vioavr::core {

Usb8x::Usb8x(const Usb8xDescriptor& desc) noexcept
    : desc_(desc) {
    u16 range_end = 0;
    auto update = [&](u16 addr) { if (addr > range_end) range_end = addr; };
    update(desc_.ctrla_address);
    update(desc_.ctrlb_address);
    update(desc_.busstate_address);
    update(desc_.addr_address);
    update(desc_.fifowp_address);
    update(desc_.fiforp_address);
    update(desc_.epptr_address);
    update(desc_.intctrla_address);
    update(desc_.intctrlb_address);
    update(desc_.intflagsa_address);
    update(desc_.intflagsb_address);
    update(desc_.cal0_address);
    update(desc_.cal1_address);
    update(desc_.pllcsr_address);
    if (desc_.ctrla_address != 0 && range_end > desc_.ctrla_address) {
        ranges_[0] = {desc_.ctrla_address, range_end};
    }
    if (desc_.usb_pll_frequency_hz > 0) {
        sof_frame_cycles_ = desc_.usb_pll_frequency_hz / 1000ULL;
    } else {
        sof_frame_cycles_ = 48000ULL;
    }
}

std::span<const AddressRange> Usb8x::mapped_ranges() const noexcept {
    if (desc_.ctrla_address != 0) return ranges_;
    return {};
}

void Usb8x::reset() noexcept {
    ctrla_ = 0;
    ctrlb_ = 0;
    busstate_ = 0;
    addr_ = 0;
    fifowp_ = 0;
    fiforp_ = 0;
    epptr_ = 0;
    intctrla_ = 0;
    intctrlb_ = 0;
    intflagsa_ = 0;
    intflagsb_ = 0;
    pllcsr_ = 0;
    fifo_count_ = 0;
    fifo_wp_ = 0;
    fifo_rp_ = 0;
    sof_cycle_counter_ = 0;
    for (auto& b : ep0_setup_pkt_) b = 0;
    ep0_setup_count_ = 0;
    ep0_stalled_ = false;
    ep0_data_dir_ = 0;
    ep0_wLength_ = 0;
    prev_bus_state_ = 0;
}

void Usb8x::detect_setup_packet() noexcept {
    u8 ep_type = epptr_ & 0x07U;
    u8 ep_num = (epptr_ >> 4) & 0x07U;
    if (ep_type != 4 || ep_num != 0) return;

    // Store the byte that was just written (before fifo_wp_ was incremented)
    u8 last_byte = fifo_buf_[(fifo_wp_ == 0) ? 63 : (fifo_wp_ - 1)];
    if (ep0_setup_count_ < 8) {
        ep0_setup_pkt_[ep0_setup_count_] = last_byte;
    }
    ++ep0_setup_count_;

    if (ep0_setup_count_ >= 8 && fifo_count_ >= 8) {
        // Full setup packet received (ep0_setup_pkt_ already contains all 8 bytes)
        ep0_data_dir_ = (ep0_setup_pkt_[0] & 0x80) ? 1 : 0;
        ep0_wLength_ = static_cast<u16>(ep0_setup_pkt_[6]) |
                       (static_cast<u16>(ep0_setup_pkt_[7]) << 8);
        ep0_stalled_ = false;
        intflagsb_ |= 0x04;
    }
}

void Usb8x::handle_bus_state_change(u8 old_state, u8 new_state) noexcept {
    // BUSSTATE bits 1:0: 0=DISABLE, 1=DISCON, 2=CONN, 3=SUSPEND
    if (old_state == new_state) return;

    if (new_state == 3) {
        intflagsa_ |= 0x01; // SUSPEND
    } else if (new_state == 2 && old_state == 1) {
        intflagsa_ |= 0x04; // EORST (connect after disconnect)
    } else if (new_state == 1 && old_state == 3) {
        intflagsa_ |= 0x08; // WAKEUP (resume from suspend)
    }
}

void Usb8x::tick(u64 elapsed_cycles) noexcept {
    bool usb_enabled = (ctrla_ & 0x01) != 0;
    // XMEGA has no PLLCSR register; PLL is always available when USB is.
    // AVR DU devices use PLLCSR bit 0 to enable the PLL.
    bool pll_enabled = (desc_.pllcsr_address == 0) || (pllcsr_ & 0x01) != 0;
    bool usb_running = usb_enabled && pll_enabled;
    if (usb_running && sof_frame_cycles_ > 0) {
        sof_cycle_counter_ += elapsed_cycles;
        if (sof_cycle_counter_ >= sof_frame_cycles_) {
            sof_cycle_counter_ -= sof_frame_cycles_;
            intflagsa_ |= 0x02;
        }
    }
    update_interrupt_state();
}

void Usb8x::update_interrupt_state() noexcept {
}

u8 Usb8x::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.ctrlb_address) return ctrlb_;
    if (address == desc_.busstate_address) return busstate_;
    if (address == desc_.addr_address) return addr_;
    if (address == desc_.fifowp_address) return fifowp_;
    if (address == desc_.fiforp_address) {
        u8 byte = 0;
        if (fifo_count_ > 0) {
            byte = fifo_buf_[fifo_rp_];
            fifo_rp_ = (fifo_rp_ + 1) & 0x3F;
            --fifo_count_;
        }
        return byte;
    }
    if (address == desc_.epptr_address) return epptr_;
    if (address == desc_.intctrla_address) return intctrla_;
    if (address == desc_.intctrlb_address) return intctrlb_;
    if (address == desc_.intflagsa_address) return intflagsa_;
    if (address == desc_.intflagsb_address) return intflagsb_;
    if (address == desc_.pllcsr_address) return pllcsr_;
    if (address == desc_.cal0_address) return 0;
    if (address == desc_.cal1_address) return 0;
    return 0;
}

void Usb8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
    }
    else if (address == desc_.ctrlb_address) {
        ctrlb_ = value;
        if (ctrlb_ & 0x01) {
            // RST: reset bus state to DISABLE
            busstate_ = 0;
            prev_bus_state_ = 0;
        }
        if (ctrlb_ & 0x02) {
            // ATTACH: transition to CONN state
            u8 old = busstate_ & 0x03U;
            busstate_ = (busstate_ & 0xFCU) | 0x02U;
            handle_bus_state_change(old, 2);
        }
        prev_bus_state_ = busstate_ & 0x03U;
    }
    else if (address == desc_.busstate_address) {
        u8 old = busstate_ & 0x03U;
        busstate_ = value;
        u8 new_s = busstate_ & 0x03U;
        handle_bus_state_change(old, new_s);
        prev_bus_state_ = new_s;
    }
    else if (address == desc_.addr_address) {
        addr_ = value;
    }
    else if (address == desc_.fifowp_address) {
        fifowp_ = value;
        if (fifo_count_ < 64) {
            fifo_buf_[fifo_wp_] = value;
            fifo_wp_ = (fifo_wp_ + 1) & 0x3F;
            ++fifo_count_;
            intflagsb_ |= 0x02;
            detect_setup_packet();
        }
    }
    else if (address == desc_.fiforp_address) {
        fiforp_ = value;
    }
    else if (address == desc_.epptr_address) {
        epptr_ = value;
        u8 ep_type = epptr_ & 0x07U;
        // Changing to SETUP type resets setup count
        if (ep_type == 4) {
            ep0_setup_count_ = 0;
        }
    }
    else if (address == desc_.intctrla_address) {
        intctrla_ = value;
    }
    else if (address == desc_.intctrlb_address) {
        intctrlb_ = value;
    }
    else if (address == desc_.intflagsa_address) {
        intflagsa_ &= ~(value & 0x0F);
    }
    else if (address == desc_.intflagsb_address) {
        intflagsb_ &= ~(value & 0x07);
    }
    else if (address == desc_.pllcsr_address) {
        pllcsr_ = value;
    }
}

bool Usb8x::pending_interrupt_request(InterruptRequest& request) const noexcept {
    u8 pending_a = intflagsa_ & intctrla_;
    u8 pending_b = intflagsb_ & intctrlb_;
    if (!(pending_a || pending_b)) return false;

    // Bus events (SUSPEND, WAKEUP, EORST) use bus_vector_index
    if (desc_.bus_vector_index && (pending_a & 0x0DU)) {
        request.vector_index = desc_.bus_vector_index;
        return true;
    }
    // Data events (SOF, SETUP, TRANS, STALL) use gen_vector_index
    if (desc_.gen_vector_index) {
        request.vector_index = desc_.gen_vector_index;
        return true;
    }
    return false;
}

bool Usb8x::consume_interrupt_request(InterruptRequest& request) noexcept {
    u8 pending_a = intflagsa_ & intctrla_;
    u8 pending_b = intflagsb_ & intctrlb_;
    if (!(pending_a || pending_b)) return false;

    if (desc_.bus_vector_index && (pending_a & 0x0DU)) {
        request.vector_index = desc_.bus_vector_index;
        // Only clear bus events
        intflagsa_ &= ~(pending_a & 0x0DU);
        return true;
    }
    if (desc_.gen_vector_index) {
        request.vector_index = desc_.gen_vector_index;
        intflagsa_ &= ~pending_a;
        intflagsb_ &= ~pending_b;
        return true;
    }
    return false;
}

} // namespace vioavr::core
