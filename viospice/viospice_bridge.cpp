#include "vioavr/core/bridge_shm_client.hpp"
#include <ngspice/cm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace vioavr::core;

/* XSPICE Entry Point */
extern "C" void cm_d_vioavr(Mif_Private_t *mif_private) {
    BridgeShmClient* bridge;
    
    /* Initialization */
    if (mif_private->circuit.init) {
        /* Hack: Store the bridge pointer in inst_var */
        mif_private->inst_var = (Mif_Inst_Var_Data_t **)malloc(sizeof(void*));
        
        /* Get Parameters */
        // hex_file [0], mcu_type [1], frequency [2]
        const char* mcu_instance = mif_private->param[1]->element[0].svalue;

        bridge = new BridgeShmClient(mcu_instance);
        if (!bridge->connect()) {
            char msg[256];
            snprintf(msg, sizeof(msg), "VioAVR: Failed to connect to SHM Bridge: %s. Is the daemon running?", mcu_instance);
            cm_message_send(msg);
            delete bridge;
            *((void **)mif_private->inst_var) = nullptr;
            return;
        }
        
        *((void **)mif_private->inst_var) = bridge;
        
        double freq = mif_private->param[2]->element[0].rvalue;
        bridge->set_frequency(freq);
        bridge->reset();
    } else {
        bridge = *((BridgeShmClient **)mif_private->inst_var);
    }

    if (!bridge || !bridge->is_connected()) return;

    /* Simulation Step */
    
    /* 1. Propagate inputs from SPICE nodes to Bridge */
    uint32_t digital_port_size = mif_private->conn[0]->size;
    for (uint32_t i = 0; i < digital_port_size; i++) {
        Mif_Port_Data_t *port_data = mif_private->conn[0]->port[i];
        Digital_t *digital_val = (Digital_t *)(port_data->input.pvalue);
        if (digital_val) {
            bridge->set_digital_input(i, digital_val->state == ONE);
        }
    }

    // 1.b Propagate Analog Voltages (if connected)
    uint32_t analog_port_size = mif_private->conn[1]->size;
    for (uint32_t i = 0; i < analog_port_size; i++) {
        Mif_Port_Data_t *port_data = mif_private->conn[1]->port[i];
        // Normalized 0.0 to 1.0 logic depends on SPICE reference, 
        // but here we pass raw voltage as the Server handles thresholds.
        bridge->set_analog_input(i, (float)port_data->input.rvalue);
    }

    /* 2. Step VioAVR Daemon */
    double delta_t = mif_private->circuit.time - mif_private->circuit.t[1];
    if (delta_t > 0) {
        bridge->step(delta_t);
        // Note: wait_step_done is called inside step()
    }

    /* 3. Collect outputs from Bridge back to SPICE */
    for (uint32_t i = 0; i < digital_port_size; i++) {
        Mif_Port_Data_t *port_data = mif_private->conn[0]->port[i];
        bool level = bridge->get_digital_output(i);
        
        Digital_t *digital_val = (Digital_t *)(port_data->output.pvalue);
        if (digital_val) {
            digital_val->state = level ? ONE : ZERO;
            digital_val->strength = STRONG;
            port_data->changed = MIF_TRUE;
        }
    }
}
