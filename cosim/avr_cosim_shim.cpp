/* avr_cosim_shim.cpp — d_cosim co-simulation shim for VioAVR
 *
 * Shared library loaded by ngspice's d_cosim XSPICE code model.
 * Embeds the VioAVR ISS in-process. No daemon or SHM needed.
 *
 * Netlist usage (24 pins, same nodes for d_in and d_out):
 *   A_AVR [pin0 ... pin23] [pin0 ... pin23] d_cosim_model
 *   .model d_cosim_model d_cosim(
 *     simulation="./cosim/libavr_cosim.so"
 *     sim_args=["atmega328p"]
 *     queue_size=1024 irreversible=1 delay=1e-9)
 *
 * The first bracket is d_in (circuit->AVR), second is d_out (AVR->circuit).
 * List the same 24 nodes in both vectors.
 *
 * Hex file lookup (in order):
 *   1. Second element of comma-separated sim_args (e.g. "atmega328p,firmware.hex")
 *   2. Default: "firmware.hex" relative to ngspice working directory
 *
 * Pin mapping: PORTB[0..7] -> idx 0..7, PORTC[0..7] -> idx 8..15, PORTD[0..7] -> idx 16..23
 */

extern "C" {
#include "ngspice/cmtypes.h"
#include "ngspice/cosim.h"
}
#include "vioavr/core/viospice_c.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PIN_COUNT 24

static const char* PORT_NAMES[4] = {"PORTA", "PORTB", "PORTC", "PORTD"};

struct CosimInst {
    VioSpiceHandle avr;
    double         last_vtime;
    double         accumulated_time;
    Digital_t      prev_out[PIN_COUNT];
};

static void shim_step(struct co_info* info)
{
    struct CosimInst* inst = (struct CosimInst*)info->handle;
    if (!inst) return;
    double delta = info->vtime - inst->last_vtime;

    if (delta > 0) {
        inst->last_vtime = info->vtime;
        inst->accumulated_time += delta;
        const double min_step = 1.0e-6; // 1 microsecond
        if (inst->accumulated_time >= min_step) {
            // Pin change event: read firmware's output changes and send to ngspice
            vioavr_step_duration(inst->avr, inst->accumulated_time);
            inst->accumulated_time = 0.0;

            VioAvrPinChange changes[PIN_COUNT * 2]; // Room for reset + firmware changes
            int n = vioavr_consume_pin_changes(inst->avr, changes, PIN_COUNT * 2);
            for (int i = 0; i < n; i++) {
                unsigned int id = changes[i].external_id;
                if (id >= PIN_COUNT) continue;

                Digital_t val;
                val.state    = (changes[i].level == VIOAVR_LEVEL_HIGH) ? ONE : ZERO;
                val.strength = STRONG;

                if (val.state != inst->prev_out[id].state ||
                    val.strength != inst->prev_out[id].strength) {
                    inst->prev_out[id] = val;
                    (*info->out_fn)(info, id, &val);
                }
            }
        }
    }
}
static void shim_input(struct co_info* info, unsigned int bit, Digital_t* value)
{
    struct CosimInst* inst = (struct CosimInst*)info->handle;
    if (bit < PIN_COUNT) {
        VioAvrPinLevel level = (value->state == ONE)
                               ? VIOAVR_LEVEL_HIGH
                               : VIOAVR_LEVEL_LOW;
        vioavr_set_external_pin(inst->avr, bit, level);
    }
}

static void shim_cleanup(struct co_info* info)
{
    struct CosimInst* inst = (struct CosimInst*)info->handle;
    if (inst) {
        if (inst->avr) vioavr_destroy(inst->avr);
        free(inst);
        info->handle = NULL;
    }
}

/* Safe no-ops used when init fails — d_cosim MUST have non-NULL callbacks. */
static void nop_step(struct co_info* info) { (void)info; }
static void nop_input(struct co_info* info, unsigned int bit, Digital_t* value)
{ (void)info; (void)bit; (void)value; }
static void nop_cleanup(struct co_info* info) { (void)info; }

