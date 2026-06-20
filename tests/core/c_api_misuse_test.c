#include "vioavr/c_api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int failures = 0;
#define TEST(name) do { printf("  %-45s ", name); } while(0)
#define CHECK(cond, msg) do { \
    if (!(cond)) { failures++; printf("FAIL: %s\n", msg); } \
    else printf("OK\n"); \
} while(0)

int main(void) {
    printf("C API Misuse & Error Handling Tests\n");
    printf("====================================\n");

    // ── NULL/Invalid handle tests ──
    TEST("vioavr_create(NULL)");
    CHECK(vioavr_create(NULL) == NULL, "Expected NULL for NULL device name");

    TEST("vioavr_destroy(NULL)");
    vioavr_destroy(NULL); // should not crash
    CHECK(1, "no crash on NULL destroy");

    TEST("vioavr_load_hex(NULL, path)");
    CHECK(vioavr_load_hex(NULL, "/some/file.hex") == VIOAVR_ERR_NULL_HANDLE, "Expected NULL_HANDLE");

    TEST("vioavr_load_hex(h, NULL)");
    VioSpiceHandle h = vioavr_create("atmega328p");
    VioAvrError err = vioavr_load_hex(h, NULL);
    CHECK(err == VIOAVR_ERR_INVALID_PARAM, "Got INVALID_PARAM on NULL path");

    TEST("vioavr_load_hex(h, \"\")");
    err = vioavr_load_hex(h, "");
    CHECK(err == VIOAVR_ERR_INVALID_PARAM, "Got INVALID_PARAM on empty path");

    TEST("vioavr_load_hex_data(NULL, data, len)");
    CHECK(vioavr_load_hex_data(NULL, "data", 4) == VIOAVR_ERR_NULL_HANDLE, "Expected NULL_HANDLE");

    TEST("vioavr_load_hex_data(h, NULL, 0)");
    CHECK(vioavr_load_hex_data(h, NULL, 0) == VIOAVR_ERR_INVALID_PARAM, "Expected INVALID_PARAM");

    TEST("vioavr_load_hex_data(h, NULL, 5)");
    CHECK(vioavr_load_hex_data(h, NULL, 5) == VIOAVR_ERR_INVALID_PARAM, "Expected INVALID_PARAM");

    TEST("vioavr_reset(NULL)");
    vioavr_reset(NULL); // should not crash
    CHECK(1, "no crash on NULL reset");

    TEST("vioavr_set_quantum(NULL, 100)");
    vioavr_set_quantum(NULL, 100);
    CHECK(1, "no crash on NULL set_quantum");

    TEST("vioavr_step_duration(NULL, 0.001)");
    CHECK(vioavr_step_duration(NULL, 0.001) == VIOAVR_ERR_NULL_HANDLE, "Expected NULL_HANDLE");

    TEST("vioavr_step_duration(h, -1.0)");
    err = vioavr_step_duration(h, -1.0);
    CHECK(err == VIOAVR_ERR_INVALID_PARAM || err == VIOAVR_OK, "Negative duration handled");

    TEST("vioavr_step_duration(h, 1e9)");
    err = vioavr_step_duration(h, 1e9);
    CHECK(err == VIOAVR_OK || err == VIOAVR_ERR_INVALID_PARAM, "Extreme duration handled");

    // Note: use-after-destroy and double-destroy are UB (not testable)

    // ── Memory operations on invalid addresses ──
    TEST("read/write out of bounds");
    VioSpiceHandle h4 = vioavr_create("atmega328p");
    (void)vioavr_read_data(h4, 0xFFFF);
    vioavr_write_data(h4, 0xFFFF, 0xAA);
    vioavr_write_data(h4, 0, 0xFF); // should work (write to R0)
    CHECK(1, "OOB memory access doesn't crash");

    // ── Pin operations on unmapped pins ──
    TEST("unmapped pin get level");
    (void)vioavr_get_pin_level(h4, 9999);
    CHECK(1, "no crash on unmapped pin");

    TEST("unmapped pin set external");
    vioavr_set_external_pin(h4, 9999, VIOAVR_LEVEL_HIGH);
    CHECK(1, "no crash on unmapped external pin");

    TEST("NULL consume_pin_changes");
    (void)vioavr_consume_pin_changes(h4, NULL, 0);
    CHECK(1, "no crash on NULL changes buffer");

    TEST("consume_pin_changes with small buffer");
    VioAvrPinChange changes[1];
    int n = vioavr_consume_pin_changes(h4, changes, 0);
    CHECK(n >= 0, "zero max_changes handled");

    // ── Analog misuse ──
    TEST("analog outputs with NULL buffer");
    (void)vioavr_get_analog_outputs(h4, NULL, 0);
    CHECK(1, "no crash on NULL analog buffer");

    TEST("analog with negative voltage");
    err = vioavr_set_external_voltage(h4, 0, -5.0);
    CHECK(err == VIOAVR_OK, "negative voltage accepted");

    TEST("analog with extreme voltage");
    err = vioavr_set_external_voltage(h4, 0, 1e6);
    CHECK(err == VIOAVR_OK, "extreme voltage accepted");

    TEST("analog with invalid channel");
    err = vioavr_set_external_voltage(h4, 255, 3.3);
    CHECK(err == VIOAVR_OK || err == VIOAVR_ERR_NOT_FOUND || err == VIOAVR_ERR_INVALID_PARAM, "invalid channel handled");

    // ── Step without firmware ──
    TEST("step without firmware");
    VioSpiceHandle h5 = vioavr_create("atmega328p");
    // Should not crash; CPU is halted, step should do nothing
    vioavr_set_quantum(h5, 100);
    err = vioavr_step_duration(h5, 0.001);
    CHECK(err == VIOAVR_OK, "step without firmware doesn't crash");
    CHECK(vioavr_get_cycles(h5) == 0, "cycles unchanged when halted");
    vioavr_destroy(h5);

    // ── Load corrupt/invalid hex files ──
    TEST("load completely invalid hex data");
    err = vioavr_load_hex_data(h4, "This is not hex!", 16);
    CHECK(err == VIOAVR_ERR_LOAD_FAILED, "invalid string rejected");

    TEST("load hex with wrong checksum");
    err = vioavr_load_hex_data(h4,
        ":1000000000000000000000000000000000000000F0\n"
        ":00000001FF\n", 43);
    CHECK(err == VIOAVR_ERR_LOAD_FAILED, "bad checksum rejected");

    // ── Error inspection without error ──
    TEST("get_last_error when no error");
    vioavr_load_hex_data(h4, "nope", 4); // triggers error
    VioAvrError ec = vioavr_get_last_error_code(h4);
    CHECK(ec != VIOAVR_OK, "error code set after failure");
    const char* msg = vioavr_get_last_error(h4);
    CHECK(msg != NULL && strlen(msg) > 0, "error message non-empty");

    // Clear error by doing something successful (use inline valid hex to avoid CI path dependency)
    {
        const char* valid_hex =
            ":02000000017E7F\n"
            ":00000001FF\n";
        err = vioavr_load_hex_data(h4, valid_hex, strlen(valid_hex));
    }
    CHECK(err == VIOAVR_OK, "load valid minimal hex");
    ec = vioavr_get_last_error_code(h4);
    CHECK(ec == VIOAVR_OK, "error cleared after successful operation");

    vioavr_destroy(h4);

    // ── Multiple create/destroy ──
    TEST("100 create/destroy cycles");
    for (int i = 0; i < 100; i++) {
        VioSpiceHandle h6 = vioavr_create("atmega328p");
        if (!h6) { CHECK(0, "create failed"); break; }
        vioavr_destroy(h6);
    }
    CHECK(1, "100 cycles no crash");

    // ── Error on non-existent file ──
    TEST("load non-existent file");
    VioSpiceHandle h7 = vioavr_create("atmega328p");
    err = vioavr_load_hex(h7, "/nonexistent/path/file.hex");
    CHECK(err == VIOAVR_ERR_LOAD_FAILED, "non-existent file rejected");
    ec = vioavr_get_last_error_code(h7);
    CHECK(ec == VIOAVR_ERR_LOAD_FAILED, "error code set");
    msg = vioavr_get_last_error(h7);
    CHECK(msg != NULL && strstr(msg, "Failed to load") != NULL, "error message mentions load failure");
    vioavr_destroy(h7);

    // ── Add pin mapping with NULL port ──
    TEST("add_pin_mapping(NULL, ...)");
    VioSpiceHandle h8 = vioavr_create("atmega328p");
    vioavr_add_pin_mapping(h8, NULL, 0, 100); // should not crash
    CHECK(1, "no crash on NULL port name");

    // ── HC-05 without enabling ──
    TEST("HC-05 without enable");
    VioSpiceHandle h9 = vioavr_create("atmega328p");
    (void)vioavr_hc05_has_tx_byte(h9);
    (void)vioavr_hc05_read_tx_byte(h9);
    vioavr_hc05_inject_data(h9, NULL, 0);
    CHECK(1, "no crash on HC-05 without enable");
    vioavr_destroy(h9);

    // ── Enabling JIT without VIOAVR_HAVE_JIT ──
    TEST("enable_jit without JIT support");
    VioSpiceHandle h10 = vioavr_create("atmega328p");
#ifdef VIOAVR_HAVE_JIT
    vioavr_enable_jit(h10, true);
    vioavr_enable_jit(h10, false);
    CHECK(1, "JIT toggle works");
#else
    CHECK(1, "JIT not available - skipped");
#endif
    vioavr_destroy(h10);

    printf("====================================\n");
    printf("Results: %d failures\n", failures);
    return failures > 0 ? 1 : 0;
}
