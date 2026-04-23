#include "vioavr/core/dma.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/logger.hpp"
#include <algorithm>
#include <vector>

namespace vioavr::core {

Dma::Dma(const DmaDescriptor& desc) : desc_(desc) {
    std::vector<u16> addrs;
    auto add_if_valid = [&](u16 addr) {
        if (addr != 0) addrs.push_back(addr);
    };

    add_if_valid(desc_.ctrla_address);
    add_if_valid(desc_.status_address);
    add_if_valid(desc_.intctrl_address);
    add_if_valid(desc_.intflags_address);
    add_if_valid(desc_.dbgctrl_address);

    for (u8 i = 0; i < desc_.channel_count; ++i) {
        const auto& c = desc_.channels[i];
        add_if_valid(c.ctrla_address);
        add_if_valid(c.ctrlb_address);
        add_if_valid(c.srcaddr_address);
        add_if_valid(c.srcaddr_address + 1);
        add_if_valid(c.dstaddr_address);
        add_if_valid(c.dstaddr_address + 1);
        add_if_valid(c.cnt_address);
        add_if_valid(c.cnt_address + 1);
        add_if_valid(c.trigsrc_address);
        add_if_valid(c.intctrl_address);
        add_if_valid(c.intflags_address);
    }

    std::sort(addrs.begin(), addrs.end());
    addrs.erase(std::unique(addrs.begin(), addrs.end()), addrs.end());

    size_t count = 0;
    if (!addrs.empty()) {
        u16 start = addrs[0];
        u16 end = addrs[0];
        for (size_t i = 1; i < addrs.size(); ++i) {
            if (addrs[i] == end + 1) {
                end = addrs[i];
            } else {
                ranges_[count++] = {start, end};
                start = addrs[i];
                end = addrs[i];
                if (count >= ranges_.size()) break;
            }
        }
        if (count < ranges_.size()) {
            ranges_[count++] = {start, end};
        }
    }
}

void Dma::reset() noexcept {
    ctrla_ = 0;
    status_ = 0;
    intctrl_ = 0;
    intflags_ = 0;
    dbgctrl_ = 0;
    for (auto& c : channels_) {
        c = Channel{};
    }
}

void Dma::tick(u64 elapsed_cycles) noexcept {
    (void)elapsed_cycles;
    if (!is_enabled()) return;

    for (u8 i = 0; i < desc_.channel_count; ++i) {
        if (channels_[i].pending_trigger && !channels_[i].busy) {
            perform_transfer(i);
        }
    }
}

std::span<const AddressRange> Dma::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

u8 Dma::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.intctrl_address) return intctrl_;
    if (address == desc_.intflags_address) return intflags_;
    if (address == desc_.dbgctrl_address) return dbgctrl_;

    for (u8 i = 0; i < desc_.channel_count; ++i) {
        const auto& cd = desc_.channels[i];
        auto& c = channels_[i];
        if (address == cd.ctrla_address) return c.ctrla;
        if (address == cd.ctrlb_address) return c.ctrlb;
        if (address == cd.srcaddr_address) return c.srcaddr & 0xFF;
        if (address == cd.srcaddr_address + 1) return (c.srcaddr >> 8) & 0xFF;
        if (address == cd.dstaddr_address) return c.dstaddr & 0xFF;
        if (address == cd.dstaddr_address + 1) return (c.dstaddr >> 8) & 0xFF;
        if (address == cd.cnt_address) return c.remaining_count & 0xFF;
        if (address == cd.cnt_address + 1) return (c.remaining_count >> 8) & 0xFF;
        if (address == cd.trigsrc_address) return c.trigsrc;
        if (address == cd.intctrl_address) return c.intctrl;
        if (address == cd.intflags_address) return c.intflags;
    }

    return 0;
}

