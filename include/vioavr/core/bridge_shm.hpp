#pragma once

#include <cstdint>
#include <atomic>
#include <array>
#include <semaphore.h>

namespace vioavr::core {

static constexpr uint32_t VIOAVR_BRIDGE_MAGIC = 0x56494F38; // "VIO8"
static constexpr uint32_t VIOAVR_BRIDGE_VERSION = 1;

enum class BridgeStatus : uint32_t {
    Idle = 0,
    Running = 1,
    Error = 2,
    Resetting = 3
};

struct AvrCpuState {
    uint16_t pc;
    uint16_t sp;
    uint8_t sreg;
    uint8_t padding;
    uint8_t gprs[32];
};

/**
 * @brief Layout of the Shared Memory Bridge.
 * This structure must be pod-compatible and layout-stable.
 */
struct VioBridgeShm {
    uint32_t magic;
    uint32_t version;
    
    // In-memory semaphores (requires pshared=1)
    sem_t sem_req; // Simulator -> Emulator
    sem_t sem_ack; // Emulator -> Simulator

    std::atomic<uint64_t> sync_counter;
    std::atomic<BridgeStatus> status;
    
    // Command Interface
    std::atomic<uint32_t> command; // Command bitmask
    uint32_t sync_counter_val;     // Incremented every sync step
    double clock_frequency;     // Target MCU frequency
    double request_duration;   // Simulation time to advance (seconds)
    uint64_t request_cycles;   // Cycles to step (if request_duration=0)
    char command_arg[256];

    // Digital I/O (128 pins max)
    // Client writes to inputs, Server writes to outputs
    std::array<uint8_t, 128> digital_inputs;
    std::array<uint8_t, 128> digital_outputs;

    // Analog Mapping (Schmitt Trigger / Thresholds)
    // Client can write voltage to analog_inputs[pin]. 
    // Server will compare to thresholds to update digital_inputs[pin].
    std::array<float, 128> analog_inputs;
    std::array<float, 128> vih_threshold;
    std::array<float, 128> vil_threshold;
    
    // Dedicated Analog Outputs (DAC)
    std::array<float, 32> analog_outputs;

    // CPU Insight
    AvrCpuState cpu_state;
    
    // Reserved for future expansion
    uint8_t reserved[1024]; 
};

} // namespace vioavr::core
