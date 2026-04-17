#include "vioavr/core/can.hpp"
#include <algorithm>

namespace vioavr::core {

CanBus::CanBus(std::string_view name, const CanDescriptor& descriptor) noexcept
    : name_(name), desc_(descriptor)
{
    mobs_.resize(desc_.mob_count);
    
    // Memory mapping for CAN peripheral
    // Registers are usually clustered, so we add a range for the whole block
    // Find min and max addresses from the descriptor
    u16 min_addr = 0xFFFFU;
    u16 max_addr = 0U;
    
    auto update_range = [&](u16 addr) {
        if (addr != 0) {
            min_addr = std::min(min_addr, addr);
            max_addr = std::max(max_addr, addr);
        }
    };
    
    update_range(desc_.cangcon_address);
    update_range(desc_.cangsta_address);
    update_range(desc_.cangit_address);
    update_range(desc_.cangie_address);
    update_range(desc_.canen1_address);
    update_range(desc_.canen2_address);
    update_range(desc_.canie1_address);
    update_range(desc_.canie2_address);
    update_range(desc_.cansit1_address);
    update_range(desc_.cansit2_address);
    update_range(desc_.canbt1_address);
    update_range(desc_.canbt2_address);
    update_range(desc_.canbt3_address);
    update_range(desc_.cantcon_address);
    update_range(desc_.cantim_address);
    update_range(desc_.canttc_address);
    update_range(desc_.cantec_address);
    update_range(desc_.canrec_address);
    update_range(desc_.canhpmob_address);
    update_range(desc_.canpage_address);
    update_range(desc_.canstmob_address);
    update_range(desc_.cancdmob_address);
    update_range(desc_.canidt_address);
    update_range(desc_.canidm_address);
    update_range(desc_.canstm_address);
    update_range(desc_.canmsg_address);
    
    // Add extra 3 for CANIDT/M if they are bases
    max_addr = std::max(max_addr, static_cast<u16>(desc_.canidt_address + 3U));
    max_addr = std::max(max_addr, static_cast<u16>(desc_.canidm_address + 3U));
    
    ranges_.push_back({min_addr, max_addr});
}

std::string_view CanBus::name() const noexcept { return name_; }
std::span<const AddressRange> CanBus::mapped_ranges() const noexcept { return ranges_; }

void CanBus::reset() noexcept {
    cangcon_ = 0;
    cangsta_ = 0x01; // ENA (waiting for internal reset? No, ENA bit 0 is enable)
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
        mob.canstmob = 0;
        mob.cancdmob = 0;
        mob.canstm = 0;
        std::fill(mob.data.begin(), mob.data.end(), 0);
    }
}

