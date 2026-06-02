#include "ngspice/cm.h"
extern void cm_avr_adc_bridge(Mif_Private_t *);
extern void cm_avr_dac_bridge(Mif_Private_t *);
extern void cm_avr_vcc_bridge(Mif_Private_t *);

#include <dlfcn.h>
#include <string.h>

/*
 * cm_avr_adc_bridge — XSPICE analog code model
 *
 * Reads analog voltage from circuit node and injects it into the
 * VioAVR analog signal bank for processing by the emulated ADC.
 *
 * Ports:
 *   in  — analog voltage input (scalar)
 *   out — unity-gain voltage output (mirrors input)
 *
 * Parameters:
 *   channel — analog signal bank channel (default 0)
 *
 * Lazy-initializes via dlsym to find the active VioAVR handle
 * exported by libavr_cosim.so (loaded by d_cosim).
 *
 * Netlist usage:
 *   A_avr_adc [adc_node 0] [dummy 0] avr_adc_bridge
 *   .model avr_adc_bridge avr_adc_bridge(channel=0)
 */

/* Function pointer cache (shared across instances) */
static void* (*g_get_handle)(void) = NULL;
static void (*g_set_voltage)(void*, int, double) = NULL;
static void* g_avr_handle = NULL;
static int   g_initialized = 0;

static void try_resolve_symbols(void)
{
    if (g_get_handle) return;

    /* Resolve from any loaded library (RTLD_DEFAULT).
     * libavr_cosim.so must be loaded with RTLD_GLOBAL for this to work. */
    g_get_handle = (void* (*)(void))dlsym(RTLD_DEFAULT,
                                          "vioavr_cosim_get_active_handle");
    g_set_voltage = (void (*)(void*, int, double))dlsym(
        RTLD_DEFAULT, "vioavr_set_external_voltage");

    if (g_get_handle) {
        g_avr_handle = g_get_handle();
        if (g_avr_handle) {
            g_initialized = 1;
        }
    }
}

void cm_avr_adc_bridge(Mif_Private_t *mif_private)
{
    double voltage;
    int channel;

    /* On first call, try to resolve symbols from the loaded cosim shim. */
    if (!g_initialized) {
        try_resolve_symbols();
    }

    /* Read the analog input voltage */
    voltage = mif_private->conn[0]->port[0]->input.rvalue;

    /* Read channel parameter */
    channel = (int)mif_private->param[0]->element[0].rvalue;
    if (channel < 0)   channel = 0;
    if (channel > 127) channel = 127;

    /* Inject into AVR analog signal bank (if handle is alive) */
    if (g_initialized && g_avr_handle && g_set_voltage) {
        /* Attempt to get fresh handle each time (handle may change
         * across simulation restarts within the same ngspice session) */
        void* current_handle = g_get_handle ? g_get_handle() : NULL;
        if (current_handle) {
            g_avr_handle = current_handle;
            g_set_voltage(g_avr_handle, channel, voltage);
        }
    }

    /* Output: unity gain (mirror input for Newton coupling) */
    mif_private->conn[1]->port[0]->output.rvalue = voltage;
    mif_private->conn[1]->port[0]->partial[0].port[0] = 1.0;
}

/* ------------------------------------------------------------------ */
/* cm_avr_dac_bridge — XSPICE analog code model                       */
/*                                                                    */
/* Reads DAC voltage from VioAVR analog outputs and presents it       */
/* as an analog node voltage to ngspice.                              */
/*                                                                    */
/* Ports:                                                             */
/*   out — analog voltage output (scalar)                             */
/*                                                                    */
/* Parameters:                                                        */
/*   channel — index into analog outputs vector (default 0)           */
/*                                                                    */
/* Lazy-initializes via dlsym to find the active VioAVR handle        */
/* exported by libavr_cosim.so (loaded by d_cosim).                   */
/*                                                                    */
/* Netlist usage:                                                     */
/*   A_avr_dac [dac_node 0] avr_dac_bridge                            */
/*   .model avr_dac_bridge avr_dac_bridge(channel=0)                  */
/* ------------------------------------------------------------------ */

/* Function pointer cache for get_analog_outputs */
static int (*g_get_analog_outputs)(void*, double*, int) = NULL;
static int g_dac_initialized = 0;

