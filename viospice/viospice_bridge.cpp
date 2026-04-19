#include "ngspice/cm.h"
#include "vioavr/core/bridge_shm_client.hpp"
#include <cstdio>
#include <cstdlib>

using namespace vioavr::core;

extern "C" void cm_d_vioavr(Mif_Private_t *mif_private) {
    BridgeShmClient* bridge;
    double vref = 5.0;
    
    /* Initialization */
    if (mif_private->circuit.init) {
        mif_private->inst_var = (Mif_Inst_Var_Data_t **)malloc(sizeof(void*));
        
        const char* mcu_instance = mif_private->param[1]->element[0].svalue;
        bridge = new BridgeShmClient(mcu_instance);
        
        if (!bridge->connect()) {
            char msg[512];
            snprintf(msg, sizeof(msg), "VioAVR Block '%s' Error: Could not connect to SHM Bridge '/vioavr_shm_%s'. "
                     "Make sure 'vioavr-daemon --mcu %s' is running.", 
                     mif_private->instance_name, mcu_instance, mcu_instance);
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

    /* Get vref from parameters */
    vref = mif_private->param[3]->element[0].rvalue;

    /* Simulation Step */
    
    /* 1. Collect inputs from SPICE */
    /* Digital Ports (avr_pins) */
    uint32_t digital_vec_size = mif_private->conn[0]->size;
    for (uint32_t i = 0; i < digital_vec_size; i++) {
        Mif_Port_Data_t *port_data = mif_private->conn[0]->port[i];
        if (port_data->input_type == MIF_DIGITAL) {
            Digital_t *digital_val = (Digital_t *)(port_data->input.pvalue);
            if (digital_val) {
                bridge->set_digital_input(i, digital_val->state == ONE);
            }
        }
    }

    /* Analog Ports (avr_analog_pins) */
    uint32_t analog_vec_size = mif_private->conn[1]->size;
    for (uint32_t i = 0; i < analog_vec_size; i++) {
        Mif_Port_Data_t *port_data = mif_private->conn[1]->port[i];
        // Normalize voltage relative to vref for the internal ADC
        float norm_v = (float)(port_data->input.rvalue / vref);
        bridge->set_analog_input(i, norm_v);
    }

    /* 2. Run Bridge */
    double delta_t = mif_private->circuit.time - mif_private->circuit.t[1];
    if (delta_t > 1e-15) { // 1fs threshold
        bridge->step(delta_t);
    }

    /* 3. Deliver Outputs back to SPICE */
    /* Digital Outputs */
    for (uint32_t i = 0; i < digital_vec_size; i++) {
        Mif_Port_Data_t *port_data = mif_private->conn[0]->port[i];
        bool level = bridge->get_digital_output(i);
        
        Digital_t *digital_val = (Digital_t *)(port_data->output.pvalue);
        if (digital_val) {
            Digital_State_t state = level ? ONE : ZERO;
            if (digital_val->state != state) {
                digital_val->state = state;
                digital_val->strength = STRONG;
                port_data->changed = MIF_TRUE;
            }
        }
    }

    /* Analog Outputs (DAC) */
    if (mif_private->conn_size > 2) {
        uint32_t dac_vec_size = mif_private->conn[2]->size;
        for (uint32_t i = 0; i < dac_vec_size; i++) {
            Mif_Port_Data_t *port_data = mif_private->conn[2]->port[i];
            double voltage = bridge->get_analog_output(i);
            port_data->output.rvalue = voltage * vref;
        }
    }
    
    /* Cleanup */
    if (mif_private->circuit.cleanup) {
        if (bridge) {
            bridge->disconnect();
            delete bridge;
        }
        if (mif_private->inst_var) {
            free(mif_private->inst_var);
            mif_private->inst_var = nullptr;
        }
    }
}