void CanBus::tick(u64 elapsed_cycles) noexcept {
    // Basic timer logic
    if (cangcon_ & 0x02U) { // ENA bit 1 is enable for AT90CAN
        // T_Q = (BRP + 1) * (1 / F_CPU)
        // BRP is bits 5:0 of CANBT1
        u32 brp = (canbt1_ & 0x3FU);
        u32 cycles_per_tq = brp + 1;
        
        timer_prescaler_cycles_ += static_cast<u32>(elapsed_cycles);
        while (timer_prescaler_cycles_ >= cycles_per_tq) {
            timer_prescaler_cycles_ -= cycles_per_tq;
            
            // CANTIM increments every TQ * (number of segments)?
            // Actually, CANTIM increments every CAN timer clock.
            // Simplified: increment every TQ for now.
            cantim_++;
            if (cantim_ == 0) { // Overflow
                cangit_ |= 0x01U; // OVRIT
                evaluate_interrupts();
            }
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
    
    // 16-bit Timer read logic (standard AVR temp register pattern)
    if (address == desc_.cantim_address) {
        timer_temp_ = static_cast<u8>(cantim_ >> 8U);
        return static_cast<u8>(cantim_ & 0xFFU);
    }
    if (address == desc_.cantim_address + 1) return timer_temp_;
    
    if (address == desc_.canttc_address) {
        timer_temp_ = static_cast<u8>(canttc_ >> 8U);
        return static_cast<u8>(canttc_ & 0xFFU);
    }
    if (address == desc_.canttc_address + 1) return timer_temp_;

    if (address == desc_.cantec_address) return cantec_;
    if (address == desc_.canrec_address) return canrec_;
    if (address == desc_.canhpmob_address) return canhpmob_;
    if (address == desc_.canpage_address) return canpage_;
    
    // Paged access
    u8 current_mob_idx = (canpage_ >> 4U);
    if (current_mob_idx < mobs_.size()) {
        auto& mob = mobs_[current_mob_idx];
        if (address == desc_.canstmob_address) return mob.canstmob;
        if (address == desc_.cancdmob_address) return mob.cancdmob;
        if (address >= desc_.canidt_address && address < desc_.canidt_address + 4U) {
            return mob.canidtags[address - desc_.canidt_address];
        }
        if (address >= desc_.canidm_address && address < desc_.canidm_address + 4U) {
            return mob.canidmasks[address - desc_.canidm_address];
        }
        if (address == desc_.canstm_address) {
            timer_temp_ = static_cast<u8>(mob.canstm >> 8U);
            return static_cast<u8>(mob.canstm & 0xFFU);
        }
        if (address == desc_.canstm_address + 1) return timer_temp_;

        if (address == desc_.canmsg_address) {
            u8 data_idx = (canpage_ & 0x07U);
            u8 val = mob.data[data_idx];
            // Auto-increment INDX bit if AINC is 0 (AINC=0 means auto-increment!)
            if (!(canpage_ & 0x08U)) {
                canpage_ = (canpage_ & 0xF8U) | ((data_idx + 1U) & 0x07U);
            }
            return val;
        }
    }
    
    return 0U;
}

void CanBus::write(u16 address, u8 value) noexcept {
    if (address == desc_.cangcon_address) {
        cangcon_ = value;
        if (value & 0x01U) reset(); // SWRES
    } else if (address == desc_.cangie_address) {
        cangie_ = value;
        evaluate_interrupts();
    } else if (address == desc_.canie1_address) {
        canie1_ = value;
        evaluate_interrupts();
    } else if (address == desc_.canie2_address) {
        canie2_ = value;
        evaluate_interrupts();
    } else if (address == desc_.canbt1_address) {
        canbt1_ = value;
    } else if (address == desc_.canbt2_address) {
        canbt2_ = value;
    } else if (address == desc_.canbt3_address) {
        canbt3_ = value;
    } else if (address == desc_.canpage_address) {
        canpage_ = value;
    } else if (address == desc_.cangit_address) {
        // CANGIT bits are cleared by writing 1
        cangit_ &= ~value;
        evaluate_interrupts();
    } else if (address == desc_.cantcon_address) {
        cantcon_ = value;
    }
    
    // Paged access
    u8 current_mob_idx = (canpage_ >> 4U);
    if (current_mob_idx < mobs_.size()) {
        auto& mob = mobs_[current_mob_idx];
        if (address == desc_.canstmob_address) {
            // CANSTMOB bits are cleared by writing 0! (Standard CAN protocol)
            mob.canstmob &= value;
            evaluate_interrupts();
        }
        else if (address == desc_.cancdmob_address) {
            mob.cancdmob = value;
            // Update CANEN
            if (current_mob_idx < 8) canen2_ |= (1 << current_mob_idx);
            else canen1_ |= (1 << (current_mob_idx - 8));

            // Trigger transmission simulation if CONMOB=01
            if ((value >> 6U) == 0x01U) {
                // TBD: Add to Tx queue or process immediately
                mob.canstmob |= 0x40U; // TXOK
                cangit_ |= 0x10U; // CANIT bit for TXOK
                evaluate_interrupts();
            }
        }
        else if (address >= desc_.canidt_address && address < desc_.canidt_address + 4U) {
            mob.canidtags[address - desc_.canidt_address] = value;
        }
        else if (address >= desc_.canidm_address && address < desc_.canidm_address + 4U) {
            mob.canidmasks[address - desc_.canidm_address] = value;
        }
        else if (address == desc_.canmsg_address) {
            u8 data_idx = (canpage_ & 0x07U);
            mob.data[data_idx] = value;
            if (!(canpage_ & 0x08U)) {
                canpage_ = (canpage_ & 0xF8U) | ((data_idx + 1U) & 0x07U);
            }
        }
    }
}

void CanBus::evaluate_interrupts() noexcept {
    bool it_pending = false;
    cansit1_ = 0;
    cansit2_ = 0;

    for (size_t i = 0; i < mobs_.size(); ++i) {
        if (mobs_[i].canstmob & 0xFEU) { // Any bit except DLCW?
            // This MOb has a pending status interrupt.
            // Check if MOb interrupt is enabled
            bool enabled = false;
            if (i < 8) {
                cansit2_ |= (1 << i);
                if (canie2_ & (1 << i)) enabled = true;
            } else {
                cansit1_ |= (1 << (i - 8));
                if (canie1_ & (1 << (i - 8))) enabled = true;
            }
            if (enabled) it_pending = true;
        }
    }

    // General interrupts
    if (cangit_ & cangie_) it_pending = true;

    canit_pending_ = it_pending && (cangie_ & 0x01U); // ENIT
}

bool CanBus::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (canit_pending_) {
        request.vector_index = desc_.canit_vector_index;
        return true;
    }
    return false;
}

bool CanBus::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (pending_interrupt_request(request)) return true;
    return false;
}

