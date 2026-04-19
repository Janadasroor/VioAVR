#include "vioavr/core/bridge_shm_client.hpp"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <semaphore.h>

namespace vioavr::core {

BridgeShmClient::BridgeShmClient(std::string instance_name)
    : shm_name_("/vioavr_shm_" + instance_name)
{
}

BridgeShmClient::~BridgeShmClient() {
    disconnect();
}

bool BridgeShmClient::connect() {
    shm_fd_ = shm_open(shm_name_.c_str(), O_RDWR, 0666);
    if (shm_fd_ < 0) return false;

    shm_ = static_cast<VioBridgeShm*>(mmap(nullptr, sizeof(VioBridgeShm),
                                         PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0));
    
    if (shm_ == MAP_FAILED) {
        shm_ = nullptr;
        close(shm_fd_);
        shm_fd_ = -1;
        return false;
    }

    return true;
}

void BridgeShmClient::disconnect() {
    if (shm_) {
        munmap(shm_, sizeof(VioBridgeShm));
        shm_ = nullptr;
    }
    if (shm_fd_ >= 0) {
        close(shm_fd_);
        shm_fd_ = -1;
    }
}

void BridgeShmClient::reset() {
    if (shm_) {
        shm_->command.store(1); // CMD_RESET
        sem_post(&shm_->sem_req);
        sem_wait(&shm_->sem_ack);
    }
}

void BridgeShmClient::step(double delta_time_seconds) {
    if (!shm_) return;
    shm_->request_duration = delta_time_seconds;
    shm_->request_cycles = 0;
    shm_->command.store(0); // Regular step
    sem_post(&shm_->sem_req);
    sem_wait(&shm_->sem_ack);
}

void BridgeShmClient::step_cycles(uint64_t cycles) {
    if (!shm_) return;
    shm_->request_cycles = cycles;
    shm_->request_duration = 0.0;
    shm_->command.store(0); // Regular step
    sem_post(&shm_->sem_req);
    sem_wait(&shm_->sem_ack);
}

void BridgeShmClient::set_frequency(double hz) {
    if (shm_) shm_->clock_frequency = hz;
}

void BridgeShmClient::set_digital_input(uint32_t pin_index, bool level) {
    if (shm_ && pin_index < 128) {
        shm_->digital_inputs[pin_index] = level ? 1 : 0;
    }
}

void BridgeShmClient::set_analog_input(uint8_t channel, float voltage) {
    if (shm_ && channel < 128) {
        shm_->analog_inputs[channel] = voltage;
    }
}

bool BridgeShmClient::get_digital_output(uint32_t pin_index) const {
    if (shm_ && pin_index < 128) {
        return shm_->digital_outputs[pin_index] != 0;
    }
    return false;
}

double BridgeShmClient::get_analog_output(uint8_t channel) const {
    if (shm_ && channel < 32) {
        return shm_->analog_outputs[channel];
    }
    return 0.0;
}

uint64_t BridgeShmClient::get_total_cycles() const {
    return shm_ ? shm_->telemetry.total_cycles : 0;
}

uint8_t BridgeShmClient::get_core_state() const {
    return shm_ ? shm_->telemetry.core_state : 0;
}

uint16_t BridgeShmClient::get_current_instruction() const {
    return shm_ ? shm_->telemetry.current_instruction_word : 0;
}

} // namespace vioavr::core
