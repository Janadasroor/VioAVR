#include "vioavr/core/viospice_c.h"
#include "vioavr/core/viospice.hpp"
#include "vioavr/core/device_catalog.hpp"
#include <cstdio>
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

#ifdef VIOAVR_HAVE_JIT
void vioavr_enable_jit(VioSpiceHandle handle, bool enabled) {
    static_cast<VioSpice*>(handle)->cpu().enable_jit(enabled);
}
#endif

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

void vioavr_set_external_voltage(VioSpiceHandle handle, uint8_t channel, double voltage_volts) {
    static_cast<VioSpice*>(handle)->set_external_voltage(channel, voltage_volts);
}

void vioavr_set_external_voltage_by_pin(VioSpiceHandle handle, uint32_t external_id, double voltage) {
    static_cast<VioSpice*>(handle)->set_external_voltage_to_digital(external_id, voltage);
}

void vioavr_set_operating_voltage(VioSpiceHandle handle, double vcc_volts) {
    static_cast<VioSpice*>(handle)->set_operating_voltage(vcc_volts);
}

int vioavr_get_analog_outputs(VioSpiceHandle handle, double* outputs, int max_outputs) {
    auto values = static_cast<VioSpice*>(handle)->get_analog_outputs();
    int count = 0;
    for (const auto& v : values) {
        if (count >= max_outputs) break;
        outputs[count++] = v;
    }
    return count;
}

uint64_t vioavr_get_cycles(VioSpiceHandle handle) {
    return static_cast<VioSpice*>(handle)->cpu().cycles();
}

void vioavr_enable_hc05(VioSpiceHandle handle) {
    static_cast<VioSpice*>(handle)->enable_hc05();
}

bool vioavr_hc05_has_tx_byte(VioSpiceHandle handle) {
    return static_cast<VioSpice*>(handle)->hc05().has_tx_byte();
}

uint8_t vioavr_hc05_read_tx_byte(VioSpiceHandle handle) {
    return static_cast<VioSpice*>(handle)->hc05().read_tx_byte();
}

void vioavr_hc05_inject_data(VioSpiceHandle handle, const uint8_t* data, uint16_t len) {
    static_cast<VioSpice*>(handle)->hc05().inject_external_data(data, len);
}

void vioavr_set_hc05_pty_fd(VioSpiceHandle handle, int fd) {
    static_cast<VioSpice*>(handle)->set_hc05_pty_fd(fd);
}

void vioavr_set_ircom_output_pin(VioSpiceHandle handle, uint32_t external_id) {
    static_cast<VioSpice*>(handle)->set_ircom_output_pin(external_id);
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
            changes[count].is_output = change.is_output;
            count++;
        }
    }
    
    return count; 
}

} // extern "C"
