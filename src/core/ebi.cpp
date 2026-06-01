#include "vioavr/core/ebi.hpp"
#include "vioavr/core/memory_bus.hpp"

namespace vioavr::core {

static constexpr u32 kExternalMemorySize = 65536;

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
    if (desc_.cs0_ctrla_address && desc_.cs0_baseaddr_address) {
        ranges_[num_ranges_++] = {desc_.cs0_ctrla_address, static_cast<u16>(desc_.cs0_baseaddr_address + 1)};
    }
}

std::span<const AddressRange> Ebi::mapped_ranges() const noexcept {
    return {ranges_.data(), num_ranges_};
}

void Ebi::update_cs0_range() noexcept {
    // CS0_BASEADDR: bits 7:0 are the base address in 64KB-aligned units
    // For SRAM: base = cs0_baseaddr_ << 8 (each unit is 256 bytes on some devices)
    // For XMEGA: CS0_BASEADDR maps the external memory to the upper half of data space
    //
    // Simplified model: external memory appears at (cs0_baseaddr_ << 8)
    // Size: computed from CS0_CTRLA mode bits (default 64KB)
    if (!(ctrl_ & 0x01)) {
        cs0_enabled_ = false;
        return;
    }

    u32 base = static_cast<u32>(cs0_baseaddr_);
    if (base >= kExternalMemorySize) {
        cs0_enabled_ = false;
        return;
    }

    u32 size;
    // CS0_CTRLA bits 2:0 = memory size (0=64KB, 1=128KB...)
    // For simplicity: default 64KB, scale by mode
    u8 mode = cs0_ctrla_ & 0x07U;
    switch (mode) {
        case 0: size = 0x10000; break; // 64KB
        case 1: size = 0x20000; break; // 128KB
        default: size = 0x10000; break;
    }

    cs0_start_ = static_cast<u16>(base);
    u32 end32 = base + size - 1;
    cs0_end_ = (end32 > 0xFFFFU) ? 0xFFFFU : static_cast<u16>(end32);
    cs0_enabled_ = true;
}

void Ebi::reset() noexcept {
    ctrl_ = 0;
    sdramctrla_ = 0;
    refresh_ = 0;
    initdly_ = 0;
    sdramctrlb_ = 0;
    sdramctrlc_ = 0;
    cs0_ctrla_ = 0;
    cs0_ctrlb_ = 0;
    cs0_baseaddr_ = 0;
    init_counter_ = 0;
    refresh_counter_ = 0;
    init_complete_ = false;
    sdram_enabled_ = false;
    cs0_start_ = 0;
    cs0_end_ = 0;
    cs0_enabled_ = false;
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
    if (address == desc_.cs0_ctrla_address)         return cs0_ctrla_;
    if (address == desc_.cs0_ctrlb_address)         return cs0_ctrlb_;
    if (address == desc_.cs0_baseaddr_address)      return static_cast<u8>(cs0_baseaddr_);
    if (address == desc_.cs0_baseaddr_address + 1)  return static_cast<u8>(cs0_baseaddr_ >> 8);
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
        update_cs0_range();
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
    else if (address == desc_.cs0_ctrla_address) {
        cs0_ctrla_ = value;
        update_cs0_range();
    }
    else if (address == desc_.cs0_ctrlb_address) {
        cs0_ctrlb_ = value;
    }
    else if (address == desc_.cs0_baseaddr_address) {
        cs0_baseaddr_ = (cs0_baseaddr_ & 0xFF00) | value;
        update_cs0_range();
    }
    else if (address == desc_.cs0_baseaddr_address + 1) {
        cs0_baseaddr_ = (cs0_baseaddr_ & 0x00FF) | (static_cast<u16>(value) << 8);
        update_cs0_range();
    }
}

u8 Ebi::get_wait_states() const noexcept {
    if (!cs0_enabled_) return 0;
    // CS0_CTRLB bits 3:0 = wait states (0-15 cycles)
    return cs0_ctrlb_ & 0x0FU;
}

u8 Ebi::external_read(u16 address) noexcept {
    if (!cs0_enabled_) return 0xFF;
    if (address < cs0_start_ || address > cs0_end_) return 0xFF;
    std::size_t offset = static_cast<std::size_t>(address) - static_cast<std::size_t>(cs0_start_);
    if (offset >= memory_.size()) return 0xFF;
    if (bus_) bus_->request_cpu_stall(get_wait_states());
    return memory_[offset];
}

void Ebi::external_write(u16 address, u8 value) noexcept {
    if (!cs0_enabled_) return;
    if (address < cs0_start_ || address > cs0_end_) return;
    std::size_t offset = static_cast<std::size_t>(address) - static_cast<std::size_t>(cs0_start_);
    if (offset >= memory_.size()) return;
    if (bus_) bus_->request_cpu_stall(get_wait_states());
    memory_[offset] = value;
}

} // namespace vioavr::core
