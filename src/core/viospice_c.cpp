#include "vioavr/core/viospice_c.h"
#include "vioavr/core/viospice.hpp"
#include "vioavr/core/device_catalog.hpp"
#include <cstring>

using namespace vioavr::core;

extern "C" {

VioSpiceHandle vioavr_create(const char* device_name) {
    const DeviceDescriptor* desc = DeviceCatalog::find(device_name);
    if (!desc) {
        desc = DeviceCatalog::find("atmega328p"); // fallback
    }
    
    if (!desc) return nullptr;
    
    return static_cast<VioSpiceHandle>(new VioSpice(*desc));
}

void vioavr_destroy(VioSpiceHandle handle) {
    delete static_cast<VioSpice*>(handle);
}

bool vioavr_load_hex(VioSpiceHandle handle, const char* path) {
    return static_cast<VioSpice*>(handle)->load_hex(path);
}

void vioavr_reset(VioSpiceHandle handle) {
    static_cast<VioSpice*>(handle)->reset();
}

void vioavr_set_quantum(VioSpiceHandle handle, uint64_t cycles) {
    static_cast<VioSpice*>(handle)->set_quantum(cycles);
}

void vioavr_step_duration(VioSpiceHandle handle, double seconds) {
    static_cast<VioSpice*>(handle)->step_duration(seconds);
}

void vioavr_tick_timer2_async(VioSpiceHandle handle, uint64_t ticks) {
    static_cast<VioSpice*>(handle)->tick_timer2_async(ticks);
}

void vioavr_add_pin_mapping(VioSpiceHandle handle, const char* port_name, uint8_t bit_index, uint32_t external_id) {
    static_cast<VioSpice*>(handle)->add_pin_mapping(port_name, bit_index, external_id);
}

void vioavr_set_external_pin(VioSpiceHandle handle, uint32_t external_id, VioAvrPinLevel level) {
    static_cast<VioSpice*>(handle)->set_external_pin(external_id, 
        (level == VIOAVR_LEVEL_HIGH) ? PinLevel::high : PinLevel::low);
}

int vioavr_consume_pin_changes(VioSpiceHandle handle, VioAvrPinChange* changes, int max_changes) {
    VioSpice* spice = static_cast<VioSpice*>(handle);
    auto internal_changes = spice->consume_pin_changes();
    int count = 0;
    
    for (const auto& change : internal_changes) {
        if (count >= max_changes) break;
        
        auto ext_id = spice->get_external_id(change.port_name, change.bit_index);
        if (ext_id) {
            changes[count].external_id = *ext_id;
            changes[count].level = change.level ? VIOAVR_LEVEL_HIGH : VIOAVR_LEVEL_LOW;
            count++;
        }
    }
    
    return count; 
}

} // extern "C"
