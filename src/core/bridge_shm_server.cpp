#include "vioavr/core/bridge_shm_server.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

namespace vioavr::core {

BridgeShmServer::BridgeShmServer(const DeviceDescriptor& device, std::string instance_name)
    : shm_name_("/vioavr_shm_" + instance_name), avr_(device) {
    
    // 1. Create Shared Memory
    shm_fd_ = shm_open(shm_name_.c_str(), O_CREAT | O_RDWR, 0666);
    if (shm_fd_ == -1) {
        throw std::runtime_error("Failed to create shared memory: " + shm_name_);
    }

    size_t size = sizeof(VioBridgeShm);
    if (ftruncate(shm_fd_, size) == -1) {
        throw std::runtime_error("Failed to size shared memory");
    }

    shm_ = static_cast<VioBridgeShm*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0));
    if (shm_ == MAP_FAILED) {
        throw std::runtime_error("Failed to map shared memory");
    }

    // 2. Initialize Struct (if new)
    shm_->magic = VIOAVR_BRIDGE_MAGIC;
    shm_->version = VIOAVR_BRIDGE_VERSION;
    shm_->status.store(BridgeStatus::Idle);
    shm_->command.store(0);
    shm_->request_cycles = 0;

    shm_->digital_inputs.fill(0);
    shm_->digital_outputs.fill(0);
    shm_->analog_inputs.fill(0.0f);
    shm_->analog_outputs.fill(0.0f);
    
    // Initialize semaphores
    if (sem_init(&shm_->sem_req, 1, 0) == -1) {
        throw std::runtime_error("Failed to init req semaphore");
    }
    if (sem_init(&shm_->sem_ack, 1, 0) == -1) {
        throw std::runtime_error("Failed to init ack semaphore");
    }

    std::cout << "VioAVR Shm Bridge Server started at " << shm_name_ << std::endl;
}

BridgeShmServer::~BridgeShmServer() {
    stop();
    if (shm_) {
        sem_destroy(&shm_->sem_req);
        sem_destroy(&shm_->sem_ack);
        munmap(shm_, sizeof(VioBridgeShm));
    }
    if (shm_fd_ != -1) {
        close(shm_fd_);
        shm_unlink(shm_name_.c_str());
    }
}

void BridgeShmServer::stop() {
    running_ = false;
    // Release any waiting clients or self
    sem_post(&shm_->sem_req);
}

void BridgeShmServer::run_loop() {
    running_ = true;
    shm_->status = BridgeStatus::Running;

    while (running_) {
        // Wait for request from Client (Simulator)
        if (sem_wait(&shm_->sem_req) == -1) {
            if (errno == EINTR) continue;
            break;
        }

        if (!running_) break;

        // Process based on command or step
        uint32_t cmd = shm_->command.exchange(0);
        if (cmd != 0) {
            handle_command();
        } else {
            handle_step();
        }

        // Signal completion
        shm_->sync_counter.fetch_add(1);
        sem_post(&shm_->sem_ack);
    }

    shm_->status = BridgeStatus::Idle;
}

void BridgeShmServer::handle_step() {
    // 1. Sync inputs from SHM to VioSpice
    for (uint32_t i = 0; i < 128; ++i) {
        // Here we need a mapping from "external_id" i to AVR pins.
        // For simplicity, let's assume a default mapping for this POC.
        // In reality, this would use BridgeShm fields to describe the mapping.
        avr_.set_external_pin(i, shm_->digital_inputs[i] ? PinLevel::high : PinLevel::low);
    }
    
    for (uint32_t i = 0; i < 32; ++i) {
        avr_.set_external_voltage(i, shm_->analog_inputs[i]);
    }

    // 2. Step the CPU
    // If request_cycles is 0, use a default quantum
    uint64_t cycles = shm_->request_cycles ? shm_->request_cycles : 1000;
    // VioSpice step_duration or direct cycle step
    // Converting cycles to seconds for step_duration
    double freq = 16000000.0; // Assume 16MHz
    avr_.step_duration(static_cast<double>(cycles) / freq);

    // 3. Sync outputs from VioSpice to SHM
    // We consume changes and update the output array
    auto changes = avr_.consume_pin_changes();
    for (const auto& ch : changes) {
        auto ext_id = avr_.get_external_id(ch.port_name, ch.bit_index);
        if (ext_id && *ext_id < 128) {
            shm_->digital_outputs[*ext_id] = ch.level ? 1 : 0;
        }
    }
    
    // Update CPU State for UI
    update_state_to_shm();
}

void BridgeShmServer::handle_command() {
    // Placeholder for LoadHex, Reset, etc.
    // cmd 1 = Reset
    avr_.reset();
    update_state_to_shm();
}

void BridgeShmServer::update_state_to_shm() {
    auto& cp = avr_.cpu();
    shm_->cpu_state.pc = static_cast<uint16_t>(cp.program_counter());
    shm_->cpu_state.sp = cp.stack_pointer();
    shm_->cpu_state.sreg = cp.sreg();
    auto regs = cp.registers();
    for (int i = 0; i < 32; i++) {
        shm_->cpu_state.gprs[i] = regs[i];
    }
}

} // namespace vioavr::core
