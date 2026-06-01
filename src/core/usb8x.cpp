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
    for (auto& ep : endpoints_) {
        ep.type = 0;
        ep.number = 0;
        ep.is_in = false;
        ep.data_toggle = false;
        ep.stalled = false;
        ep.data_count = 0;
    }
}

void Usb8x::detect_setup_packet() noexcept {
    u8 ep_type = epptr_ & 0x07U;
    u8 ep_num = (epptr_ >> 4) & 0x07U;
    if (ep_type != 4 || ep_num != 0) return;

    u8 last_byte = fifo_buf_[(fifo_wp_ == 0) ? 63 : (fifo_wp_ - 1)];
    if (ep0_setup_count_ < 8) {
        ep0_setup_pkt_[ep0_setup_count_] = last_byte;
    }
    ++ep0_setup_count_;

    if (ep0_setup_count_ >= 8 && fifo_count_ >= 8) {
        ep0_data_dir_ = (ep0_setup_pkt_[0] & 0x80) ? 1 : 0;
        ep0_wLength_ = static_cast<u16>(ep0_setup_pkt_[6]) |
                       (static_cast<u16>(ep0_setup_pkt_[7]) << 8);
        ep0_stalled_ = false;
        intflagsb_ |= 0x04;
    }
}

void Usb8x::handle_bus_state_change(u8 old_state, u8 new_state) noexcept {
    if (old_state == new_state) return;

    if (new_state == 3) {
        intflagsa_ |= 0x01;
    } else if (new_state == 2 && old_state == 1) {
        intflagsa_ |= 0x04;
    } else if (new_state == 1 && old_state == 3) {
        intflagsa_ |= 0x08;
    }
}

void Usb8x::tick(u64 elapsed_cycles) noexcept {
    bool usb_enabled = (ctrla_ & 0x01) != 0;
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
        // If current EPPTR points to an OUT/SETUP endpoint with pending SIE data,
        // return SIE data first (before the regular FIFO).
        // SIE data for setup/out is in the FIFO (sie_push_fifo).
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
            busstate_ = 0;
            prev_bus_state_ = 0;
        }
        if (ctrlb_ & 0x02) {
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

    if (desc_.bus_vector_index && (pending_a & 0x0DU)) {
        request.vector_index = desc_.bus_vector_index;
        return true;
    }
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

// ── SIE Simulation ───────────────────────────────────────

void Usb8x::simulate_usb_reset() noexcept {
    busstate_ = 0;
    prev_bus_state_ = 0;
    addr_ = 0;
    intflagsa_ &= ~0x0FU;
    intflagsb_ &= ~0x07U;
    intflagsa_ |= 0x04;
    ep0_setup_count_ = 0;
    ep0_stalled_ = false;
    ep0_data_dir_ = 0;
    ep0_wLength_ = 0;
    for (auto& ep : endpoints_) {
        ep.type = 0;
        ep.number = 0;
        ep.is_in = false;
        ep.data_toggle = false;
        ep.stalled = false;
        ep.data_count = 0;
    }
}

void Usb8x::simulate_setup_packet(const SetupPacket& setup) noexcept {
    // Write 8-byte setup packet into FIFO at the current write position
    u8 pkt[8];
    pkt[0] = setup.bmRequestType;
    pkt[1] = setup.bRequest;
    pkt[2] = static_cast<u8>(setup.wValue & 0xFF);
    pkt[3] = static_cast<u8>((setup.wValue >> 8) & 0xFF);
    pkt[4] = static_cast<u8>(setup.wIndex & 0xFF);
    pkt[5] = static_cast<u8>((setup.wIndex >> 8) & 0xFF);
    pkt[6] = static_cast<u8>(setup.wLength & 0xFF);
    pkt[7] = static_cast<u8>((setup.wLength >> 8) & 0xFF);

    for (int i = 0; i < 8 && fifo_count_ < 64; ++i) {
        fifo_buf_[fifo_wp_] = pkt[i];
        fifo_wp_ = (fifo_wp_ + 1) & 0x3F;
        ++fifo_count_;
    }

    // Copy to EP0 setup packet tracking
    for (int i = 0; i < 8; ++i) {
        ep0_setup_pkt_[i] = pkt[i];
    }
    ep0_setup_count_ = 8;

    ep0_data_dir_ = (setup.bmRequestType & 0x80) ? 1 : 0;
    ep0_wLength_ = setup.wLength;
    ep0_stalled_ = false;

    // Store in endpoint 0 data buffer (OUT direction = SIE has data for CPU)
    auto& ep0 = endpoints_[0];
    ep0.type = 4;
    ep0.number = 0;
    ep0.is_in = false; // SETUP is always OUT (SIE→CPU)
    for (int i = 0; i < 8 && i < 64; ++i) {
        ep0.data[i] = pkt[i];
    }
    ep0.data_count = 8;

    intflagsb_ |= 0x04; // SETUP flag
}

void Usb8x::simulate_out_packet(u8 ep_num, std::span<const u8> data) noexcept {
    if (ep_num >= 8) return;
    auto& ep = endpoints_[ep_num];
    ep.type = get_ep_type();
    ep.number = ep_num;
    ep.is_in = false;

    // Check STALL
    if (ep.stalled) {
        return;
    }

    // Write data into FIFO at current write position
    for (size_t i = 0; i < data.size() && fifo_count_ < 64; ++i) {
        fifo_buf_[fifo_wp_] = data[i];
        fifo_wp_ = (fifo_wp_ + 1) & 0x3F;
        ++fifo_count_;
    }

    // Store in endpoint buffer
    size_t to_copy = data.size() > 64 ? 64 : data.size();
    for (size_t i = 0; i < to_copy; ++i) {
        ep.data[i] = data[i];
    }
    ep.data_count = static_cast<u8>(to_copy);

    // Toggle DATA0/DATA1
    ep.data_toggle = !ep.data_toggle;

    intflagsb_ |= 0x02; // TRANS flag
}

bool Usb8x::simulate_in_token(u8 ep_num) noexcept {
    if (ep_num >= 8) return false;
    auto& ep = endpoints_[ep_num];
    ep.type = get_ep_type();
    ep.number = ep_num;
    ep.is_in = true;

    if (ep.stalled) {
        return false;
    }

    // Read data from FIFO at the read position (where CPU wrote it)
    // We read up to whatever the CPU has written
    u8 temp[64];
    u8 count = 0;
    while (fifo_count_ > 0 && count < 64) {
        temp[count] = fifo_buf_[fifo_rp_];
        fifo_rp_ = (fifo_rp_ + 1) & 0x3F;
        --fifo_count_;
        ++count;
    }

    // Store what we read in the endpoint buffer (for host test to verify)
    if (count > 0) {
        for (u8 i = 0; i < count; ++i) {
            ep.data[i] = temp[i];
        }
        ep.data_count = count;
    }

    ep.data_toggle = !ep.data_toggle;
    intflagsb_ &= ~0x02U; // Clear TRANS flag (data consumed)

    return count > 0;
}

std::span<const u8> Usb8x::get_endpoint_data(u8 ep_num) const noexcept {
    if (ep_num >= 8) return {};
    const auto& ep = endpoints_[ep_num];
    if (ep.data_count == 0) return {};
    return {ep.data, ep.data_count};
}

} // namespace vioavr::core
