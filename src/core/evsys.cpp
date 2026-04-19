#include "vioavr/core/evsys.hpp"
#include <algorithm>

namespace vioavr::core {

EventSystem::EventSystem(const EvsysDescriptor& desc) : desc_(desc) {
    if (desc_.channel_count > 0) {
        channels_.resize(desc_.channel_count, 0);
    }
    if (desc_.user_count > 0) {
        users_.resize(desc_.user_count, 0);
        callbacks_.resize(desc_.user_count, nullptr);
    }

    if (desc_.strobe_address != 0) {
        ranges_[0] = {desc_.strobe_address, desc_.strobe_address};
        if (desc_.channels_address != 0) {
            ranges_[1] = {desc_.channels_address, static_cast<u16>(desc_.channels_address + desc_.channel_count - 1)};
        }
        if (desc_.users_address != 0) {
            ranges_[2] = {desc_.users_address, static_cast<u16>(desc_.users_address + desc_.user_count - 1)};
        }
    }
}

void EventSystem::reset() noexcept {
    strobe_ = 0;
    std::fill(channels_.begin(), channels_.end(), 0);
    std::fill(users_.begin(), users_.end(), 0);
    rebuild_cache();
}

std::span<const AddressRange> EventSystem::mapped_ranges() const noexcept {
    size_t count = 0;
    while (count < ranges_.size() && ranges_[count].begin != 0) count++;
    return {ranges_.data(), count};
}

u8 EventSystem::read(u16 address) noexcept {
    if (address == desc_.strobe_address) return strobe_;
    
    if (address >= desc_.channels_address && address < desc_.channels_address + desc_.channel_count) {
        return channels_[address - desc_.channels_address];
    }
    
    if (address >= desc_.users_address && address < desc_.users_address + desc_.user_count) {
        return users_[address - desc_.users_address];
    }

    return 0;
}

void EventSystem::write(u16 address, u8 value) noexcept {
    if (address == desc_.strobe_address) {
        strobe_ = value;
        // Handle manual strobes
        for (u8 i = 0; i < desc_.channel_count; ++i) {
            if (value & (1 << i)) {
                // Trigger events on this channel manually? 
                // EVSYS strobe triggers the USERS of that channel.
                for (size_t u = 0; u < users_.size(); ++u) {
                    if (users_[u] == (i + 1)) { // Channel index + 1
                        if (callbacks_[u]) callbacks_[u]();
                    }
                }
            }
        }
    } else if (address >= desc_.channels_address && address < desc_.channels_address + desc_.channel_count) {
        channels_[address - desc_.channels_address] = value;
        rebuild_cache();
    } else if (address >= desc_.users_address && address < desc_.users_address + desc_.user_count) {
        users_[address - desc_.users_address] = value;
        rebuild_cache();
    }
}

void EventSystem::tick(u64) noexcept {
    // Event actions are mostly immediate upon trigger_event call
}

void EventSystem::trigger_event(u8 generator_id) noexcept {
    if (generator_id == 0) return; // OFF
    
    // Direct generator listeners (mostly for internal simulation hooks)
    if (auto it = generator_callbacks_.find(generator_id); it != generator_callbacks_.end()) {
        for (auto& cb : it->second) {
            if (cb) cb();
        }
    }

    // Routed users via cache
    if (auto it = routing_cache_.find(generator_id); it != routing_cache_.end()) {
        for (u8 user_index : it->second) {
            if (callbacks_[user_index]) {
                callbacks_[user_index]();
            }
        }
    }
}

void EventSystem::rebuild_cache() noexcept {
    routing_cache_.clear();
    
    for (size_t c = 0; c < channels_.size(); ++c) {
        u8 generator_id = channels_[c];
        if (generator_id == 0) continue;
        
        // Find all users of this channel (c + 1)
        for (size_t u = 0; u < users_.size(); ++u) {
            if (users_[u] == static_cast<u8>(c + 1)) {
                routing_cache_[generator_id].push_back(static_cast<u8>(u));
            }
        }
    }
}

void EventSystem::register_user_callback(u8 user_index, EventCallback callback) {
    if (user_index < callbacks_.size()) {
        callbacks_[user_index] = std::move(callback);
    }
}

void EventSystem::register_generator_callback(u8 generator_id, EventCallback callback) {
    if (generator_id != 0) {
        generator_callbacks_[generator_id].push_back(std::move(callback));
    }
}

} // namespace vioavr::core
