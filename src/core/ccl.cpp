#include "vioavr/core/ccl.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <algorithm>

namespace vioavr::core {

Ccl::Ccl(const CclDescriptor& desc) : desc_(desc) {
    size_t range_idx = 0;
    if (desc_.ctrla_address != 0) {
        ranges_[range_idx++] = {desc_.ctrla_address, desc_.ctrla_address};
    }
    
    // Check SEQCTRL range
    u16 min_seq = 0xFFFF, max_seq = 0x0000;
    bool has_seq = false;
    for (auto addr : desc_.seqctrl_addresses) {
        if (addr != 0) {
            min_seq = std::min(min_seq, addr);
            max_seq = std::max(max_seq, addr);
            has_seq = true;
        }
    }
    if (has_seq) ranges_[range_idx++] = {min_seq, max_seq};

    // INTCTRL/FLAGS
    for (int i=0; i<2; ++i) {
        if (desc_.intctrl_addresses[i] != 0) ranges_[range_idx++] = {desc_.intctrl_addresses[i], desc_.intctrl_addresses[i]};
        if (desc_.intflags_addresses[i] != 0) ranges_[range_idx++] = {desc_.intflags_addresses[i], desc_.intflags_addresses[i]};
    }

    // LUTs
    u16 min_lut = 0xFFFF, max_lut = 0x0000;
    bool has_lut = false;
    for (u8 i = 0; i < desc_.lut_count; ++i) {
        min_lut = std::min({min_lut, desc_.luts[i].ctrla_address, desc_.luts[i].truth_address});
        max_lut = std::max({max_lut, desc_.luts[i].ctrla_address, desc_.luts[i].truth_address});
        has_lut = true;
    }
    if (has_lut) ranges_[range_idx++] = {min_lut, max_lut};
}

void Ccl::reset() noexcept {
    ctrla_ = 0;
    std::fill(seqctrl_.begin(), seqctrl_.end(), 0);
    std::fill(intctrl_.begin(), intctrl_.end(), 0);
    std::fill(intflags_.begin(), intflags_.end(), 0);
    for (auto& lut : luts_) {
        lut.ctrla = 0; lut.ctrlb = 0; lut.ctrlc = 0; lut.truth = 0;
        std::fill(lut.inputs.begin(), lut.inputs.end(), false);
    }
    std::fill(outputs_.begin(), outputs_.end(), false);
    std::fill(prev_outputs_.begin(), prev_outputs_.end(), false);
    std::fill(seq_state_.begin(), seq_state_.end(), false);
}

bool Ccl::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (desc_.vector_index == 0) return false;
    
    for (int i=0; i<2; ++i) {
        if (intflags_[i] & intctrl_[i]) {
            request.vector_index = desc_.vector_index;
            return true;
        }
    }
    return false;
}

std::span<const AddressRange> Ccl::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

u8 Ccl::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    
    for (int i=0; i<4; ++i) if (address == desc_.seqctrl_addresses[i]) return seqctrl_[i];
    for (int i=0; i<2; ++i) if (address == desc_.intctrl_addresses[i]) return intctrl_[i];
    for (int i=0; i<2; ++i) if (address == desc_.intflags_addresses[i]) return intflags_[i];

    for (u8 i = 0; i < desc_.lut_count; ++i) {
        if (address == desc_.luts[i].ctrla_address) return luts_[i].ctrla;
        if (address == desc_.luts[i].ctrlb_address) return luts_[i].ctrlb;
        if (address == desc_.luts[i].ctrlc_address) return luts_[i].ctrlc;
        if (address == desc_.luts[i].truth_address) return luts_[i].truth;
    }
    return 0;
}

void Ccl::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) ctrla_ = value;
    else {
        bool found = false;
        for (int i=0; i<4; ++i) if (address == desc_.seqctrl_addresses[i]) { seqctrl_[i] = value; found = true; break; }
        if (!found) for (int i=0; i<2; ++i) if (address == desc_.intctrl_addresses[i]) { intctrl_[i] = value; found = true; break; }
        if (!found) for (int i=0; i<2; ++i) if (address == desc_.intflags_addresses[i]) { intflags_[i] &= ~value; found = true; break; }

        if (!found) {
            for (u8 i = 0; i < desc_.lut_count; ++i) {
                if (address == desc_.luts[i].ctrla_address) { luts_[i].ctrla = value; found = true; break; }
                if (address == desc_.luts[i].ctrlb_address) { luts_[i].ctrlb = value; found = true; break; }
                if (address == desc_.luts[i].ctrlc_address) { luts_[i].ctrlc = value; found = true; break; }
                if (address == desc_.luts[i].truth_address) { luts_[i].truth = value; found = true; break; }
            }
        }
    }
    update_logic();
}

