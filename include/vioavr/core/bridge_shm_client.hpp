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

    bool get_digital_output(uint32_t pin_index) const;
    double get_analog_output(uint8_t channel) const;

    // Telemetry access
    uint64_t get_total_cycles() const;
    uint8_t get_core_state() const;
    uint16_t get_current_instruction() const;

private:
    std::string shm_name_;
    int shm_fd_ {-1};
    VioBridgeShm* shm_ {nullptr};

    void wait_step_done();
};

} // namespace vioavr::core
