#include "vioavr/core/can.hpp"
#include <algorithm>
#include <iostream>

namespace vioavr::core {

CanBus::CanBus(std::string_view name, const CanDescriptor& descriptor) noexcept
    : name_(name), desc_(descriptor), mobs_(descriptor.mob_count)
{
    // Collect all valid register addresses
    // We group them into simple ranges since they typically span from 0xD8 to 0xFA in AT90CAN.
    // Instead of exhaustive ranges, we just define a single block.
    // Let's deduce the span.
    u16 min_addr = 0xFFFF;
    u16 max_addr = 0x0000;

    auto check = [&](u16 addr) {
        if (addr != 0) {
            min_addr = std::min(min_addr, addr);
            max_addr = std::max(max_addr, addr);
        }
    };

    check(desc_.cangcon_address);
    check(desc_.cangsta_address);
    check(desc_.cangit_address);
    check(desc_.cangie_address);
    check(desc_.canen1_address);
    check(desc_.canen2_address);
    check(desc_.canie1_address);
    check(desc_.canie2_address);
    check(desc_.cansit1_address);
    check(desc_.cansit2_address);
    check(desc_.canbt1_address);
    check(desc_.canbt2_address);
    check(desc_.canbt3_address);
    check(desc_.cantcon_address);
    check(desc_.cantim_address);
    check(desc_.canttc_address);
    check(desc_.cantec_address);
    check(desc_.canrec_address);
    check(desc_.canhpmob_address);
    check(desc_.canpage_address);
    check(desc_.canstmob_address);
    check(desc_.cancdmob_address);
    check(desc_.canidt_address);
    check(desc_.canidm_address);
    check(desc_.canstm_address);
    check(desc_.canmsg_address);

    if (min_addr <= max_addr) {
        // canidt and canidm actually cover 4 bytes each, cantim/canttc/canstm cover 2 bytes.
        // Assuming max address typically is CANMSG = 0xFA or CANIDx = 0xF7.
        // Adding conservative +3 to max to cover multi-byte registers if they are at the very end.
        ranges_.push_back({min_addr, static_cast<u16>(max_addr + 4)});
    }
}

std::string_view CanBus::name() const noexcept {
    return name_;
}

std::span<const AddressRange> CanBus::mapped_ranges() const noexcept {
    return ranges_;
}

void CanBus::reset() noexcept {
    cangcon_ = 0;
    cangsta_ = 0;
    cangit_ = 0;
    cangie_ = 0;
    canen1_ = 0;
    canen2_ = 0;
    canie1_ = 0;
    canie2_ = 0;
    cansit1_ = 0;
    cansit2_ = 0;
    canbt1_ = 0;
    canbt2_ = 0;
    canbt3_ = 0;
    cantcon_ = 0;
    cantim_ = 0;
    canttc_ = 0;
    cantec_ = 0;
    canrec_ = 0;
    canhpmob_ = 0;
    canpage_ = 0;

    for (auto& mob : mobs_) {
        mob = MessageObject();
    }

    canit_pending_ = false;
    ovrit_pending_ = false;
    timer_prescaler_cycles_ = 0;
}

void CanBus::tick(u64 elapsed_cycles) noexcept {
    if (!(cangcon_ & 0x02)) { // ENASTB Enable/Standby bit
        return;
    }

    // Increment CAN Timer
    // Timer prescaler cantcon_ gives prescaler value (e.g., 0-255).
    // Actual prescale is generally (CANTCON + 1) * 8.
    u32 target_cycles = (cantcon_ + 1) * 8;
    timer_prescaler_cycles_ += elapsed_cycles;
    while (timer_prescaler_cycles_ >= target_cycles) {
        timer_prescaler_cycles_ -= target_cycles;
        cantim_++;
        if (cantim_ == 0) { // Overflow
            cangit_ |= 0x20; // OVRTIM
            evaluate_interrupts();
        }
    }
}

u8 CanBus::read(u16 address) noexcept {
    if (address == desc_.cangcon_address) return cangcon_;
    if (address == desc_.cangsta_address) return cangsta_;
    if (address == desc_.cangit_address) return cangit_;
    if (address == desc_.cangie_address) return cangie_;
    if (address == desc_.canen1_address) return canen1_;
    if (address == desc_.canen2_address) return canen2_;
    if (address == desc_.canie1_address) return canie1_;
    if (address == desc_.canie2_address) return canie2_;
    if (address == desc_.cansit1_address) return cansit1_;
    if (address == desc_.cansit2_address) return cansit2_;
    if (address == desc_.canbt1_address) return canbt1_;
    if (address == desc_.canbt2_address) return canbt2_;
    if (address == desc_.canbt3_address) return canbt3_;
    if (address == desc_.cantcon_address) return cantcon_;
    if (address == desc_.cantim_address) return cantim_ & 0xFF; // CANTIML
    if (address == desc_.cantim_address + 1) return (cantim_ >> 8) & 0xFF; // CANTIMH
    if (address == desc_.canttc_address) return canttc_ & 0xFF;
    if (address == desc_.canttc_address + 1) return (canttc_ >> 8) & 0xFF;
    if (address == desc_.cantec_address) return cantec_;
    if (address == desc_.canrec_address) return canrec_;
    if (address == desc_.canhpmob_address) return canhpmob_;
    if (address == desc_.canpage_address) return canpage_;

    u8 mob_idx = (canpage_ >> 4) & 0x0F;
    if (mob_idx >= mobs_.size()) return 0;
    auto& mob = mobs_[mob_idx];

    if (address == desc_.canstmob_address) return mob.canstmob;
    if (address == desc_.cancdmob_address) return mob.cancdmob;
    
    if (address >= desc_.canidt_address && address < desc_.canidt_address + 4) {
        return mob.canidtags[address - desc_.canidt_address];
    }
    if (address >= desc_.canidm_address && address < desc_.canidm_address + 4) {
        return mob.canidmasks[address - desc_.canidm_address];
    }
    
    if (address == desc_.canstm_address) return mob.canstm & 0xFF;
    if (address == desc_.canstm_address + 1) return (mob.canstm >> 8) & 0xFF;

    if (address == desc_.canmsg_address) {
        u8 data_idx = canpage_ & 0x07;
        u8 val = mob.data[data_idx];
        if (canpage_ & 0x08) { // AINC (Auto Increment)
            canpage_ = (canpage_ & 0xF8) | ((data_idx + 1) & 0x07);
        }
        return val;
    }

    return 0;
}

void CanBus::write(u16 address, u8 value) noexcept {
    if (address == desc_.cangcon_address) {
        cangcon_ = value;
        if (value & 0x01) { // SWRES
            reset();
        }
        return;
    }
    if (address == desc_.cangit_address) {
        // Writing 1 clears the flag
        cangit_ &= ~value;
        evaluate_interrupts();
        return;
    }
    if (address == desc_.cangie_address) {
        cangie_ = value;
        evaluate_interrupts();
        return;
    }
    if (address == desc_.canie1_address) { canie1_ = value; evaluate_interrupts(); return; }
    if (address == desc_.canie2_address) { canie2_ = value; evaluate_interrupts(); return; }
    
    if (address == desc_.canbt1_address) { canbt1_ = value; return; }
    if (address == desc_.canbt2_address) { canbt2_ = value; return; }
    if (address == desc_.canbt3_address) { canbt3_ = value; return; }
    if (address == desc_.cantcon_address) { cantcon_ = value; return; }
    
    // CANTIM is read-only according to datasheet, but let's allow it for sim setup if needed
    if (address == desc_.cantim_address) { cantim_ = (cantim_ & 0xFF00) | value; return; }
    if (address == desc_.cantim_address + 1) { cantim_ = (cantim_ & 0x00FF) | (value << 8); return; }

    if (address == desc_.canttc_address) { canttc_ = (canttc_ & 0xFF00) | value; return; }
    if (address == desc_.canttc_address + 1) { canttc_ = (canttc_ & 0x00FF) | (value << 8); return; }

    if (address == desc_.canpage_address) {
        canpage_ = value;
        return;
    }

    u8 mob_idx = (canpage_ >> 4) & 0x0F;
    if (mob_idx >= mobs_.size()) return;
    auto& mob = mobs_[mob_idx];

    if (address == desc_.canstmob_address) {
        mob.canstmob &= ~value; // Write 1 to clear
        // Update CANSIT
        bool mob_interrupt = (mob.canstmob != 0);
        if (mob_idx < 8) {
            if (mob_interrupt) cansit2_ |= (1 << mob_idx);
            else cansit2_ &= ~(1 << mob_idx);
        } else {
            if (mob_interrupt) cansit1_ |= (1 << (mob_idx - 8));
            else cansit1_ &= ~(1 << (mob_idx - 8));
        }
        evaluate_interrupts();
        return;
    }
    if (address == desc_.cancdmob_address) {
        mob.cancdmob = value;
        u8 conf = (value >> 6) & 0x03;
        if (conf != 0) { // Enabled
            if (mob_idx < 8) canen2_ |= (1 << mob_idx);
            else canen1_ |= (1 << (mob_idx - 8));
        } else {
            if (mob_idx < 8) canen2_ &= ~(1 << mob_idx);
            else canen1_ &= ~(1 << (mob_idx - 8));
        }
        return;
    }

    if (address >= desc_.canidt_address && address < desc_.canidt_address + 4) {
        mob.canidtags[address - desc_.canidt_address] = value;
        return;
    }
    if (address >= desc_.canidm_address && address < desc_.canidm_address + 4) {
        mob.canidmasks[address - desc_.canidm_address] = value;
        return;
    }

    if (address == desc_.canmsg_address) {
        u8 data_idx = canpage_ & 0x07;
        mob.data[data_idx] = value;
        if (canpage_ & 0x08) { // AINC
            canpage_ = (canpage_ & 0xF8) | ((data_idx + 1) & 0x07);
        }
        return;
    }
}

void CanBus::evaluate_interrupts() noexcept {
    bool canit = false;
    bool ovrit = false;

    // Check OVRIT
    if ((cangie_ & 0x80) && (cangie_ & 0x01) && (cangit_ & 0x20)) {
        ovrit = true;
    }

    // Check CANIT
    if (cangie_ & 0x80) {
        // Any mob interrupts?
        u16 it_enabled = (canie1_ << 8) | canie2_;
        u16 it_status = (cansit1_ << 8) | cansit2_;
        if ((it_enabled & it_status) != 0 && (cangie_ & 0x20)) { // ENRX logic simplified
            canit = true;
        }

        // Other flags in CANGIT
        if ((cangit_ & 0x40) && (cangie_ & 0x40)) canit = true; // BOFFIT
        if ((cangit_ & 0x10) && (cangie_ & 0x04)) canit = true; // BXOK
        // More specific general errors could be checked here
    }

    canit_pending_ = canit;
    ovrit_pending_ = ovrit;
}

bool CanBus::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (ovrit_pending_ && desc_.ovrit_vector_index > 0) {
        request = {desc_.ovrit_vector_index, 0};
        return true;
    }
    if (canit_pending_ && desc_.canit_vector_index > 0) {
        request = {desc_.canit_vector_index, 0};
        return true;
    }
    return false;
}

