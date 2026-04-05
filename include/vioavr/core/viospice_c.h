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

// Stepping
void vioavr_step_duration(VioSpiceHandle handle, double seconds);
void vioavr_tick_timer2_async(VioSpiceHandle handle, uint64_t ticks);

// Pin Mapping & I/O
void vioavr_add_pin_mapping(VioSpiceHandle handle, const char* port_name, uint8_t bit_index, uint32_t external_id);
void vioavr_set_external_pin(VioSpiceHandle handle, uint32_t external_id, VioAvrPinLevel level);

// Results (Called by SPICE to see if any pins changed)
typedef struct {
    uint32_t external_id;
    VioAvrPinLevel level;
} VioAvrPinChange;

// Returns the number of changes collected
int vioavr_consume_pin_changes(VioSpiceHandle handle, VioAvrPinChange* changes, int max_changes);

#ifdef __cplusplus
}
#endif
