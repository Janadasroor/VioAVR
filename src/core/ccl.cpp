#include "vioavr/core/ccl.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/tca.hpp"
#include "vioavr/core/tcb.hpp"
#include "vioavr/core/ac8x.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <algorithm>

namespace vioavr::core {

Ccl::Ccl(const CclDescriptor& desc) : desc_(desc),
    luts_(desc_.lut_count),
    outputs_(desc_.lut_count, 0),
    raw_outputs_(desc_.lut_count, 0),
    prev_outputs_(desc_.lut_count, 0),
    prev_raw_outputs_(desc_.lut_count, 0),
    prev_luts_in2_(2, 0)
{
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

Ccl::~Ccl() noexcept {
    if (!evsys_) return;
    for (const auto& lut : luts_) {
        if (lut.user_a_index != 0xFF) evsys_->unregister_user_callback(lut.user_a_index);
        if (lut.user_b_index != 0xFF) evsys_->unregister_user_callback(lut.user_b_index);
    }
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
    std::fill(outputs_.begin(), outputs_.end(), 0);
    std::fill(raw_outputs_.begin(), raw_outputs_.end(), 0);
    std::fill(prev_outputs_.begin(), prev_outputs_.end(), 0);
    std::fill(prev_raw_outputs_.begin(), prev_raw_outputs_.end(), 0);
    std::fill(prev_luts_in2_.begin(), prev_luts_in2_.end(), 0);
    std::fill(seq_state_.begin(), seq_state_.end(), false);
    update_routing();
    update_logic();
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

void Ccl::set_memory_bus(MemoryBus* bus) noexcept {
    bus_ = bus;
    if (!bus_) return;

    tca0_ = static_cast<Tca*>(bus_->get_peripheral_by_name("TCA0"));
    for (int i = 0; i < 4; ++i) {
        static constexpr std::string_view kTcbNames[] = {"TCB0", "TCB1", "TCB2", "TCB3"};
        tcbs_[i] = static_cast<Tcb*>(bus_->get_peripheral_by_name(kTcbNames[i]));
    }
    ac0_ = static_cast<Ac8x*>(bus_->get_peripheral_by_name("AC0"));
}

void Ccl::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    if (!evsys_) return;
    
    // Register callbacks for user events to trigger update_logic
    for (u8 i = 0; i < desc_.lut_count; ++i) {
        u16 user_a_addr = desc_.luts[i].user_event_a_address;
        u16 user_b_addr = desc_.luts[i].user_event_b_address;
        
        if (user_a_addr != 0) {
            luts_[i].user_a_index = static_cast<u8>(user_a_addr - evsys_->users_base());
            evsys_->register_user_callback(luts_[i].user_a_index, [this](bool) { update_logic(); });
        }
        if (user_b_addr != 0) {
            luts_[i].user_b_index = static_cast<u8>(user_b_addr - evsys_->users_base());
            evsys_->register_user_callback(luts_[i].user_b_index, [this](bool) { update_logic(); });
        }
    }
}

void Ccl::set_pin_mux(PinMux* pm) noexcept {
    pin_mux_ = pm;
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
        if (!found) for (int i=0; i<2; ++i) if (address == desc_.intflags_addresses[i]) { intflags_[i] &= ~value; update_interrupt_state(); found = true; break; }

        if (!found) {
            for (u8 i = 0; i < desc_.lut_count; ++i) {
                if (address == desc_.luts[i].ctrla_address) { luts_[i].ctrla = value; found = true; break; }
                if (address == desc_.luts[i].ctrlb_address) { luts_[i].ctrlb = value; found = true; break; }
                if (address == desc_.luts[i].ctrlc_address) { luts_[i].ctrlc = value; found = true; break; }
                if (address == desc_.luts[i].truth_address) { luts_[i].truth = value; found = true; break; }
            }
        }
        if (!found) return;
    }
    update_routing(); 
    update_logic();
}

void Ccl::tick(u64) noexcept {
    // Logic updates on writes or events, but we might want to refresh periodicly if inputs are from timers
    update_logic();
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
            case 0x01: // FEEDBACK (use pre-seq raw output)
                in[j] = raw_outputs_[index];
                break;
            case 0x02: // LINK
                in[j] = outputs_[(index + desc_.lut_count - 1) % desc_.lut_count];
                break;
            case 0x03: // EVENTA
                if (evsys_ && luts_[index].user_a_index != 0xFF) 
                    in[j] = evsys_->get_user_level(luts_[index].user_a_index);
                break;
            case 0x04: // EVENTB
                if (evsys_ && luts_[index].user_b_index != 0xFF) 
                    in[j] = evsys_->get_user_level(luts_[index].user_b_index);
                break;
            case 0x05: // IO PIN
                in[j] = luts_[index].inputs[j];
                break;
            case 0x06: // AC
                if (ac0_) in[j] = ac0_->get_state();
                break;
            case 0x0A: // TCA0
                if (tca0_) in[j] = tca0_->get_wo_level(j); // WO0, WO1, WO2 for IN0, IN1, IN2
                break;
            case 0x0C: // TCB
                if (tcbs_[index]) in[j] = tcbs_[index]->get_wo_level();
                break;
            default:
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
    const auto initial_outputs = outputs_;
    for (int iter = 0; iter < 100; ++iter) {
        bool changed = false;
        for (u8 i = 0; i < desc_.lut_count; ++i) {
            bool old_val = outputs_[i];
            outputs_[i] = compute_lut(i);
            if (outputs_[i] != old_val) changed = true;
        }
        if (!changed) break;
        // Oscillation detection: if state returns to initial after even # of passes
        if ((iter & 1) && iter >= 1) {
            bool back = true;
            for (u8 i = 0; i < desc_.lut_count && back; ++i) {
                if (outputs_[i] != initial_outputs[i]) back = false;
            }
            if (back) break;
        }
    }

    // Sequential Units
    raw_outputs_ = outputs_;
    for (u8 s = 0; s < 2; ++s) {
        u8 mode = seqctrl_[s] & 0x07;
        if (mode == 0) continue;

        // The sequential units use the RAW outputs of the LUTs as inputs.
        // We use compute_lut to get the raw state without the SEQ override.
        bool in0 = compute_lut(s * 2 + 1); // Input 0 = LUT(2n+1)
        bool in1 = compute_lut(s * 2);     // Input 1 = LUT(2n)
        bool in2 = luts_[s * 2].inputs[2]; // Input 2 = IN2 of LUT(2n)

        bool prev_in1 = prev_raw_outputs_[s * 2];
        bool rising_edge_in1 = in1 && !prev_in1;

        bool prev_in2 = prev_luts_in2_[s];
        bool rising_edge_in2 = in2 && !prev_in2;
        
        switch (mode) {
            case 0x01: // D Flip-Flop
                if (rising_edge_in1) {
                    seq_state_[s] = in0;
                }
                break;
            case 0x02: // JK Flip-Flop
                if (rising_edge_in2) {
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
                // S=1, R=1: hold previous state (do nothing)
                break;
            default: break;
        }
        prev_raw_outputs_[s * 2] = in1;
        prev_raw_outputs_[s * 2 + 1] = in0;
        prev_luts_in2_[s] = in2;
        
        // When sequential unit is enabled, it overrides LUT(2n) output
        outputs_[s * 2] = seq_state_[s];
    }

    // Interrupts & Output Synchronization
    for (u8 i = 0; i < desc_.lut_count; ++i) {
        if (outputs_[i] != prev_outputs_[i]) {
            // Trigger Interrupts
            u8 intmode = (intctrl_[i / 2] >> ((i % 2) * 4)) & 0x03;
            bool trigger = (intmode == 0x01 && outputs_[i]) ||
                           (intmode == 0x02 && !outputs_[i]) ||
                           (intmode == 0x03);
            
            if (trigger) {
                intflags_[i / 2] |= (1 << (i % 2));
                update_interrupt_state();
            }

            // Trigger Event System
            if (evsys_ && desc_.luts[i].generator_id != 0) {
                evsys_->trigger_event(desc_.luts[i].generator_id, outputs_[i]);
            }
        }
        
        // Drive physical pin if (luts_[i].ctrla & 0x40) (OUTEN)
        if (pin_mux_ && (luts_[i].ctrla & 0x40) && desc_.luts[i].output_pin_address != 0) {
            pin_mux_->update_pin_by_address(desc_.luts[i].output_pin_address, desc_.luts[i].output_bit_index,
                PinOwner::ccl, true, outputs_[i], false);
        }
    }

    prev_outputs_ = outputs_;
}

void Ccl::set_pin_input(u8 lut_index, u8 input_index, bool level) noexcept {
    if (lut_index < desc_.lut_count && input_index < 3) {
        if (luts_[lut_index].inputs[input_index] != level) {
            luts_[lut_index].inputs[input_index] = level;
            update_logic();
        }
    }
}

void Ccl::update_routing() noexcept {
    if (!pin_mux_) return;
    for (u8 i = 0; i < desc_.lut_count; ++i) {
        u16 pin_addr = desc_.luts[i].output_pin_address;
        u8 pin_bit = desc_.luts[i].output_bit_index;
        if (pin_addr == 0) continue;

        if (ctrla_ & 0x01 && (luts_[i].ctrla & 0x40)) {
            pin_mux_->claim_pin_by_address(pin_addr, pin_bit, PinOwner::ccl);
        } else {
            pin_mux_->release_pin_by_address(pin_addr, pin_bit, PinOwner::ccl);
        }
    }
}
void Ccl::update_interrupt_state() noexcept {
    InterruptRequest req;
    set_interrupt_pending(pending_interrupt_request(req));
}
} // namespace vioavr::core
