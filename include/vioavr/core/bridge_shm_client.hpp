#pragma once

#include "vioavr/core/bridge_shm.hpp"
#include <string>
#include <memory>

namespace vioavr::core {

class BridgeShmClient {
public:
    BridgeShmClient(std::string instance_name = "default");
    ~BridgeShmClient();

    // Connection
    bool connect();
    void disconnect();
    bool is_connected() const { return shm_ != nullptr; }
    VioBridgeShm* shm() noexcept { return shm_; }

    // Control
    void reset();
    void step(uint64_t cycles);
    void step(double duration);
    void set_frequency(double hz);
    void wait_step_done();

    // Data Transfer
    void set_digital_input(uint8_t pin, bool high);
    bool get_digital_output(uint8_t pin) const;
    void set_analog_input(uint8_t channel, float voltage);
    float get_analog_output(uint8_t channel) const;
    
    // Thresholds
    void set_thresholds(uint8_t pin, float vih, float vil);

    // Snapshot
    AvrCpuState get_cpu_state() const;

private:
    std::string shm_name_;
    int shm_fd_ {-1};
    VioBridgeShm* shm_ {nullptr};
};

} // namespace vioavr::core
