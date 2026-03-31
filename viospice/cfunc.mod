/* Implementation of VioAVR XSPICE Code Model */

#include "vioavr/core/viospice_c.h"
#include <stdio.h>
#include <stdlib.h>

void cm_d_vioavr(ARGS) {
    VioSpiceHandle avr;
    
    /* Initialization */
    if (INIT) {
        /* Allocate private storage for the simulator instance */
        cm_event_alloc(0, sizeof(VioSpiceHandle));
        avr = vioavr_create(PARAM(mcu_type));
        
        if (!avr) {
            cm_message_send("VioAVR: Failed to create MCU instance.");
            return;
        }
        
        *((VioSpiceHandle *) cm_event_get_ptr(0, 0)) = avr;
        
        /* Load firmware */
        if (!vioavr_load_hex(avr, PARAM(hex_file))) {
            char msg[256];
            sprintf(msg, "VioAVR: Failed to load HEX file: %s", PARAM(hex_file));
            cm_message_send(msg);
        }
        
        vioavr_set_quantum(avr, (uint64_t)PARAM(quantum_cycles));
        
        /* Map ports to XSPICE vector indices 
           0-7   -> PORTB
           8-15  -> PORTC
           16-23 -> PORTD
        */
        for (int i = 0; i < 8; i++) vioavr_add_pin_mapping(avr, "PORTB", i, i);
        for (int i = 0; i < 8; i++) vioavr_add_pin_mapping(avr, "PORTC", i, 8 + i);
        for (int i = 0; i < 8; i++) vioavr_add_pin_mapping(avr, "PORTD", i, 16 + i);
        
        vioavr_reset(avr);
    } else {
        avr = *((VioSpiceHandle *) cm_event_get_ptr(0, 0));
    }

    /* Simulation Step */
    
    /* 1. Propagate inputs from SPICE to AVR */
    for (int i = 0; i < PORT_SIZE(avr_pins); i++) {
        if (LOAD(avr_pins[i])) {
            Digital_State_t state = INPUT_STATE(avr_pins[i]);
            VioAvrPinLevel level = (state == HIGH) ? VIOAVR_LEVEL_HIGH : VIOAVR_LEVEL_LOW;
            vioavr_set_external_pin(avr, i, level);
        }
    }

    /* 2. Run AVR for the time delta */
    /* T(1) is the current time, T(2) is the previous time */
    double delta_t = T(1) - T(2);
    if (delta_t > 0) {
        vioavr_step_duration(avr, delta_t);
    }

    /* 3. Collect outputs from AVR and push to SPICE */
    VioAvrPinChange changes[32];
    int count = vioavr_consume_pin_changes(avr, changes, 32);
    
    for (int i = 0; i < count; i++) {
        int idx = changes[i].external_id;
        if (idx < PORT_SIZE(avr_pins)) {
            OUTPUT_STATE(avr_pins[idx]) = (changes[i].level == VIOAVR_LEVEL_HIGH) ? HIGH : LOW;
            OUTPUT_STRENGTH(avr_pins[idx]) = STRONG;
        }
    }
}