void CanBus::receive_message(const CanMessage& msg) noexcept {
    // 1. Check if CAN is enabled
    if (!(cangcon_ & 0x02U)) return; // ENA bit 1

    // 2. Iterate through MObs to find a match (Priority: lowest index first)
    for (size_t i = 0; i < mobs_.size(); ++i) {
        auto& mob = mobs_[i];
        u8 conmob = (mob.cancdmob >> 6U);
        
        // Mode must be Receiver (10) or Frame Listen (11)
        if (conmob != 0x02U && conmob != 0x03U) continue;

        // Check IDE (Identifier Extension) consistency
        bool mob_ide = (mob.cancdmob & 0x10U); // bit 4 is IDE
        if (mob_ide != msg.ide) continue;

        // ID Comparison
        bool match = true;
        if (msg.ide) {
            // Extended ID (29 bits)
            // Tags: IDT1(MSB), IDT2, IDT3, IDT4(LSB)
            // IDT1: ID28:21 (Index 3), IDT2: ID20:13 (Index 2), IDT3: ID12:5 (Index 1), IDT4: ID4:0 + RTRTAG... (Index 0)
            u32 mob_id = (static_cast<u32>(mob.canidtags[3]) << 21U) |
                         (static_cast<u32>(mob.canidtags[2]) << 13U) |
                         (static_cast<u32>(mob.canidtags[1]) << 5U) |
                         (static_cast<u32>(mob.canidtags[0] >> 3U));
            
            u32 mob_mask = (static_cast<u32>(mob.canidmasks[3]) << 21U) |
                           (static_cast<u32>(mob.canidmasks[2]) << 13U) |
                           (static_cast<u32>(mob.canidmasks[1]) << 5U) |
                           (static_cast<u32>(mob.canidmasks[0] >> 3U));
            
            if ((msg.id & mob_mask) != (mob_id & mob_mask)) match = false;
        } else {
            // Standard ID (11 bits)
            // Tags: IDT1(ID10:3) (Index 3), IDT2(ID2:0 + RTRTAG + IDE + RB0TAG) (Index 2)
            u32 mob_id = (static_cast<u32>(mob.canidtags[3]) << 3U) |
                         (static_cast<u32>(mob.canidtags[2] >> 5U));
            
            u32 mob_mask = (static_cast<u32>(mob.canidmasks[3]) << 3U) |
                           (static_cast<u32>(mob.canidmasks[2] >> 5U));
            
            if ((msg.id & mob_mask) != (mob_id & mob_mask)) match = false;
        }

        if (match) {
            // MOb Match found!
            // Update Data
            size_t dlc = (mob.cancdmob & 0x0FU);
            for (size_t d = 0; d < std::min<size_t>(msg.data.size(), dlc); ++d) {
                mob.data[d] = msg.data[d];
            }
            
            // Update Status
            mob.canstmob |= 0x20U; // RXOK
            if (msg.data.size() != dlc) mob.canstmob |= 0x80U; // DLCW

            cangit_ |= 0x20U; // RXOK bit in CANGIT
            
            // Disable MOb after reception unless in Listen mode? 
            // Actually, in AT90CAN, CONMOB is cleared after RXOK.
            mob.cancdmob &= 0x3FU; 
            
            evaluate_interrupts();
            return;
        }
    }
}

void CanBus::inject_message(const CanMessage& msg) noexcept {
    receive_message(msg);
}

} // namespace vioavr::core
