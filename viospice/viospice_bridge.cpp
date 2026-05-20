extern "C" {
#include "ngspice/cm.h"
}
#include "vioavr/core/bridge_shm_client.hpp"
#include <cstdio>
#include <cstdlib>

using namespace vioavr::core;

extern "C" void cm_d_vioavr(Mif_Private_t *mif_private) {
    BridgeShmClient* bridge = nullptr;
    double vref = 5.0;
    
    if (mif_private->circuit.init) {
        const char* mcu_instance = mif_private->param[1]->element[0].svalue;
        bridge = new BridgeShmClient(mcu_instance);
        
        if (!bridge->connect()) {
            fprintf(stderr, "VioAVR Block Error: Could not connect to SHM Bridge. "
                    "Make sure 'vioavr-daemon' is running.\n");
            delete bridge;
            return;
        }
        
        mif_private->inst_var[0]->element[0].pvalue = (void*)bridge;
        
        double freq = mif_private->param[2]->element[0].rvalue;
        bridge->set_frequency(freq);
        const char* hex_file = mif_private->param[0]->element[0].svalue;
        bridge->load_hex(hex_file);
        bridge->reset();
        
        int quantum_cycles = mif_private->param[4]->element[0].ivalue;
        double event_step = (double)quantum_cycles / freq;
        cm_event_queue(mif_private->circuit.time + event_step);
        return;
    }
    
    if (mif_private->num_inst_var > 0 && mif_private->inst_var[0]) {
        bridge = (BridgeShmClient *)mif_private->inst_var[0]->element[0].pvalue;
    }
    // fprintf(stderr, "[VioSpice step] num_inst_var=%d, inst_var=%p, bridge=%p, connected=%d\n", 
    //        mif_private->num_inst_var, (void*)mif_private->inst_var, (void*)bridge, bridge ? (bridge->is_connected() ? 1 : 0) : 0);
    
    if (!bridge || !bridge->is_connected()) return;
    
    vref = mif_private->param[3]->element[0].rvalue;
    
    uint32_t digital_vec_size = mif_private->conn[0]->size;
    for (uint32_t i = 0; i < digital_vec_size; i++) {
        Mif_Port_Data_t *port_data = mif_private->conn[0]->port[i];
        if (port_data->type == MIF_DIGITAL) {
            Digital_t *digital_val = (Digital_t *)(port_data->input.pvalue);
            if (digital_val) {
                bridge->set_digital_input(i, digital_val->state == ONE);
            }
        }
    }
    
    uint32_t analog_vec_size = mif_private->conn[1]->size;
    for (uint32_t i = 0; i < analog_vec_size; i++) {
        Mif_Port_Data_t *port_data = mif_private->conn[1]->port[i];
        float norm_v = (float)(port_data->input.rvalue / vref);
        bridge->set_analog_input(i, norm_v);
    }
    
    double freq = mif_private->param[2]->element[0].rvalue;
    uint64_t target_cycles = (uint64_t)(mif_private->circuit.time * freq);
    uint64_t last_cycles = bridge->get_last_target_cycles();
    
    if (target_cycles > last_cycles) {
        uint64_t delta_cycles = target_cycles - last_cycles;
        bridge->step_cycles(delta_cycles);
        bridge->set_last_target_cycles(target_cycles);
    }
    
    for (uint32_t i = 0; i < digital_vec_size; i++) {
        Mif_Port_Data_t *port_data = mif_private->conn[0]->port[i];
        bool level = bridge->get_digital_output(i);
        // fprintf(stderr, "[VioSpice Output] i=%u, level=%d\n", i, level ? 1 : 0);
        
        Digital_t *digital_val = (Digital_t *)(port_data->output.pvalue);
        if (digital_val) {
            Digital_State_t new_state = level ? ONE : ZERO;
            if (digital_val->state != new_state || mif_private->circuit.init) {
                digital_val->state = new_state;
                digital_val->strength = STRONG;
                port_data->changed = MIF_TRUE;
            }
        }
    }
    
    if (mif_private->num_conn > 2) {
        uint32_t dac_vec_size = mif_private->conn[2]->size;
        for (uint32_t i = 0; i < dac_vec_size; i++) {
            Mif_Port_Data_t *port_data = mif_private->conn[2]->port[i];
            double voltage = bridge->get_analog_output(i);
            port_data->output.rvalue = voltage * vref;
        }
     }
     
     int quantum_cycles = mif_private->param[4]->element[0].ivalue;
     double event_step = (double)quantum_cycles / freq;
     cm_event_queue(mif_private->circuit.time + event_step);
 }
