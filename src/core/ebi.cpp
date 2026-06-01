#include "vioavr/core/ebi.hpp"
#include "vioavr/core/memory_bus.hpp"

namespace vioavr::core {

static constexpr u32 kExternalMemorySize = 65536;

u16 Ebi::cs_ctrla_address(const EbiDescriptor& desc, u8 cs_index) noexcept {
    switch (cs_index) {
        case 0: return desc.cs0_ctrla_address;
        case 1: return desc.cs1_ctrla_address;
        case 2: return desc.cs2_ctrla_address;
        case 3: return desc.cs3_ctrla_address;
        default: return 0;
    }
}

u16 Ebi::cs_ctrlb_address(const EbiDescriptor& desc, u8 cs_index) noexcept {
    switch (cs_index) {
        case 0: return desc.cs0_ctrlb_address;
        case 1: return desc.cs1_ctrlb_address;
        case 2: return desc.cs2_ctrlb_address;
        case 3: return desc.cs3_ctrlb_address;
        default: return 0;
    }
}

u16 Ebi::cs_baseaddr_address(const EbiDescriptor& desc, u8 cs_index) noexcept {
    switch (cs_index) {
        case 0: return desc.cs0_baseaddr_address;
        case 1: return desc.cs1_baseaddr_address;
        case 2: return desc.cs2_baseaddr_address;
        case 3: return desc.cs3_baseaddr_address;
        default: return 0;
    }
}

Ebi::Ebi(std::string_view name, const EbiDescriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    memory_.resize(kExternalMemorySize, 0U);
    update_ranges();
}

void Ebi::update_ranges() {
    num_ranges_ = 0;
    if (desc_.ctrl_address && desc_.sdramctrlc_address) {
        ranges_[num_ranges_++] = {desc_.ctrl_address, desc_.sdramctrlc_address};
    } else if (desc_.ctrl_address) {
        u16 end = desc_.ctrl_address;
        if (desc_.sdramctrla_address && desc_.sdramctrla_address > end) end = desc_.sdramctrla_address;
        if (desc_.refresh_address && desc_.refresh_address + 1 > end) end = desc_.refresh_address + 1;
        if (desc_.initdly_address && desc_.initdly_address + 1 > end) end = desc_.initdly_address + 1;
        if (desc_.sdramctrlb_address && desc_.sdramctrlb_address > end) end = desc_.sdramctrlb_address;
        ranges_[num_ranges_++] = {desc_.ctrl_address, end};
    }
    if (desc_.sdramctrlb_address && !desc_.sdramctrlc_address) {
        ranges_[num_ranges_++] = {desc_.sdramctrlb_address, desc_.sdramctrlb_address};
    }
    // All CS registers in one contiguous range (CS0..CS3: base+0x10 to base+0x1F)
    u16 cs_start_addr = 0, cs_end_addr = 0;
    for (u8 i = 0; i < 4; ++i) {
        u16 addr = cs_ctrla_address(desc_, i);
        if (addr) {
            if (cs_start_addr == 0 || addr < cs_start_addr) cs_start_addr = addr;
            u16 baseaddr = cs_baseaddr_address(desc_, i);
            if (baseaddr) {
                if (baseaddr + 1 > cs_end_addr) cs_end_addr = baseaddr + 1;
            } else {
                if (addr + 3 > cs_end_addr) cs_end_addr = addr + 3;
            }
        }
    }
    if (cs_start_addr && cs_end_addr) {
        ranges_[num_ranges_++] = {cs_start_addr, cs_end_addr};
    }
}

std::span<const AddressRange> Ebi::mapped_ranges() const noexcept {
    return {ranges_.data(), num_ranges_};
}

void Ebi::update_cs_range(u8 cs_index) noexcept {
    if (cs_index >= 4) return;
    auto& cs = cs_[cs_index];
    if (!(ctrl_ & 0x01) || cs.ctrla_ == 0) {
        cs.enabled_ = false;
        return;
    }
    u32 base = static_cast<u32>(cs.baseaddr_);
    if (base >= kExternalMemorySize) {
        cs.enabled_ = false;
        return;
    }
    u32 size;
    u8 mode = cs.ctrla_ & 0x07U;
    switch (mode) {
        case 0: size = 0x10000; break;
        case 1: size = 0x20000; break;
        default: size = 0x10000; break;
    }
    cs.start_ = static_cast<u16>(base);
    u32 end32 = base + size - 1;
    cs.end_ = (end32 > 0xFFFFU) ? 0xFFFFU : static_cast<u16>(end32);
    cs.enabled_ = true;
}

void Ebi::reset() noexcept {
    ctrl_ = 0;
    sdramctrla_ = 0;
    refresh_ = 0;
    initdly_ = 0;
    sdramctrlb_ = 0;
    sdramctrlc_ = 0;
    for (auto& cs : cs_) {
        cs = CsState{};
    }
    init_counter_ = 0;
    refresh_counter_ = 0;
    init_complete_ = false;
    sdram_enabled_ = false;
    std::fill(memory_.begin(), memory_.end(), 0);
}

bool Ebi::is_sdram_enabled() const noexcept {
    return (ctrl_ & 0x02) != 0;
}

void Ebi::tick(u64 elapsed_cycles) noexcept {
    if (!is_sdram_enabled()) return;
    for (u64 i = 0; i < elapsed_cycles; ++i) {
        if (!init_complete_) {
            if (init_counter_ > 0) {
                --init_counter_;
                if (init_counter_ == 0) {
                    init_complete_ = true;
                    refresh_counter_ = refresh_;
                }
            }
        } else {
            if (refresh_counter_ > 0) {
                --refresh_counter_;
            } else {
                refresh_counter_ = refresh_;
            }
        }
    }
    sdram_enabled_ = is_sdram_enabled();
}

u8 Ebi::read(u16 address) noexcept {
    if (address == desc_.ctrl_address)              return ctrl_;
    if (address == desc_.sdramctrla_address)        return sdramctrla_;
    if (address == desc_.refresh_address)           return static_cast<u8>(refresh_);
    if (address == desc_.refresh_address + 1)       return static_cast<u8>(refresh_ >> 8);
    if (address == desc_.initdly_address)           return static_cast<u8>(initdly_);
    if (address == desc_.initdly_address + 1)       return static_cast<u8>(initdly_ >> 8);
    if (address == desc_.sdramctrlb_address)        return sdramctrlb_;
    if (address == desc_.sdramctrlc_address)        return sdramctrlc_;
    for (u8 i = 0; i < 4; ++i) {
        u16 ca = cs_ctrla_address(desc_, i);
        if (!ca) continue;
        u16 ba = cs_baseaddr_address(desc_, i);
        if (address < ca || address > ba + 1) continue;
        if (address == ca)          return cs_[i].ctrla_;
        if (address == cs_ctrlb_address(desc_, i)) return cs_[i].ctrlb_;
        if (address == ba)          return static_cast<u8>(cs_[i].baseaddr_);
        if (address == ba + 1)      return static_cast<u8>(cs_[i].baseaddr_ >> 8);
    }
    return 0;
}

void Ebi::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrl_address) {
        bool was_enabled = is_sdram_enabled();
        ctrl_ = value;
        bool now_enabled = is_sdram_enabled();
        if (!was_enabled && now_enabled) {
            init_counter_ = initdly_ ? initdly_ : 1;
            init_complete_ = false;
            refresh_counter_ = 0;
        }
        sdram_enabled_ = now_enabled;
        for (u8 i = 0; i < 4; ++i) update_cs_range(i);
    }
    else if (address == desc_.sdramctrla_address) {
        sdramctrla_ = value;
    }
    else if (address == desc_.refresh_address) {
        refresh_ = (refresh_ & 0xFF00) | value;
    }
    else if (address == desc_.refresh_address + 1) {
        refresh_ = (refresh_ & 0x00FF) | (static_cast<u16>(value) << 8);
    }
    else if (address == desc_.initdly_address) {
        initdly_ = (initdly_ & 0xFF00) | value;
    }
    else if (address == desc_.initdly_address + 1) {
        initdly_ = (initdly_ & 0x00FF) | (static_cast<u16>(value) << 8);
    }
    else if (address == desc_.sdramctrlb_address) {
        sdramctrlb_ = value;
    }
    else if (address == desc_.sdramctrlc_address) {
        sdramctrlc_ = value;
    }
    else {
        for (u8 i = 0; i < 4; ++i) {
            u16 ca = cs_ctrla_address(desc_, i);
            if (!ca) continue;
            u16 ba = cs_baseaddr_address(desc_, i);
            if (address < ca || address > ba + 1) continue;
            auto& cs = cs_[i];
            if (address == ca) {
                cs.ctrla_ = value;
                update_cs_range(i);
            } else if (address == cs_ctrlb_address(desc_, i)) {
                cs.ctrlb_ = value;
            } else if (address == ba) {
                cs.baseaddr_ = (cs.baseaddr_ & 0xFF00) | value;
                update_cs_range(i);
            } else if (address == ba + 1) {
                cs.baseaddr_ = (cs.baseaddr_ & 0x00FF) | (static_cast<u16>(value) << 8);
                update_cs_range(i);
            }
            return;
        }
    }
}

