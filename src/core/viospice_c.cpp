#include "vioavr/core/viospice_c.h"
#include "vioavr/core/viospice.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/uart.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <stdexcept>

using namespace vioavr::core;

extern "C" {

// =========================================================================
// Version
// =========================================================================

VioAvrVersion vioavr_get_api_version(void) {
    return { 0, 1, 0 };
}

// =========================================================================
// Internal helpers
// =========================================================================

static VioSpice* to_spice(VioSpiceHandle handle) {
    return static_cast<VioSpice*>(handle);
}

static VioAvrError code_to_error(int code) {
    if (code <= 0) return VIOAVR_OK;
    return static_cast<VioAvrError>(code);
}

// =========================================================================
// Lifecycle
// =========================================================================

VioSpiceHandle vioavr_create(const char* device_name) {
    if (!device_name) return nullptr;
    const DeviceDescriptor* desc = DeviceCatalog::find(device_name);
    if (!desc) {
        desc = DeviceCatalog::find("atmega328p");
    }
    if (!desc) return nullptr;
    return static_cast<VioSpiceHandle>(new VioSpice(*desc));
}

void vioavr_destroy(VioSpiceHandle handle) {
    delete to_spice(handle);
}

VioAvrError vioavr_load_hex(VioSpiceHandle handle, const char* path) {
    auto* spice = to_spice(handle);
    if (!spice) return VIOAVR_ERR_NULL_HANDLE;
    if (!path || !*path) {
        spice->set_error("Invalid hex file path", VIOAVR_ERR_INVALID_PARAM);
        return VIOAVR_ERR_INVALID_PARAM;
    }
    spice->clear_error();
    bool ok = spice->load_hex(path);
    if (!ok) {
        char buf[512];
        std::snprintf(buf, sizeof(buf), "Failed to load hex file: %s", path);
        spice->set_error(buf, VIOAVR_ERR_LOAD_FAILED);
    }
    return ok ? VIOAVR_OK : VIOAVR_ERR_LOAD_FAILED;
}

VioAvrError vioavr_load_hex_data(VioSpiceHandle handle, const char* hex_data, size_t len) {
    auto* spice = to_spice(handle);
    if (!spice) return VIOAVR_ERR_NULL_HANDLE;
    if (!hex_data || len == 0) {
        spice->set_error("Empty hex data", VIOAVR_ERR_INVALID_PARAM);
        return VIOAVR_ERR_INVALID_PARAM;
    }
    spice->clear_error();
    std::string s(hex_data, len);
    try {
        HexImage image = HexImageLoader::load_text(s, spice->bus().device());
        if (image.flash_words.empty()) {
            spice->set_error("No flash data found in hex data", VIOAVR_ERR_LOAD_FAILED);
            return VIOAVR_ERR_LOAD_FAILED;
        }
        spice->bus().load_image(image);
        return VIOAVR_OK;
    } catch (const std::exception& e) {
        spice->set_error(e.what(), VIOAVR_ERR_LOAD_FAILED);
        return VIOAVR_ERR_LOAD_FAILED;
    }
}

void vioavr_reset(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    if (spice) spice->reset();
}

// =========================================================================
// Error inspection
// =========================================================================

VioAvrError vioavr_get_last_error_code(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    if (!spice) return VIOAVR_ERR_NULL_HANDLE;
    return code_to_error(spice->last_error_code());
}

const char* vioavr_get_last_error(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    if (!spice) return "Null handle";
    return spice->last_error().c_str();
}

// =========================================================================
// Execution control
// =========================================================================

void vioavr_set_quantum(VioSpiceHandle handle, uint64_t cycles) {
    auto* spice = to_spice(handle);
    if (spice) spice->set_quantum(cycles);
}

VioAvrError vioavr_step_duration(VioSpiceHandle handle, double seconds) {
    auto* spice = to_spice(handle);
    if (!spice) return VIOAVR_ERR_NULL_HANDLE;
    spice->step_duration(seconds);
    return VIOAVR_OK;
}

void vioavr_tick_timer2_async(VioSpiceHandle handle, uint64_t ticks) {
    auto* spice = to_spice(handle);
    if (spice) spice->tick_timer2_async(ticks);
}

#ifdef VIOAVR_HAVE_JIT
void vioavr_enable_jit(VioSpiceHandle handle, bool enabled) {
    auto* spice = to_spice(handle);
    if (spice) spice->cpu().enable_jit(enabled);
}
#endif

// =========================================================================
// Pin mapping & digital I/O
// =========================================================================

void vioavr_add_pin_mapping(VioSpiceHandle handle, const char* port_name, uint8_t bit_index, uint32_t external_id) {
    auto* spice = to_spice(handle);
    if (spice && port_name) spice->add_pin_mapping(port_name, bit_index, external_id);
}

void vioavr_set_external_pin(VioSpiceHandle handle, uint32_t external_id, VioAvrPinLevel level) {
    auto* spice = to_spice(handle);
    if (spice) spice->set_external_pin(external_id,
        (level == VIOAVR_LEVEL_HIGH) ? PinLevel::high : PinLevel::low);
}

VioAvrPinLevel vioavr_get_pin_level(VioSpiceHandle handle, uint32_t external_id) {
    auto* spice = to_spice(handle);
    if (!spice) return VIOAVR_LEVEL_LOW;
    auto level = spice->get_pin_level(external_id);
    return (level == PinLevel::high) ? VIOAVR_LEVEL_HIGH : VIOAVR_LEVEL_LOW;
}

int vioavr_consume_pin_changes(VioSpiceHandle handle, VioAvrPinChange* changes, int max_changes) {
    auto* spice = to_spice(handle);
    if (!spice) return 0;
    auto internal_changes = spice->consume_pin_changes();
    int count = 0;
    for (const auto& change : internal_changes) {
        if (count >= max_changes) break;
        auto ext_id = spice->get_external_id(change.port_name, change.bit_index);
        if (ext_id) {
            changes[count].external_id = *ext_id;
            changes[count].level = change.level ? VIOAVR_LEVEL_HIGH : VIOAVR_LEVEL_LOW;
            changes[count].is_output = change.is_output;
            changes[count].wired_and = change.wired_and;
            count++;
        }
    }
    return count;
}

// =========================================================================
// Analog I/O
// =========================================================================

VioAvrError vioavr_set_external_voltage(VioSpiceHandle handle, uint8_t channel, double voltage_volts) {
    auto* spice = to_spice(handle);
    if (!spice) return VIOAVR_ERR_NULL_HANDLE;
    spice->set_external_voltage(channel, voltage_volts);
    return VIOAVR_OK;
}

VioAvrError vioavr_set_external_voltage_by_pin(VioSpiceHandle handle, uint32_t external_id, double voltage) {
    auto* spice = to_spice(handle);
    if (!spice) return VIOAVR_ERR_NULL_HANDLE;
    spice->set_external_voltage_to_digital(external_id, voltage);
    return VIOAVR_OK;
}

void vioavr_set_operating_voltage(VioSpiceHandle handle, double vcc_volts) {
    auto* spice = to_spice(handle);
    if (spice) spice->set_operating_voltage(vcc_volts);
}

int vioavr_get_analog_outputs(VioSpiceHandle handle, double* outputs, int max_outputs) {
    auto* spice = to_spice(handle);
    if (!spice) return 0;
    auto values = spice->get_analog_outputs();
    int count = 0;
    for (const auto& v : values) {
        if (count >= max_outputs) break;
        outputs[count++] = v;
    }
    return count;
}

// =========================================================================
// CPU introspection
// =========================================================================

uint64_t vioavr_get_cycles(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    return spice ? spice->cpu().cycles() : 0;
}

uint64_t vioavr_get_instructions_executed(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    return spice ? spice->instructions_executed() : 0;
}

uint64_t vioavr_get_sleep_cycles(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    return spice ? spice->sleep_cycles() : 0;
}

uint64_t vioavr_get_pc(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    return spice ? spice->cpu().program_counter() : 0;
}

uint8_t vioavr_get_sreg(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    return spice ? spice->cpu().sreg() : 0;
}

uint16_t vioavr_get_sp(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    return spice ? spice->cpu().stack_pointer() : 0;
}

VioAvrCpuState vioavr_get_cpu_state(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    if (!spice) return VIOAVR_CPU_HALTED;
    switch (spice->cpu().state()) {
        case CpuState::running:  return VIOAVR_CPU_RUNNING;
        case CpuState::paused:   return VIOAVR_CPU_PAUSED;
        case CpuState::halted:   return VIOAVR_CPU_HALTED;
        case CpuState::sleeping: return VIOAVR_CPU_SLEEPING;
    }
    return VIOAVR_CPU_RUNNING;
}

// =========================================================================
// Data-space memory access
// =========================================================================

uint8_t vioavr_read_data(VioSpiceHandle handle, uint16_t address) {
    auto* spice = to_spice(handle);
    return spice ? spice->bus().read_data(address) : 0;
}

void vioavr_write_data(VioSpiceHandle handle, uint16_t address, uint8_t value) {
    auto* spice = to_spice(handle);
    if (spice) spice->bus().write_data(address, value);
}

// =========================================================================
// UART I/O
// =========================================================================

bool vioavr_uart_has_tx_byte(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    if (!spice) return false;
    auto* uart = spice->uart();
    return uart ? uart->has_tx_byte() : false;
}

uint16_t vioavr_uart_read_tx_byte(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    if (!spice) return 0;
    auto* uart = spice->uart();
    return uart ? uart->read_tx_byte() : 0;
}

bool vioavr_uart_can_inject_rx(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    if (!spice) return false;
    auto* uart = spice->uart();
    return uart != nullptr;
}

VioAvrError vioavr_uart_inject_rx(VioSpiceHandle handle, uint8_t byte) {
    auto* spice = to_spice(handle);
    if (!spice) return VIOAVR_ERR_NULL_HANDLE;
    auto* uart = spice->uart();
    if (!uart) {
        spice->set_error("No UART on this device", VIOAVR_ERR_NOT_FOUND);
        return VIOAVR_ERR_NOT_FOUND;
    }
    bool ok = uart->inject_received_byte(byte);
    if (!ok) {
        spice->set_error("UART RX rejected: receiver disabled or busy", VIOAVR_ERR_INTERNAL);
        return VIOAVR_ERR_INTERNAL;
    }
    return VIOAVR_OK;
}

// =========================================================================
// HC-05 Bluetooth bridge
// =========================================================================

void vioavr_enable_hc05(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    if (spice) spice->enable_hc05();
}

bool vioavr_hc05_has_tx_byte(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    return spice ? spice->hc05().has_tx_byte() : false;
}

uint8_t vioavr_hc05_read_tx_byte(VioSpiceHandle handle) {
    auto* spice = to_spice(handle);
    return spice ? spice->hc05().read_tx_byte() : 0;
}

void vioavr_hc05_inject_data(VioSpiceHandle handle, const uint8_t* data, uint16_t len) {
    auto* spice = to_spice(handle);
    if (spice) spice->hc05().inject_external_data(data, len);
}

void vioavr_set_hc05_pty_fd(VioSpiceHandle handle, int fd) {
    auto* spice = to_spice(handle);
    if (spice) spice->set_hc05_pty_fd(fd);
}

void vioavr_set_ircom_output_pin(VioSpiceHandle handle, uint32_t external_id) {
    auto* spice = to_spice(handle);
    if (spice) spice->set_ircom_output_pin(external_id);
}

} // extern "C"
