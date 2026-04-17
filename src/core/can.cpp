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
    if (cangcon_ & 0x01) { // ENA (actually ENA bit in CANCON?)
        // In AT90CAN, bit 1 is ENA, but bit 0 is SWRES?
        // Let's check GCON bits: bit 7: ABRQ, bit 4: OVRQ, bit 3: TTC, bit 2: SYNTTC, bit 1: LISTEN, bit 0: ENA?
        // No! bit 0 is SWRES. Bit 1: ENA.
        
        // Wait! Let's check GCON bits in JSON/Datasheet.
        // GCON bits: [7] ABRQ, [6] RESERVED, [5] RESERVED, [4] OVRQ, [3] TTC, [2] SYNTTC, [1] LISTEN, [0] SWRES
        // CON bit 1: ENA? No, ENA is bit 0 of CANGCON in some AVRs.
        // Actually for AT90CAN, bit 1 is ENA, bit 0 is SWRES.
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
    if (address == desc_.cantim_address) return static_cast<u8>(cantim_ & 0xFF);
    if (address == desc_.cantim_address + 1) return static_cast<u8>(cantim_ >> 8);
    if (address == desc_.canttc_address) return static_cast<u8>(canttc_ & 0xFF);
    if (address == desc_.canttc_address + 1) return static_cast<u8>(canttc_ >> 8);
    if (address == desc_.cantec_address) return cantec_;
    if (address == desc_.canrec_address) return canrec_;
    if (address == desc_.canhpmob_address) return canhpmob_;
    if (address == desc_.canpage_address) return canpage_;
    
    // Paged access
    u8 current_mob_idx = (canpage_ >> 4);
    if (current_mob_idx < mobs_.size()) {
        auto& mob = mobs_[current_mob_idx];
        if (address == desc_.canstmob_address) return mob.canstmob;
        if (address == desc_.cancdmob_address) return mob.cancdmob;
        if (address >= desc_.canidt_address && address < desc_.canidt_address + 4) {
            return mob.canidtags[address - desc_.canidt_address];
        }
        if (address >= desc_.canidm_address && address < desc_.canidm_address + 4) {
            return mob.canidmasks[address - desc_.canidm_address];
        }
        if (address == desc_.canmsg_address) {
            u8 data_idx = (canpage_ & 0x07);
            u8 val = mob.data[data_idx];
            // Auto-increment data index in CANPAGE
            canpage_ = (canpage_ & 0xF8) | ((data_idx + 1) & 0x07);
            return val;
        }
    }
    
    return 0;
}

void CanBus::write(u16 address, u8 value) noexcept {
    if (address == desc_.cangcon_address) {
        cangcon_ = value;
        if (value & 0x01) reset(); // SWRES
    } else if (address == desc_.cangie_address) {
        cangie_ = value;
    } else if (address == desc_.canbt1_address) {
        canbt1_ = value;
    } else if (address == desc_.canbt2_address) {
        canbt2_ = value;
    } else if (address == desc_.canbt3_address) {
        canbt3_ = value;
    } else if (address == desc_.canpage_address) {
        canpage_ = value;
    }
    
    // Paged access
    u8 current_mob_idx = (canpage_ >> 4);
    if (current_mob_idx < mobs_.size()) {
        auto& mob = mobs_[current_mob_idx];
        if (address == desc_.canstmob_address) mob.canstmob = value;
        else if (address == desc_.cancdmob_address) {
            mob.cancdmob = value;
            // Trigger transmission or reception based on CONMOB (bits 7:6)
            u8 conmob = (value >> 6);
            if (conmob == 0x01) { // Transmission
                // Mock success for now
                mob.canstmob |= 0x40; // TXOK
                canit_pending_ = true;
                evaluate_interrupts();
            }
        }
        else if (address >= desc_.canidt_address && address < desc_.canidt_address + 4) {
            mob.canidtags[address - desc_.canidt_address] = value;
        }
        else if (address >= desc_.canidm_address && address < desc_.canidm_address + 4) {
            mob.canidmasks[address - desc_.canidm_address] = value;
        }
        else if (address == desc_.canmsg_address) {
            u8 data_idx = (canpage_ & 0x07);
            mob.data[data_idx] = value;
            canpage_ = (canpage_ & 0xF8) | ((data_idx + 1) & 0x07);
        }
    }
}

void CanBus::evaluate_interrupts() noexcept {
    // Simplified interrupt evaluation
    // Check if any MOB has interrupt pending and CANIE is set
}

bool CanBus::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (canit_pending_) {
        request.vector_index = desc_.canit_vector_index;
        return true;
    }
    return false;
}

bool CanBus::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (canit_pending_ && request.vector_index == desc_.canit_vector_index) {
        // Clear pending? No, usually handled by reading registers or clearing bits
        return true;
    }
    return false;
}

void CanBus::inject_message(const CanMessage& msg) noexcept {
    // Receive simulation
    for (auto& mob : mobs_) {
        u8 conmob = (mob.cancdmob >> 6);
        if (conmob == 0x02) { // Reception
            // Compare ID with tag/mask
            // For now, simple match
            for (size_t i = 0; i < std::min<size_t>(msg.data.size(), 8U); ++i) {
                mob.data[i] = msg.data[i];
            }
            mob.canstmob |= 0x20; // RXOK
            canit_pending_ = true;
            evaluate_interrupts();
            return;
        }
    }
}

} // namespace vioavr::core
