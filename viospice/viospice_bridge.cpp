/* C++ Bridge for XSPICE without CMPP dependency */
#include "vioavr/core/viospice_c.h"
#include <ngspice/cm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* XSPICE Entry Point */
extern "C" void cm_d_vioavr(Mif_Private_t *mif_private) {
    VioSpiceHandle avr;
    
    /* Initialization */
    if (mif_private->circuit.init) {
        /* Allocate private storage */
        mif_private->inst_var = (Mif_Inst_Var_t *)malloc(sizeof(VioSpiceHandle));
        
        /* Get Parameters */
        const char* mcu_type = mif_private->param[0]->element[0].rvalue.svalue;
        const char* hex_file = mif_private->param[1]->element[0].rvalue.svalue;
        double quantum = mif_private->param[2]->element[0].rvalue.rvalue;

        avr = vioavr_create(mcu_type);
        if (!avr) {
            cm_message_send((char*)"VioAVR: Failed to create MCU instance.");
            return;
        }
        
        *((VioSpiceHandle *)mif_private->inst_var) = avr;
        
        if (!vioavr_load_hex(avr, hex_file)) {
            char msg[256];
            snprintf(msg, sizeof(msg), "VioAVR: Failed to load HEX file: %s", hex_file);
            cm_message_send(msg);
        }
        
        vioavr_set_quantum(avr, (uint64_t)quantum);
        
        /* Default mappings */
        for (int i = 0; i < 8; i++) vioavr_add_pin_mapping(avr, "PORTB", i, i);
        for (int i = 0; i < 8; i++) vioavr_add_pin_mapping(avr, "PORTC", i, 8 + i);
        for (int i = 0; i < 8; i++) vioavr_add_pin_mapping(avr, "PORTD", i, 16 + i);
        
        vioavr_reset(avr);
    } else {
        avr = *((VioSpiceHandle *)mif_private->inst_var);
    }

    /* Simulation Step */
    
    /* 1. Propagate inputs */
    int port_size = mif_private->conn[0]->size;
    for (int i = 0; i < port_size; i++) {
        Digital_t *pin = (Digital_t *)mif_private->conn[0]->port[i];
        if (pin->changed) {
            VioAvrPinLevel level = (pin->state == HI) ? VIOAVR_LEVEL_HIGH : VIOAVR_LEVEL_LOW;
            vioavr_set_external_pin(avr, i, level);
        }
    }

    /* 2. Step AVR */
    double delta_t = mif_private->circuit.time - mif_private->circuit.t[1];
    if (delta_t > 0) {
        vioavr_step_duration(avr, delta_t);
    }

    /* 3. Collect outputs */
    VioAvrPinChange changes[32];
    int count = vioavr_consume_pin_changes(avr, changes, 32);
    
    for (int i = 0; i < count; i++) {
        int idx = changes[i].external_id;
        if (idx < port_size) {
            Digital_t *pin = (Digital_t *)mif_private->conn[0]->port[idx];
            pin->state = (changes[i].level == VIOAVR_LEVEL_HIGH) ? HI : LO;
            pin->strength = STRONG;
        }
    }
}
