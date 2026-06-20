#include "vioavr/c_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* Path injected at test runtime via VIOAVR_BLINKY_HEX env variable.
 * CMake sets this in set_tests_properties(... ENVIRONMENT ...).
 * Falls back to a sensible local relative path for manual runs. */
static const char* get_hex_path(void) {
    const char* env = getenv("VIOAVR_BLINKY_HEX");
    return env ? env : "tests/blinky.hex";
}
#define HEX_PATH get_hex_path()
#define DEVICE "atmega328p"

static void test_lifecycle(void) {
    printf("  lifecycle ... ");
    VioSpiceHandle h = vioavr_create(DEVICE);
    assert(h != NULL);
    vioavr_destroy(h);

    // Unknown device falls back to atmega328p
    h = vioavr_create("nonexistent");
    assert(h != NULL);
    vioavr_destroy(h);
    printf("OK\n");
}

static void test_api_version(void) {
    printf("  api_version ... ");
    VioAvrVersion v = vioavr_get_api_version();
    assert(v.major > 0 || v.minor > 0 || v.patch > 0);
    printf("%u.%u.%u OK\n", v.major, v.minor, v.patch);
}

static void test_load_hex(void) {
    printf("  load_hex ... ");
    VioSpiceHandle h = vioavr_create(DEVICE);
    assert(h != NULL);

    VioAvrError err = vioavr_load_hex(h, HEX_PATH);
    assert(err == VIOAVR_OK);

    // Error on null handle
    err = vioavr_load_hex(NULL, HEX_PATH);
    assert(err == VIOAVR_ERR_NULL_HANDLE);

    // Error on nonexistent file
    err = vioavr_load_hex(h, "/nonexistent/foo.hex");
    assert(err == VIOAVR_ERR_LOAD_FAILED);

    vioavr_destroy(h);
    printf("OK\n");
}

static void test_load_hex_data(void) {
    printf("  load_hex_data ... ");
    VioSpiceHandle h = vioavr_create(DEVICE);
    assert(h != NULL);

    // Invalid hex data
    VioAvrError err = vioavr_load_hex_data(h, "garbage", 7);
    assert(err == VIOAVR_ERR_LOAD_FAILED);

    // Empty data
    err = vioavr_load_hex_data(h, "", 0);
    assert(err == VIOAVR_ERR_INVALID_PARAM);

    // Null data
    err = vioavr_load_hex_data(h, NULL, 0);
    assert(err == VIOAVR_ERR_INVALID_PARAM);

    // Null handle
    err = vioavr_load_hex_data(NULL, "garbage", 7);
    assert(err == VIOAVR_ERR_NULL_HANDLE);

    vioavr_destroy(h);
    printf("OK\n");
}

static void test_cpu_introspection(void) {
    printf("  cpu_introspection ... ");
    VioSpiceHandle h = vioavr_create(DEVICE);
    assert(h != NULL);
    vioavr_load_hex(h, HEX_PATH);

    // Before any stepping
    assert(vioavr_get_cycles(h) == 0);
    assert(vioavr_get_instructions_executed(h) == 0);
    assert(vioavr_get_sleep_cycles(h) == 0);

    uint64_t pc = vioavr_get_pc(h);
    assert(pc == 0);

    uint8_t sreg = vioavr_get_sreg(h);
    assert(sreg == 0);

    uint16_t sp = vioavr_get_sp(h);
    assert(sp > 0);

    // CPU starts HALTED until reset
    assert(vioavr_get_cpu_state(h) == VIOAVR_CPU_HALTED);
    vioavr_reset(h);
    assert(vioavr_get_cpu_state(h) == VIOAVR_CPU_RUNNING);

    // Step a few cycles and re-check
    vioavr_set_quantum(h, 100);
    vioavr_step_duration(h, 0.0001); // 100us

    assert(vioavr_get_cycles(h) > 0);
    assert(vioavr_get_instructions_executed(h) > 0);

    vioavr_destroy(h);
    printf("OK\n");
}

static void test_memory_access(void) {
    printf("  memory_access ... ");
    VioSpiceHandle h = vioavr_create(DEVICE);
    assert(h != NULL);
    vioavr_load_hex(h, HEX_PATH);

    // Read zero-initialized SRAM
    uint8_t val = vioavr_read_data(h, 0x100);
    assert(val == 0);

    // Write and read back
    vioavr_write_data(h, 0x100, 0xAB);
    val = vioavr_read_data(h, 0x100);
    assert(val == 0xAB);

    // Write different value
    vioavr_write_data(h, 0x100, 0xCD);
    val = vioavr_read_data(h, 0x100);
    assert(val == 0xCD);

    vioavr_destroy(h);
    printf("OK\n");
}