u8 Ebi::get_wait_states() const noexcept {
    u8 max_ws = 0;
    for (const auto& cs : cs_) {
        if (cs.enabled_) {
            u8 ws = cs.ctrlb_ & 0x0FU;
            if (ws > max_ws) max_ws = ws;
        }
    }
    return max_ws;
}

u8 Ebi::external_read(u16 address) noexcept {
    for (const auto& cs : cs_) {
        if (cs.enabled_ && address >= cs.start_ && address <= cs.end_) {
            std::size_t offset = static_cast<std::size_t>(address) - static_cast<std::size_t>(cs.start_);
            if (offset < memory_.size()) {
                if (bus_) bus_->request_cpu_stall(cs.ctrlb_ & 0x0FU);
                return memory_[offset];
            }
        }
    }
    return 0xFF;
}

void Ebi::external_write(u16 address, u8 value) noexcept {
    for (const auto& cs : cs_) {
        if (cs.enabled_ && address >= cs.start_ && address <= cs.end_) {
            std::size_t offset = static_cast<std::size_t>(address) - static_cast<std::size_t>(cs.start_);
            if (offset < memory_.size()) {
                if (bus_) bus_->request_cpu_stall(cs.ctrlb_ & 0x0FU);
                memory_[offset] = value;
            }
            return;
        }
    }
}

bool Ebi::is_cs_active(u8 cs_index) const noexcept {
    if (cs_index >= 4) return false;
    return cs_[cs_index].enabled_;
}

u16 Ebi::cs_start(u8 cs_index) const noexcept {
    if (cs_index >= 4) return 0;
    return cs_[cs_index].start_;
}

u16 Ebi::cs_end(u8 cs_index) const noexcept {
    if (cs_index >= 4) return 0;
    return cs_[cs_index].end_;
}

} // namespace vioavr::core
