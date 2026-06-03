#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef void* VioSpiceHandle;

typedef enum {
    VIOAVR_LEVEL_LOW = 0,
    VIOAVR_LEVEL_HIGH = 1
} VioAvrPinLevel;

// Simulation Management
VioSpiceHandle vioavr_create(const char* device_name);
void vioavr_destroy(VioSpiceHandle handle);
bool vioavr_load_hex(VioSpiceHandle handle, const char* path);
void vioavr_reset(VioSpiceHandle handle);
void vioavr_set_quantum(VioSpiceHandle handle, uint64_t cycles);
#ifdef VIOAVR_HAVE_JIT
void vioavr_enable_jit(VioSpiceHandle handle, bool enabled);
#endif

// Stepping
void vioavr_step_duration(VioSpiceHandle handle, double seconds);
void vioavr_tick_timer2_async(VioSpiceHandle handle, uint64_t ticks);

// Pin Mapping & I/O
void vioavr_add_pin_mapping(VioSpiceHandle handle, const char* port_name, uint8_t bit_index, uint32_t external_id);
void vioavr_set_external_pin(VioSpiceHandle handle, uint32_t external_id, VioAvrPinLevel level);

// Analog voltage injection (for ADC / comparator inputs)
void vioavr_set_external_voltage(VioSpiceHandle handle, uint8_t channel, double voltage_volts);
void vioavr_set_external_voltage_by_pin(VioSpiceHandle handle, uint32_t external_id, double voltage);
void vioavr_set_operating_voltage(VioSpiceHandle handle, double vcc_volts);

// Query analog outputs (DAC voltages)
int vioavr_get_analog_outputs(VioSpiceHandle handle, double* outputs, int max_outputs);

// HC-05 Bluetooth bridge
void vioavr_enable_hc05(VioSpiceHandle handle);
bool vioavr_hc05_has_tx_byte(VioSpiceHandle handle);
uint8_t vioavr_hc05_read_tx_byte(VioSpiceHandle handle);
void vioavr_hc05_inject_data(VioSpiceHandle handle, const uint8_t* data, uint16_t len);
void vioavr_set_hc05_pty_fd(VioSpiceHandle handle, int fd);

void vioavr_set_ircom_output_pin(VioSpiceHandle handle, uint32_t external_id);

// Query
uint64_t vioavr_get_cycles(VioSpiceHandle handle);

// Results (Called by SPICE to see if any pins changed)
typedef struct {
    uint32_t external_id;
    VioAvrPinLevel level;
    bool is_output;
} VioAvrPinChange;

// Returns the number of changes collected
int vioavr_consume_pin_changes(VioSpiceHandle handle, VioAvrPinChange* changes, int max_changes);

#ifdef __cplusplus
}
#endif
