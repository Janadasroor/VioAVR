extern "C" {
#include "ngspice/cm.h"
}
#include "vioavr/core/bridge_shm_client.hpp"
#include <cstdio>
#include <cstdlib>

using namespace vioavr::core;

static const char* get_string_param(const Mif_Private_t* mp, int idx, const char* fallback) {
    if (mp->num_param > idx && mp->param[idx] && mp->param[idx]->element &&
        mp->param[idx]->element[0].svalue && mp->param[idx]->element[0].svalue[0]) {
        return mp->param[idx]->element[0].svalue;
    }
    return fallback;
}

static double get_real_param(const Mif_Private_t* mp, int idx, double fallback) {
    if (mp->num_param > idx && mp->param[idx] && mp->param[idx]->element) {
        return mp->param[idx]->element[0].rvalue;
    }
    return fallback;
}

static int get_int_param(const Mif_Private_t* mp, int idx, int fallback) {
    if (mp->num_param > idx && mp->param[idx] && mp->param[idx]->element) {
        return mp->param[idx]->element[0].ivalue;
    }
    return fallback;
}

extern "C" void cm_d_vioavr(Mif_Private_t *mif_private) {
    BridgeShmClient* bridge = nullptr;
    
    // Parameter indices (matches ifspec.ifs order):
    //   0 = hex_file, 1 = instance, 2 = mcu_type,
    //   3 = frequency, 4 = vref, 5 = quantum_cycles
    enum { P_HEX_FILE = 0, P_INSTANCE, P_MCU_TYPE,
           P_FREQ, P_VREF, P_QUANTUM };
    
    if (mif_private->circuit.init) {
        // Instance name: use explicit 'instance' param, fall back to mcu_type
        const char* mcu_type = get_string_param(mif_private, P_MCU_TYPE, "atmega328");
        const char* instance = get_string_param(mif_private, P_INSTANCE, mcu_type);
        
        bridge = new BridgeShmClient(instance);
        
        if (!bridge->connect()) {
            fprintf(stderr, "VioAVR Block Error: Could not connect to SHM Bridge "
                    "\"%s\". Make sure 'vioavr-bridge-daemon --instance %s' is running.\n",
                    instance, instance);
            delete bridge;
            return;
        }
        
        if (mif_private->num_inst_var > 0 && mif_private->inst_var && 
            mif_private->inst_var[0] && mif_private->inst_var[0]->element) {
            mif_private->inst_var[0]->element[0].pvalue = (void*)bridge;
        }
        
        double freq = get_real_param(mif_private, P_FREQ, 16e6);
        bridge->set_frequency(freq);
        
        const char* hex_file = get_string_param(mif_private, P_HEX_FILE, "firmware.hex");
        bridge->load_hex(hex_file);
        bridge->reset();
        
        int quantum_cycles = get_int_param(mif_private, P_QUANTUM, 1000);
        double event_step = (double)quantum_cycles / freq;
        cm_event_queue(mif_private->circuit.time + event_step);
        return;
    }
    
    if (mif_private->num_inst_var > 0 && mif_private->inst_var && 
        mif_private->inst_var[0] && mif_private->inst_var[0]->element) {
        bridge = (BridgeShmClient *)mif_private->inst_var[0]->element[0].pvalue;
    }
    
    if (!bridge || !bridge->is_connected()) return;
    
    double vref = get_real_param(mif_private, P_VREF, 5.0);
    double freq = get_real_param(mif_private, P_FREQ, 16e6);
    
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
     
     int quantum_cycles = get_int_param(mif_private, P_QUANTUM, 1000);
     double event_step = (double)quantum_cycles / freq;
     cm_event_queue(mif_private->circuit.time + event_step);
 }
