#include "ngspice/cm.h"
extern void cm_avr_adc_bridge(Mif_Private_t *);

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
