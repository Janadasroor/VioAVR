#include "vioavr/core/ccl.hpp"
#include <algorithm>

namespace vioavr::core {

Ccl::Ccl(const CclDescriptor& desc) : desc_(desc) {
    size_t range_idx = 0;
    if (desc_.ctrla_address != 0) {
        ranges_[range_idx++] = {desc_.ctrla_address, desc_.ctrla_address};
    }
    
    // Check SEQCTRL range (often contiguous)
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
    }
    std::fill(outputs_.begin(), outputs_.end(), false);
    std::fill(seq_state_.begin(), seq_state_.end(), false);
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
        if (!found) for (int i=0; i<2; ++i) if (address == desc_.intflags_addresses[i]) { intflags_[i] &= ~value; found = true; break; } // Clear on 1

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
    // Mostly asynchronous, but sequential logic might need a "clock" if selected
}

bool Ccl::compute_lut(u8 index) const noexcept {
    if (!(luts_[index].ctrla & 0x01)) return false; // Not enabled
    
    // Evaluate 3 inputs based on INSEL
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
            case 0x01: // FEEDBACK
                in[j] = outputs_[index];
                break;
            case 0x02: // LINK
                in[j] = outputs_[(index + 3) % 4]; // 0->3, 1->0, 2->1, 3->2
                break;
            case 0x05: // IO PIN
                in[j] = luts_[index].inputs[j];
                break;
            default:
                // Other sources (AC, EVENT, etc.) - assume false for now
                in[j] = false;
                break;
        }
    }

    // Truth Table: bits 0..7 correspond to input pattern 000..111
    u8 pattern = (in[2] << 2) | (in[1] << 1) | (in[0] << 0);
    return (luts_[index].truth >> pattern) & 0x01;
}

void Ccl::update_logic() noexcept {
    if (!(ctrla_ & 0x01)) return; // Peripheral disabled

    // Iterative update for feedback/links (limit iterations to avoid loops)
    for (int iter = 0; iter < 4; ++iter) {
        bool changed = false;
        for (u8 i = 0; i < desc_.lut_count; ++i) {
            bool old_val = outputs_[i];
            outputs_[i] = compute_lut(i);
            if (outputs_[i] != old_val) changed = true;
        }
        if (!changed) break;
    }
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