bool CanBus::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (ovrit_pending_ && desc_.ovrit_vector_index > 0) {
        request = {desc_.ovrit_vector_index, 0};
        // OVRTIM needs to be cleared by software usually.
        return true;
    }
    if (canit_pending_ && desc_.canit_vector_index > 0) {
        request = {desc_.canit_vector_index, 0};
        return true;
    }
    return false;
}

void CanBus::receive_message(const CanMessage& msg) noexcept {
    if (!(cangcon_ & 0x02)) return; // not enabled

    for (u8 idx = 0; idx < mobs_.size(); ++idx) {
        auto& mob = mobs_[idx];
        u8 conf = (mob.cancdmob >> 6) & 0x03;
        if (conf == 2) { // Enable Receive
            // Simple ID matching (omitting RTR logic here for brevity)
            bool extended = (mob.cancdmob & 0x10) != 0;
            // Compare IDs and IDMs...
            // If match:
            mob.canstmob |= 0x20; // RXOK
            mob.canstm = cantim_;
            
            u8 dlc = std::min(static_cast<u8>(msg.data.size()), static_cast<u8>(8));
            mob.cancdmob = (mob.cancdmob & 0xF0) | dlc;
            
            for(u8 i = 0; i < dlc; ++i) {
                mob.data[i] = msg.data[i];
            }

            // Update CANSIT
            if (idx < 8) cansit2_ |= (1 << idx);
            else cansit1_ |= (1 << (idx - 8));

            evaluate_interrupts();
            break; // Message consumed by this MOb
        }
    }
}

void CanBus::inject_message(const CanMessage& msg) noexcept {
    receive_message(msg);
}

}  // namespace vioavr::core
