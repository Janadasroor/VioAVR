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
    int_pending_ = false;
    for (auto& c : channels_) {
        c = Channel{};
    }
}

void Dma::tick(u64 elapsed_cycles) noexcept {
    if (!is_enabled()) return;

    for (u8 i = 0; i < desc_.channel_count; ++i) {
        auto& c = channels_[i];
        if (c.pending_trigger && !c.busy && c.remaining_count > 0) {
            start_transfer(i);
        }
        if (c.busy) {
            c.xfer.cycle_accumulator += elapsed_cycles;
            process_transfer_beats(i);
        }
    }
}

void Dma::start_transfer(u8 channel_idx) noexcept {
    auto& c = channels_[channel_idx];
    c.busy = true;
    c.pending_trigger = false;
    c.xfer.trigact = c.ctrlb & 0x03U;
    u8 burstlen = ((c.ctrla >> 2) & 0x03U);
    c.xfer.block_size = static_cast<u8>(1U << burstlen);
    c.xfer.started = true;
    c.xfer.beats_in_burst = 0;

    switch (c.xfer.trigact) {
        case 0: c.xfer.beats_remaining = static_cast<u16>(c.remaining_count); break;
        case 1: c.xfer.beats_remaining = static_cast<u16>(c.remaining_count); break;
        case 2: c.xfer.beats_remaining = c.xfer.block_size; break;
        case 3: c.xfer.beats_remaining = c.xfer.block_size; break;
        default: c.xfer.beats_remaining = 1; break;
    }
}

void Dma::process_transfer_beats(u8 channel_idx) noexcept {
    auto& c = channels_[channel_idx];

    while (c.xfer.beats_remaining > 0 && c.remaining_count > 0) {
        u8 ws_src = bus_->get_wait_states(c.srcaddr);
        u8 ws_dst = bus_->get_wait_states(c.dstaddr);
        u16 beat_cost = static_cast<u16>(1 + ws_src + 1 + ws_dst);

        if (c.xfer.cycle_accumulator < beat_cost) break;

        u8 data = bus_->read_data(c.srcaddr);
        bus_->write_data(c.dstaddr, data);

        u8 srcinc = (c.ctrla >> 6) & 0x03U;
        u8 dstinc = (c.ctrla >> 4) & 0x03U;
        if (srcinc == 0x01U) c.srcaddr++;
        else if (srcinc >= 0x02U) c.srcaddr += 2;
        if (dstinc == 0x01U) c.dstaddr++;
        else if (dstinc >= 0x02U) c.dstaddr += 2;

        c.remaining_count--;
        c.xfer.beats_remaining--;
        c.xfer.beats_in_burst++;
        c.xfer.cycle_accumulator -= beat_cost;

        if (c.remaining_count == 0) {
            c.intflags |= 0x01;
            if (c.intctrl & 0x01) int_pending_ = true;
            c.busy = false;
            c.xfer.started = false;
            c.xfer.cycle_accumulator = 0;
            return;
        }
    }

    if (c.xfer.beats_remaining == 0 && c.remaining_count > 0) {
        switch (c.xfer.trigact) {
            case 0:
                c.busy = false;
                c.xfer.started = false;
                c.xfer.cycle_accumulator = 0;
                break;
            case 1:
                c.xfer.beats_remaining = static_cast<u16>(c.remaining_count);
                break;
            case 2:
                c.busy = false;
                c.xfer.started = false;
                c.xfer.cycle_accumulator = 0;
                break;
            case 3:
                c.xfer.beats_remaining = c.xfer.block_size;
                break;
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
    } else if (address == desc_.intctrl_address) {
        intctrl_ = value;
    } else if (address == desc_.intflags_address) {
        intflags_ &= ~value;
        if (!(intflags_ & 0x01)) int_pending_ = false;
    } else if (address == desc_.dbgctrl_address) {
        dbgctrl_ = value;
    } else {
        for (u8 i = 0; i < desc_.channel_count; ++i) {
            const auto& cd = desc_.channels[i];
            auto& c = channels_[i];
            if (address == cd.ctrla_address) {
                c.ctrla = value;
                if (value & 0x01) {
                    c.remaining_count = c.cnt;
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
                if (!(c.ctrla & 0x01U)) c.remaining_count = c.cnt;
            } else if (address == cd.cnt_address + 1) {
                c.cnt = (c.cnt & 0x00FF) | (static_cast<u16>(value) << 8);
                if (!(c.ctrla & 0x01U)) c.remaining_count = c.cnt;
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

bool Dma::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (int_pending_ && desc_.vector_index != 0xFF) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    for (u8 i = 0; i < desc_.channel_count; ++i) {
        if ((channels_[i].intflags & 0x01) && (channels_[i].intctrl & 0x01)) {
            request.vector_index = desc_.vector_index;
            return true;
        }
    }
    return false;
}

bool Dma::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (int_pending_ && desc_.vector_index != 0xFF) {
        request.vector_index = desc_.vector_index;
        int_pending_ = false;
        intflags_ &= ~0x01;
        return true;
    }
    for (u8 i = 0; i < desc_.channel_count; ++i) {
        if ((channels_[i].intflags & 0x01) && (channels_[i].intctrl & 0x01)) {
            request.vector_index = desc_.vector_index;
            channels_[i].intflags &= ~0x01;
            return true;
        }
    }
    return false;
}

void Dma::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
    if (evsys_) {
        for (u8 i = 0; i < 128; ++i) {
            evsys_->register_generator_callback(i, [this, i](bool level) {
                if (level) this->on_trigger(i);
            });
        }

        u16 user_base = evsys_->users_base();
        if (user_base != 0) {
            for (u8 i = 0; i < desc_.channel_count; ++i) {
                u16 user_addr = desc_.channels[i].user_event_address;
                if (user_addr >= user_base) {
                    u8 user_idx = static_cast<u8>(user_addr - user_base);
                    evsys_->register_user_callback(user_idx, [this, i](bool level) {
                        if (level && is_enabled() && (channels_[i].ctrla & 0x01U)) {
                            channels_[i].pending_trigger = true;
                        }
                    });
                }
            }
        }
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