/* Parse sim_args in any of these formats:
 *   sim_args=["atmega328p"]                    (single string, no comma)
 *   sim_args=["atmega328p,firmware.hex"]       (single string with commas)
 *   sim_args=["atmega328p","firmware.hex"]     (multiple array elements)
 *   sim_args=["atmega328p","firmware.hex","0"] (multiple array elements, jit flag)
 * jit_flag: "0" = disable JIT, "1" or absent = enable JIT (default)
 */
static void parse_sim_args(struct co_info* info, const char** mcu,
                           const char** hex, bool* jit_enabled)
{
    *mcu = "atmega328p";
    *hex = "firmware.hex";
    *jit_enabled = true;
    if (info->sim_argc > 0) {
        // Prefer comma-separated single string (backward compat)
        if (info->sim_argv[0]) {
            const char* s = info->sim_argv[0];
            const char* comma1 = strchr(s, ',');
            if (comma1) {
                size_t mcu_len = (size_t)(comma1 - s);
                char* mcu_buf = (char*)malloc(mcu_len + 1);
                if (mcu_buf) {
                    memcpy(mcu_buf, s, mcu_len);
                    mcu_buf[mcu_len] = '\0';
                    *mcu = mcu_buf;
                }
                const char* rest = comma1 + 1;
                const char* comma2 = strchr(rest, ',');
                if (comma2) {
                    size_t hex_len = (size_t)(comma2 - rest);
                    char* hex_buf = (char*)malloc(hex_len + 1);
                    if (hex_buf) {
                        memcpy(hex_buf, rest, hex_len);
                        hex_buf[hex_len] = '\0';
                        *hex = hex_buf;
                    }
                    const char* jit_str = comma2 + 1;
                    *jit_enabled = (jit_str[0] == '1');
                } else {
                    *hex = rest;
                }
            } else {
                *mcu = s;
            }
        }
        // Override with separate array elements if available
        if (info->sim_argc >= 2 && info->sim_argv[1] && info->sim_argv[1][0]) {
            *hex = info->sim_argv[1];
        }
        if (info->sim_argc >= 3 && info->sim_argv[2]) {
            *jit_enabled = (info->sim_argv[2][0] == '1');
        }
    }
}

void Cosim_setup(struct co_info* info)
{
    /* Set safe defaults — d_cosim MUST have non-NULL callbacks even on failure. */
    info->handle     = NULL;
    info->in_count   = 0;
    info->out_count  = 0;
    info->inout_count = 0;
    info->step       = nop_step;
    info->in_fn      = nop_input;
    info->cleanup    = nop_cleanup;
    info->method     = Normal;

    const char* mcu = NULL;
    const char* hex = NULL;
    bool jit_enabled = true;
    parse_sim_args(info, &mcu, &hex, &jit_enabled);

    struct CosimInst* inst = (struct CosimInst*)calloc(1, sizeof(struct CosimInst));
    if (!inst) return;

    inst->avr = vioavr_create(mcu);
    if (!inst->avr) {
        free(inst);
        return;
    }

    vioavr_enable_jit(inst->avr, jit_enabled);

    for (int p = 0; p < 4; p++)
        for (int b = 0; b < 8; b++)
            vioavr_add_pin_mapping(inst->avr, PORT_NAMES[p], b, p * 8 + b);

    if (!vioavr_load_hex(inst->avr, hex)) {
        vioavr_destroy(inst->avr);
        free(inst);
        return;
    }

    inst->last_vtime = 0.0;
    inst->accumulated_time = 0.0;
    for (int i = 0; i < PIN_COUNT; i++) {
        inst->prev_out[i].state    = ZERO;
        inst->prev_out[i].strength = STRONG;
    }

    vioavr_reset(inst->avr);

    // Drain initial pin changes from reset so subsequent firmware changes aren't lost
    VioAvrPinChange drain[64];
    while (vioavr_consume_pin_changes(inst->avr, drain, 64) > 0) {}

    /* All init succeeded — publish the interface. */
    info->handle     = inst;
    info->in_count   = PIN_COUNT;
    info->out_count  = PIN_COUNT;
    info->step       = shim_step;
    info->in_fn      = shim_input;
    info->cleanup    = shim_cleanup;
}