void Ccl::tick(u64) noexcept {
    // Mostly asynchronous, update_logic already handles everything on change.
}

bool Ccl::compute_lut(u8 index) const noexcept {
    if (!(luts_[index].ctrla & 0x01)) return false; // Not enabled
    
    bool in[3] = {false, false, false};
    
    for (int j = 0; j < 3; ++j) {
        u8 insel = 0;
        if (j == 0) insel = luts_[index].ctrlb & 0x0F;
        else if (j == 1) insel = (luts_[index].ctrlb >> 4) & 0x0F;
        else if (j == 2) insel = luts_[index].ctrlc & 0x0F;

        switch (insel) {
            case 0x00: // MASK
                in[j] = false;
                break;
            case 0x05: // IO PIN
                in[j] = luts_[index].inputs[j];
                break;
            case 0x02: // LINK
                in[j] = outputs_[(index + desc_.lut_count - 1) % desc_.lut_count];
                break;
            case 0x01: // FEEDBACK
                in[j] = outputs_[index];
                break;
            default:
                // TODO: AC, EVSYS, TC sources
                in[j] = false;
                break;
        }
    }

    u8 pattern = (in[2] << 2) | (in[1] << 1) | (in[0] << 0);
    return (luts_[index].truth >> pattern) & 0x01;
}

void Ccl::update_logic() noexcept {
    if (!(ctrla_ & 0x01)) return; // Peripheral disabled

    // Iterative update for feedback/links
    for (int iter = 0; iter < 4; ++iter) {
        bool changed = false;
        for (u8 i = 0; i < desc_.lut_count; ++i) {
            bool old_val = outputs_[i];
            outputs_[i] = compute_lut(i);
            if (outputs_[i] != old_val) changed = true;
        }
        if (!changed) break;
    }

    // Sequential Units
    for (u8 s = 0; s < 2; ++s) {
        u8 mode = seqctrl_[s] & 0x07;
        if (mode == 0) continue;

        bool in0 = outputs_[s * 2];
        bool in1 = outputs_[s * 2 + 1];
        bool prev_in1 = prev_outputs_[s * 2 + 1];
        bool rising_edge_in1 = in1 && !prev_in1;
        
        switch (mode) {
            case 0x01: // D Flip-Flop
                if (rising_edge_in1) seq_state_[s] = in0;
                break;
            case 0x02: // JK Flip-Flop
                if (rising_edge_in1) {
                    bool J = in0;
                    bool K = in1;
                    if (J && !K) seq_state_[s] = true;
                    else if (!J && K) seq_state_[s] = false;
                    else if (J && K) seq_state_[s] = !seq_state_[s];
                }
                break;
            case 0x03: // D Latch
                if (in1) seq_state_[s] = in0;
                break;
            case 0x04: // RS Latch
                if (in0 && !in1) seq_state_[s] = true;
                else if (!in0 && in1) seq_state_[s] = false;
                else if (in0 && in1) seq_state_[s] = false; // Reset wins
                break;
            default: break;
        }
    }

    // Interrupts & Output Synchronization
    for (u8 i = 0; i < desc_.lut_count; ++i) {
        if (outputs_[i] != prev_outputs_[i]) {
            // Trigger Interrupts
            u8 intmode = (intctrl_[i / 2] >> ((i % 2) * 4)) & 0x03;
            bool trigger = (intmode == 0x01 && outputs_[i]) ||
                           (intmode == 0x02 && !outputs_[i]) ||
                           (intmode == 0x03);
            
            if (trigger) intflags_[i / 2] |= (1 << (i % 2));

            // TODO: Drive physical pin if (luts_[i].ctrla & 0x40) (OUTEN)
        }
    }

    prev_outputs_ = outputs_;
}

void Ccl::set_pin_input(u8 lut_index, u8 input_index, bool level) noexcept {
    if (lut_index < 4 && input_index < 3) {
        if (luts_[lut_index].inputs[input_index] != level) {
            luts_[lut_index].inputs[input_index] = level;
            update_logic();
        }
    }
}

} // namespace vioavr::core