static void try_resolve_dac_symbols(void)
{
    if (g_get_analog_outputs) return;

    if (!g_get_handle) {
        g_get_handle = (void* (*)(void))dlsym(RTLD_DEFAULT,
                                              "vioavr_cosim_get_active_handle");
    }
    g_get_analog_outputs = (int (*)(void*, double*, int))dlsym(
        RTLD_DEFAULT, "vioavr_get_analog_outputs");

    if (g_get_handle && g_get_analog_outputs) {
        void* h = g_get_handle();
        if (h) {
            g_avr_handle = h;
            g_initialized = 1;
            g_dac_initialized = 1;
        }
    }
}

void cm_avr_dac_bridge(Mif_Private_t *mif_private)
{
    int channel;
    double outputs[4];

    if (!g_dac_initialized) {
        try_resolve_dac_symbols();
    }

    channel = (int)mif_private->param[0]->element[0].rvalue;
    if (channel < 0)   channel = 0;
    if (channel > 3)   channel = 3;

    double vout = 0.0;
    if (g_dac_initialized && g_avr_handle && g_get_analog_outputs) {
        void* current_handle = g_get_handle ? g_get_handle() : NULL;
        if (current_handle) {
            g_avr_handle = current_handle;
            int n = g_get_analog_outputs(g_avr_handle, outputs, 4);
            if (channel < n) {
                vout = outputs[channel];
            }
        }
    }

    /* Input: dummy read (grounded) */
    double vin = mif_private->conn[0]->port[0]->input.rvalue;

    /* Output: DAC voltage */
    mif_private->conn[1]->port[0]->output.rvalue = vout;
    mif_private->conn[1]->port[0]->partial[0].port[0] = 1.0;
}

/* ------------------------------------------------------------------ */
/* cm_avr_vcc_bridge — Dynamic VCC tracking for co-simulation          */
/*                                                                     */
/* Reads VCC voltage from an analog circuit node and injects it into   */
/* the AVR core via vioavr_set_operating_voltage(), which propagates   */
/* to all VDD-dependent peripherals (ADC, AC, DAC, ZCD, Opamp).       */
/*                                                                     */
/* Ports:                                                             */
/*   vcc — analog voltage input (VCC node)                            */
/*   out — unity-gain voltage output (mirrors input for Newton)       */
/*                                                                     */
/* Netlist usage:                                                     */
/*   A_avr_vcc [vcc_node 0] [dummy 0] avr_vcc_bridge                  */
/*   .model avr_vcc_bridge avr_vcc_bridge                             */
/* ------------------------------------------------------------------ */

static void (*g_set_operating_voltage)(void*, double) = NULL;
static int g_vcc_initialized = 0;

static void try_resolve_vcc_symbols(void)
{
    if (g_set_operating_voltage) return;

    if (!g_get_handle) {
        g_get_handle = (void* (*)(void))dlsym(RTLD_DEFAULT,
                                              "vioavr_cosim_get_active_handle");
    }
    g_set_operating_voltage = (void (*)(void*, double))dlsym(
        RTLD_DEFAULT, "vioavr_set_operating_voltage");

    if (g_get_handle && g_set_operating_voltage) {
        void* h = g_get_handle();
        if (h) {
            g_avr_handle = h;
            g_initialized = 1;
            g_vcc_initialized = 1;
        }
    }
}

void cm_avr_vcc_bridge(Mif_Private_t *mif_private)
{
    if (!g_vcc_initialized) {
        try_resolve_vcc_symbols();
    }

    /* Read VCC voltage from analog input port */
    double vcc = mif_private->conn[0]->port[0]->input.rvalue;

    /* Inject into AVR core via set_operating_voltage (dlsym'd) */
    if (g_vcc_initialized && g_avr_handle && g_set_operating_voltage) {
        void* current_handle = g_get_handle ? g_get_handle() : NULL;
        if (current_handle) {
            g_avr_handle = current_handle;
            g_set_operating_voltage(g_avr_handle, vcc);
        }
    }

    /* Output: unity gain mirror for Newton coupling */
    mif_private->conn[1]->port[0]->output.rvalue = vcc;
    mif_private->conn[1]->port[0]->partial[0].port[0] = 1.0;
}
