#pragma once

#include "vioavr/core/bridge_shm.hpp"
#include "vioavr/core/viospice.hpp"
#include <string>
#include <thread>
#include <atomic>

namespace vioavr::core {

class BridgeShmServer {
public:
    BridgeShmServer(const DeviceDescriptor& device, std::string instance_name = "default");
    ~BridgeShmServer();

    void run_loop();
    void stop();

private:
    std::string shm_name_;
    int shm_fd_ {-1};
    VioBridgeShm* shm_ {nullptr};
    
    VioSpice avr_;
    std::atomic<bool> running_ {false};

    void handle_step();
    void handle_command();
    void update_state_to_shm();
};

} // namespace vioavr::core
