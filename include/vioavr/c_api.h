#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ---- API version ----
typedef struct { uint32_t major, minor, patch; } VioAvrVersion;
VioAvrVersion vioavr_get_api_version(void);

// ---- Opaque handle to a VioAVR simulation instance ----
typedef void* VioSpiceHandle;

// ---- Error codes ----
typedef enum {
    VIOAVR_OK = 0,
    VIOAVR_ERR_NULL_HANDLE,
    VIOAVR_ERR_NOT_FOUND,
    VIOAVR_ERR_LOAD_FAILED,
    VIOAVR_ERR_INVALID_PARAM,
    VIOAVR_ERR_INTERNAL,
} VioAvrError;

// ---- Pin level ----
typedef enum { VIOAVR_LEVEL_LOW = 0, VIOAVR_LEVEL_HIGH = 1 } VioAvrPinLevel;

// ---- CPU execution state ----
typedef enum {
    VIOAVR_CPU_RUNNING = 0,
    VIOAVR_CPU_HALTED,
    VIOAVR_CPU_SLEEPING,
    VIOAVR_CPU_PAUSED,
} VioAvrCpuState;

// ---- Pin change event (output via vioavr_consume_pin_changes) ----
typedef struct {
    uint32_t external_id;
    VioAvrPinLevel level;
    bool is_output;
    bool wired_and;
} VioAvrPinChange;

// ---- Snapshot for save/restore (opaque, sized for ATmega328P + SRAM) ----
#define VIOAVR_MAX_SNAPSHOT_SIZE 4096
typedef struct {
    uint8_t data[VIOAVR_MAX_SNAPSHOT_SIZE];
    uint32_t size;
} VioAvrSnapshot;

// =========================================================================
// Lifecycle
// =========================================================================

VioSpiceHandle vioavr_create(const char* device_name);
void vioavr_destroy(VioSpiceHandle handle);
VioAvrError vioavr_load_hex(VioSpiceHandle handle, const char* path);
VioAvrError vioavr_load_hex_data(VioSpiceHandle handle, const char* hex_data, size_t len);
void vioavr_reset(VioSpiceHandle handle);

// =========================================================================
// Device catalog
// =========================================================================

int vioavr_device_count(void);
const char* vioavr_device_name(int index);

// =========================================================================
// Error inspection
// =========================================================================

VioAvrError vioavr_get_last_error_code(VioSpiceHandle handle);
const char* vioavr_get_last_error(VioSpiceHandle handle);

// =========================================================================
// Execution control
// =========================================================================

void vioavr_set_quantum(VioSpiceHandle handle, uint64_t cycles);
VioAvrError vioavr_step_duration(VioSpiceHandle handle, double seconds);
void vioavr_set_frequency(VioSpiceHandle handle, double hz);
void vioavr_tick_timer2_async(VioSpiceHandle handle, uint64_t ticks);
#ifdef VIOAVR_HAVE_JIT
void vioavr_enable_jit(VioSpiceHandle handle, bool enabled);
#endif

// =========================================================================
// Pin mapping and digital I/O
// =========================================================================

void vioavr_add_pin_mapping(VioSpiceHandle handle, const char* port_name, uint8_t bit_index, uint32_t external_id);
void vioavr_set_external_pin(VioSpiceHandle handle, uint32_t external_id, VioAvrPinLevel level);
VioAvrPinLevel vioavr_get_pin_level(VioSpiceHandle handle, uint32_t external_id);
int vioavr_consume_pin_changes(VioSpiceHandle handle, VioAvrPinChange* changes, int max_changes);

// =========================================================================
// Analog I/O
// =========================================================================

VioAvrError vioavr_set_external_voltage(VioSpiceHandle handle, uint8_t channel, double voltage_volts);
VioAvrError vioavr_set_external_voltage_by_pin(VioSpiceHandle handle, uint32_t external_id, double voltage);
VioAvrError vioavr_set_external_voltages(VioSpiceHandle handle, uint32_t count, const uint32_t* external_ids, const double* voltages);
void vioavr_set_operating_voltage(VioSpiceHandle handle, double vcc_volts);
int vioavr_get_analog_outputs(VioSpiceHandle handle, double* outputs, int max_outputs);

// =========================================================================
// CPU introspection
// =========================================================================

uint64_t vioavr_get_cycles(VioSpiceHandle handle);
uint64_t vioavr_get_instructions_executed(VioSpiceHandle handle);
uint64_t vioavr_get_sleep_cycles(VioSpiceHandle handle);
uint64_t vioavr_get_pc(VioSpiceHandle handle);
uint8_t  vioavr_get_sreg(VioSpiceHandle handle);
uint16_t vioavr_get_sp(VioSpiceHandle handle);
VioAvrCpuState vioavr_get_cpu_state(VioSpiceHandle handle);

// =========================================================================
// Data-space memory access (registers, I/O, SRAM)
// =========================================================================

uint8_t vioavr_read_data(VioSpiceHandle handle, uint16_t address);
void vioavr_write_data(VioSpiceHandle handle, uint16_t address, uint8_t value);

// =========================================================================
// Debug CLI command execution
// =========================================================================

VioAvrError vioavr_debug_command(VioSpiceHandle handle, const char* command, char* out_buf, size_t out_buf_len);

// =========================================================================
// UART I/O (first hardware UART)
// =========================================================================

bool vioavr_uart_has_tx_byte(VioSpiceHandle handle);
uint16_t vioavr_uart_read_tx_byte(VioSpiceHandle handle);
bool vioavr_uart_can_inject_rx(VioSpiceHandle handle);
VioAvrError vioavr_uart_inject_rx(VioSpiceHandle handle, uint8_t byte);

// =========================================================================
// HC-05 Bluetooth bridge
// =========================================================================

void vioavr_enable_hc05(VioSpiceHandle handle);
bool vioavr_hc05_has_tx_byte(VioSpiceHandle handle);
uint8_t vioavr_hc05_read_tx_byte(VioSpiceHandle handle);
void vioavr_hc05_inject_data(VioSpiceHandle handle, const uint8_t* data, uint16_t len);
void vioavr_set_hc05_pty_fd(VioSpiceHandle handle, int fd);
void vioavr_set_ircom_output_pin(VioSpiceHandle handle, uint32_t external_id);

// =========================================================================
// Snapshot save/restore (for cosimulation rollback)
// =========================================================================

VioAvrError vioavr_save_state(VioSpiceHandle handle, VioAvrSnapshot* snapshot);
VioAvrError vioavr_restore_state(VioSpiceHandle handle, const VioAvrSnapshot* snapshot);

#ifdef __cplusplus
}
#endif