void Dma::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
    } else if (address == desc_.status_address) {
        // Status might be read-only or clear-on-write
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.intflags_address) {
        intflags_ &= ~value;
    } else if (address == desc_.dbgctrl_address) {
        dbgctrl_ = value;
    } else {
        for (u8 i = 0; i < desc_.channel_count; ++i) {
            const auto& cd = desc_.channels[i];
            auto& c = channels_[i];
            if (address == cd.ctrla_address) {
                c.ctrla = value;
                if (value & 0x01) { // ENABLE
                    c.remaining_count = c.cnt;
                    Logger::debug("DMA Channel " + std::to_string(i) + " enabled, count=" + std::to_string(c.cnt));
                }
            } else if (address == cd.ctrlb_address) {
                c.ctrlb = value;
            } else if (address == cd.srcaddr_address) {
                c.srcaddr = (c.srcaddr & 0xFF00) | value;
            } else if (address == cd.srcaddr_address + 1) {
                c.srcaddr = (c.srcaddr & 0x00FF) | (static_cast<u16>(value) << 8);
            } else if (address == cd.dstaddr_address) {
                c.dstaddr = (c.dstaddr & 0xFF00) | value;
            } else if (address == cd.dstaddr_address + 1) {
                c.dstaddr = (c.dstaddr & 0x00FF) | (static_cast<u16>(value) << 8);
            } else if (address == cd.cnt_address) {
                c.cnt = (c.cnt & 0xFF00) | value;
                c.remaining_count = c.cnt;
            } else if (address == cd.cnt_address + 1) {
                c.cnt = (c.cnt & 0x00FF) | (static_cast<u16>(value) << 8);
                c.remaining_count = c.cnt;
            } else if (address == cd.trigsrc_address) {
                c.trigsrc = value;
            } else if (address == cd.intctrl_address) {
                c.intctrl = value;
            } else if (address == cd.intflags_address) {
                c.intflags &= ~value;
            }
        }
    }
}

void Dma::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    if (evsys_) {
        // 1. Direct Generator Listeners (for peripherals that bypass EVSYS channels)
        // We register for a wide range of generators. 
        for (u8 i = 0; i < 128; ++i) {
            evsys_->register_generator_callback(i, [this, i](bool level) {
                if (level) this->on_trigger(i);
            });
        }

        // 2. EVSYS User Slot Listeners (for triggers routed through Channels)
        u16 user_base = evsys_->users_base();
        if (user_base != 0) {
            for (u8 i = 0; i < desc_.channel_count; ++i) {
                u16 user_addr = desc_.channels[i].user_event_address;
                if (user_addr >= user_base) {
                    u8 user_idx = static_cast<u8>(user_addr - user_base);
                    evsys_->register_user_callback(user_idx, [this, i](bool level) {
                        // In this case, the channel 'i' is triggered directly by its EVSYS user slot.
                        // TRIGSRC is ignored if triggered via User slot callback.
                        if (level && is_enabled() && (channels_[i].ctrla & 0x01U)) {
                            channels_[i].pending_trigger = true;
                        }
                    });
                }
            }
        }
    }
}

void Dma::perform_transfer(u8 channel_idx) noexcept {
    auto& c = channels_[channel_idx];
    if (!bus_ || c.remaining_count == 0) {
        c.pending_trigger = false;
        return;
    }

    c.busy = true;
    
    // Transfer logic
    // Mode 0: Burst transfer (move all at once)
    // Mode 1: Single byte per trigger
    bool burst = (c.ctrlb & 0x03) == 0; 

    do {
        u8 data = bus_->read_data(c.srcaddr);
        bus_->write_data(c.dstaddr, data);
        Logger::debug("DMA Transfer: [0x" + Logger::hex(c.srcaddr) + "] -> [0x" + Logger::hex(c.dstaddr) + "] = 0x" + Logger::hex(data));
        
        // Address increment logic
        // CTRLA[3:2] = SRCINC, CTRLA[5:4] = DSTINC
        if ((c.ctrla >> 2) & 0x01) c.srcaddr++; // Heuristic: bit 2 is SRCINC
        if ((c.ctrla >> 4) & 0x01) c.dstaddr++; // Heuristic: bit 4 is DSTINC

        c.remaining_count--;
        
        if (c.remaining_count == 0) {
            c.busy = false;
            c.pending_trigger = false;
            c.intflags |= 0x01; // Transfer Complete flag
            Logger::debug("DMA Channel " + std::to_string(channel_idx) + " transfer complete");
            break;
        }
    } while (burst);

    if (!burst) {
        c.busy = false;
        c.pending_trigger = false;
    }
}

void Dma::on_trigger(u8 trigger_id) noexcept {
    if (!is_enabled()) return;
    
    for (u8 i = 0; i < desc_.channel_count; ++i) {
        if (channels_[i].trigsrc == trigger_id && (channels_[i].ctrla & 0x01)) {
            channels_[i].pending_trigger = true;
        }
    }
}

} // namespace vioavr::core