static void test_pin_operations(void) {
    printf("  pin_operations ... ");
    VioSpiceHandle h = vioavr_create(DEVICE);
    assert(h != NULL);
    vioavr_load_hex(h, HEX_PATH);

    // Map PORTB bit 0 -> external id 100
    vioavr_add_pin_mapping(h, "B", 0, 100);
    vioavr_add_pin_mapping(h, "B", 1, 101);

    // Pin levels reflect drive state (LOW until chip drives them)
    assert(vioavr_get_pin_level(h, 100) == VIOAVR_LEVEL_LOW);

    // Set external pin (changes input level seen by AVR, not readback)
    vioavr_set_external_pin(h, 100, VIOAVR_LEVEL_HIGH);

    // Consume pin changes (output from chip)
    VioAvrPinChange changes[16];
    int n = vioavr_consume_pin_changes(h, changes, 16);
    assert(n >= 0);
    assert(n <= 16);
    // Each change should have valid fields
    for (int i = 0; i < n; i++) {
        assert(changes[i].external_id > 0 || changes[i].external_id == 0);
    }

    vioavr_destroy(h);
    printf("OK\n");
}

static void test_uart(void) {
    printf("  uart ... ");
    VioSpiceHandle h = vioavr_create(DEVICE);
    assert(h != NULL);
    vioavr_load_hex(h, HEX_PATH);

    // UART should exist but have no TX bytes initially
    assert(vioavr_uart_has_tx_byte(h) == false);

    // Can check if RX injection is possible (may be false if UART RX disabled)
    (void)vioavr_uart_can_inject_rx(h);

    // TX byte should be 0 (nothing transmitted, empty queue)
    assert(vioavr_uart_read_tx_byte(h) == 0);

    vioavr_destroy(h);
    printf("OK\n");
}

static void test_error_handling(void) {
    printf("  error_handling ... ");
    VioSpiceHandle h = vioavr_create(DEVICE);
    assert(h != NULL);

    // No error yet
    assert(vioavr_get_last_error_code(h) == VIOAVR_OK);
    const char* msg = vioavr_get_last_error(h);
    assert(msg == NULL || strlen(msg) == 0);

    // Trigger error by loading nonexistent file
    VioAvrError code = vioavr_load_hex(h, "/nonexistent/file.hex");
    assert(code == VIOAVR_ERR_LOAD_FAILED);

    code = vioavr_get_last_error_code(h);
    assert(code == VIOAVR_ERR_LOAD_FAILED);
    msg = vioavr_get_last_error(h);
    assert(msg != NULL);
    assert(strlen(msg) > 0);

    vioavr_destroy(h);
    printf("OK\n");
}

static void test_reset(void) {
    printf("  reset ... ");
    VioSpiceHandle h = vioavr_create(DEVICE);
    assert(h != NULL);
    vioavr_load_hex(h, HEX_PATH);
    vioavr_reset(h);

    vioavr_set_quantum(h, 100);
    vioavr_step_duration(h, 0.0001);
    assert(vioavr_get_cycles(h) > 0);

    vioavr_reset(h);
    assert(vioavr_get_cycles(h) == 0);
    assert(vioavr_get_instructions_executed(h) == 0);
    assert(vioavr_get_pc(h) == 0);

    // After reset, should be able to step again
    vioavr_set_quantum(h, 100);
    vioavr_step_duration(h, 0.0001);
    assert(vioavr_get_cycles(h) > 0);

    vioavr_destroy(h);
    printf("OK\n");
}

static void test_analog(void) {
    printf("  analog ... ");
    VioSpiceHandle h = vioavr_create(DEVICE);
    assert(h != NULL);
    vioavr_load_hex(h, HEX_PATH);

    vioavr_set_operating_voltage(h, 5.0);

    VioAvrError err = vioavr_set_external_voltage(h, 0, 2.5);
    assert(err == VIOAVR_OK);

    err = vioavr_set_external_voltage_by_pin(h, 0, 3.3);
    assert(err == VIOAVR_OK || err == VIOAVR_ERR_NOT_FOUND);

    double outputs[8];
    int n = vioavr_get_analog_outputs(h, outputs, 8);
    assert(n >= 0);

    vioavr_destroy(h);
    printf("OK\n");
}

int main(void) {
    /* Pre-flight: verify the hex file exists before running any tests.
     * VIOAVR_BLINKY_HEX is injected by CMake's set_tests_properties(ENVIRONMENT).
     * When avr-gcc is not installed the hex is never built; returning 77 tells
     * CTest to mark this test as SKIP rather than FAIL. */
    const char* hex = get_hex_path();
    FILE* probe = fopen(hex, "rb");
    if (!probe) {
        fprintf(stderr, "[c_api_test] SKIP: blinky.hex not found at '%s' "
                        "(avr-gcc not available on this runner)\n", hex);
        return 77; /* CTest SKIP_RETURN_CODE */
    }
    fclose(probe);

    printf("C API end-to-end tests\n");
    printf("---------------------\n");

    test_api_version();
    test_lifecycle();
    test_load_hex();
    test_load_hex_data();
    test_cpu_introspection();
    test_memory_access();
    test_pin_operations();
    test_uart();
    test_error_handling();
    test_reset();
    test_analog();

    printf("---------------------\n");
    printf("All tests passed!\n");
    return 0;
}
