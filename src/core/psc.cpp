#include "vioavr/core/psc.hpp"
#include "vioavr/core/adc.hpp"
#include <algorithm>

namespace vioavr::core {

Psc::Psc(std::string_view name, const PscDescriptor& desc)
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs = {
        desc.pctl_address, desc.psoc_address, desc.pconf_address,
        desc.pim_address, desc.pifr_address, desc.picr_address,
        desc.ocrsa_address, static_cast<u16>(desc.ocrsa_address + 1),
        desc.ocrra_address, static_cast<u16>(desc.ocrra_address + 1),
        desc.ocrsb_address, static_cast<u16>(desc.ocrsb_address + 1),
        desc.ocrrb_address, static_cast<u16>(desc.ocrrb_address + 1),
        desc.pfrc0a_address, desc.pfrc0b_address
    };
    
    std::sort(addrs.begin(), addrs.end());
    for (u16 addr : addrs) {
        if (addr == 0) continue;
        if (!ranges_.empty() && addr == ranges_.back().end + 1) {
            ranges_.back().end = addr;
        } else {
            ranges_.push_back({addr, addr});
        }
    }
}

std::span<const AddressRange> Psc::mapped_ranges() const noexcept {
    return {ranges_.data(), ranges_.size()};
}

void Psc::reset() noexcept {
    counter_ = 0;
    pctl_ = 0;
    psoc_ = 0;
    pconf_ = 0;
    pim_ = 0;
    pifr_ = 0;
    ocrsa_ = 0;
    ocrra_ = 0;
    ocrsb_ = 0;
    ocrrb_ = 0;
    temp_ = 0;
}

u8 Psc::read(u16 address) noexcept {
    if (address == desc_.pctl_address) return pctl_;
    if (address == desc_.psoc_address) return psoc_;
    if (address == desc_.pconf_address) return pconf_;
    if (address == desc_.pim_address) return pim_;
    if (address == desc_.pifr_address) return pifr_;
    
    if (address == desc_.ocrsa_address) return ocrsa_ & 0xFF;
    if (address == desc_.ocrsa_address + 1) return (ocrsa_ >> 8) & 0x0F;
    if (address == desc_.ocrra_address) return ocrra_ & 0xFF;
    if (address == desc_.ocrra_address + 1) return (ocrra_ >> 8) & 0x0F;
    if (address == desc_.ocrsb_address) return ocrsb_ & 0xFF;
    if (address == desc_.ocrsb_address + 1) return (ocrsb_ >> 8) & 0x0F;
    if (address == desc_.ocrrb_address) return ocrrb_ & 0xFF;
    if (address == desc_.ocrrb_address + 1) return (ocrrb_ >> 8) & 0x0F;
    
    return 0;
}

void Psc::write(u16 address, u8 value) noexcept {
    if (address == desc_.pctl_address) {
        pctl_ = value;
    } else if (address == desc_.psoc_address) {
        psoc_ = value;
    } else if (address == desc_.pconf_address) {
        pconf_ = value;
    } else if (address == desc_.pim_address) {
        pim_ = value;
    } else if (address == desc_.pifr_address) {
        pifr_ &= ~value;
    } else if (address == desc_.ocrsa_address) {
        temp_ = value;
    } else if (address == desc_.ocrsa_address + 1) {
        ocrsa_ = (static_cast<u16>(value & 0x0F) << 8) | temp_;
    } else if (address == desc_.ocrra_address) {
        temp_ = value;
    } else if (address == desc_.ocrra_address + 1) {
        ocrra_ = (static_cast<u16>(value & 0x0F) << 8) | temp_;
    } else if (address == desc_.ocrsb_address) {
        temp_ = value;
    } else if (address == desc_.ocrsb_address + 1) {
        ocrsb_ = (static_cast<u16>(value & 0x0F) << 8) | temp_;
    } else if (address == desc_.ocrrb_address) {
        temp_ = value;
    } else if (address == desc_.ocrrb_address + 1) {
        ocrrb_ = (static_cast<u16>(value & 0x0F) << 8) | temp_;
    }
}

void Psc::tick(u64 elapsed_cycles) noexcept {
    if (!(pctl_ & 0x80)) return; // PRUN bit

    for (u64 i = 0; i < elapsed_cycles; ++i) {
        counter_++;
        // OCRRA defines the end of cycle in most modes
        if (counter_ >= ocrra_ && ocrra_ > 0) {
            counter_ = 0;
            pifr_ |= 0x01; // PEV: PSC End Cycle Event flag
            
            if (adc_trigger_) {
                AdcAutoTriggerSource src = AdcAutoTriggerSource::psc0_sync;
                if (desc_.psc_index == 1) src = AdcAutoTriggerSource::psc1_sync;
                else if (desc_.psc_index == 2) src = AdcAutoTriggerSource::psc2_sync;
                adc_trigger_->notify_auto_trigger(src);
            }
        }
    }
}

bool Psc::pending_interrupt_request(InterruptRequest& req) const noexcept {
    if ((pifr_ & 0x01) && (pim_ & 0x01)) {
        req.vector_index = desc_.ec_vector_index;
        return true;
    }
    return false;
}

bool Psc::consume_interrupt_request(InterruptRequest& req) noexcept {
    if (pending_interrupt_request(req)) {
        pifr_ &= ~0x01; // Clear EC flag
        return true;
    }
    return false;
}

} // namespace vioavr::core
