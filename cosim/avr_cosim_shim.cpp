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

static const char* PORT_NAMES[3] = {"PORTB", "PORTC", "PORTD"};

struct CosimInst {
    VioSpiceHandle avr;
    double         last_vtime;
    double         accumulated_time;
    Digital_t      prev_out[PIN_COUNT];
};

static void shim_step(struct co_info* info)
{
    struct CosimInst* inst = (struct CosimInst*)info->handle;
    double delta = info->vtime - inst->last_vtime;

    if (delta > 0) {
        inst->last_vtime = info->vtime;
        inst->accumulated_time += delta;

        // Batch small time steps: only step the AVR when enough time accumulates
        // to avoid the ISR overshoot problem (budget too small for interrupt entry).
        // At 16MHz, 1µs = 16 cycles, enough for ISR entry (4) + instructions + return.
        const double min_step = 1.0e-6; // 1 microsecond
        if (inst->accumulated_time >= min_step) {
            vioavr_step_duration(inst->avr, inst->accumulated_time);
            inst->accumulated_time = 0.0;

            VioAvrPinChange changes[PIN_COUNT];
            int n = vioavr_consume_pin_changes(inst->avr, changes, PIN_COUNT);
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

/* Parse combined sim_args string in format "mcu_type,hex_path"
 * or fall back to defaults "atmega328p,firmware.hex".
 */
static void parse_sim_args(struct co_info* info, const char** mcu, const char** hex)
{
    *mcu = "atmega328p";
    *hex = "firmware.hex";
    if (info->sim_argc > 0 && info->sim_argv[0]) {
        const char* s = info->sim_argv[0];
        const char* comma = strchr(s, ',');
        if (comma) {
            size_t mcu_len = (size_t)(comma - s);
            char* mcu_buf = (char*)malloc(mcu_len + 1);
            if (mcu_buf) {
                memcpy(mcu_buf, s, mcu_len);
                mcu_buf[mcu_len] = '\0';
                *mcu = mcu_buf;
            }
            *hex = comma + 1;
        } else {
            *mcu = s;
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
    parse_sim_args(info, &mcu, &hex);

    struct CosimInst* inst = (struct CosimInst*)calloc(1, sizeof(struct CosimInst));
    if (!inst) return;

    inst->avr = vioavr_create(mcu);
    if (!inst->avr) {
        free(inst);
        return;
    }

    for (int p = 0; p < 3; p++)
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

    /* All init succeeded — publish the interface. */
    info->handle     = inst;
    info->in_count   = PIN_COUNT;
    info->out_count  = PIN_COUNT;
    info->step       = shim_step;
    info->in_fn      = shim_input;
    info->cleanup    = shim_cleanup;
}
