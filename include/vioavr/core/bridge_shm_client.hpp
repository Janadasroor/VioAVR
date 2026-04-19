#pragma once

#include "vioavr/core/bridge_shm.hpp"
#include <string>
#include <vector>

namespace vioavr::core {

class BridgeShmClient {
public:
    explicit BridgeShmClient(std::string instance_name);
    ~BridgeShmClient();

    bool connect();
    void disconnect();
    bool is_connected() const { return shm_ != nullptr; }

    void reset();
    void step(double delta_time_seconds);
    void step_cycles(uint64_t cycles);

    void set_frequency(double hz);
    void set_digital_input(uint32_t pin_index, bool level);
    void set_analog_input(uint8_t channel, float voltage);
    void set_thresholds(uint32_t pin_index, float vih, float vil);

    bool get_digital_output(uint32_t pin_index) const;
    double get_analog_output(uint8_t channel) const;

    // Direct status control (for shutdown)
    void set_status(BridgeStatus status);
    BridgeStatus get_status() const;

    // Telemetry and State
    uint64_t get_total_cycles() const;
    uint8_t get_core_state() const;
    uint16_t get_current_instruction() const;
    AvrCpuState get_cpu_state() const;

    /** @brief ONLY FOR TESTING: Access digital inputs raw */
    bool test_get_digital_input(uint32_t pin_index) const {
        if (shm_ && pin_index < 128) return shm_->digital_inputs[pin_index] != 0;
        return false;
    }

private:
    std::string shm_name_;
    int shm_fd_ {-1};
    VioBridgeShm* shm_ {nullptr};

    void signal_and_wait();
};

} // namespace vioavr::core
 
