#pragma once

#include "vioavr/core/types.hpp"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <optional>

namespace vioavr::core {

/**
 * @brief Maps an AVR pin (Port + Bit) to an external world identifier (e.g., ngspice node ID).
 */
struct PinMapping {
    std::string port_name;
    u8 bit_index;
    u32 external_id; // Could be a node index in ngspice
    std::string label; // Friendly name like "LED1" or "Net_42"
};

class PinMap {
public:
    void add_mapping(std::string_view port_name, u8 bit_index, u32 external_id, std::string_view label = "") {
        const std::string key = make_key(port_name, bit_index);
        
        mappings_.push_back(PinMapping{
            .port_name = std::string(port_name),
            .bit_index = bit_index,
            .external_id = external_id,
            .label = std::string(label)
        });
        
        PinMapping* ptr = &mappings_.back();
        lookup_by_pin_[key] = ptr;
        lookup_by_external_[external_id] = ptr;
    }

    void clear() {
        mappings_.clear();
        lookup_by_pin_.clear();
        lookup_by_external_.clear();
    }

    [[nodiscard]] std::optional<u32> get_external_id(std::string_view port_name, u8 bit_index) const {
        const std::string key = make_key(port_name, bit_index);
        if (auto it = lookup_by_pin_.find(key); it != lookup_by_pin_.end()) {
            return it->second->external_id;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<PinMapping> get_mapping_by_external(u32 external_id) const {
        if (auto it = lookup_by_external_.find(external_id); it != lookup_by_external_.end()) {
            return *it->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] size_t size() const noexcept {
        return mappings_.size();
    }

private:
    [[nodiscard]] static std::string make_key(std::string_view port_name, u8 bit_index) {
        return std::string(port_name) + ":" + std::to_string(bit_index);
    }

    std::deque<PinMapping> mappings_;
    std::unordered_map<std::string, PinMapping*> lookup_by_pin_;
    std::unordered_map<u32, PinMapping*> lookup_by_external_;
};

}  // namespace vioavr::core
