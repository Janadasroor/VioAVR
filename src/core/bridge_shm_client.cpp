#include "vioavr/core/bridge_shm_client.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

namespace vioavr::core {

BridgeShmClient::BridgeShmClient(std::string instance_name)
    : shm_name_("/vioavr_shm_" + instance_name) {}

BridgeShmClient::~BridgeShmClient() {
    disconnect();
}

bool BridgeShmClient::connect() {
    shm_fd_ = shm_open(shm_name_.c_str(), O_RDWR, 0666);
    if (shm_fd_ == -1) return false;

    shm_ = static_cast<VioBridgeShm*>(mmap(nullptr, sizeof(VioBridgeShm), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0));
    if (shm_ == MAP_FAILED) {
        shm_ = nullptr;
        return false;
    }

    if (shm_->magic != VIOAVR_BRIDGE_MAGIC) {
        disconnect();
        return false;
    }

    return true;
}

void BridgeShmClient::disconnect() {
    if (shm_) {
        munmap(shm_, sizeof(VioBridgeShm));
        shm_ = nullptr;
    }
    if (shm_fd_ != -1) {
        close(shm_fd_);
        shm_fd_ = -1;
    }
}

void BridgeShmClient::reset() {
    if (!shm_) return;
    shm_->command.store(1); // Reset
    shm_->request_duration = 0;
    sem_post(&shm_->sem_req);
    sem_wait(&shm_->sem_ack);
}

void BridgeShmClient::step(uint64_t cycles) {
    if (!shm_) return;
    shm_->request_duration = 0;
    shm_->request_cycles = cycles;
    shm_->command = 0; // 0 = step
    sem_post(&shm_->sem_req);
    wait_step_done();
}

void BridgeShmClient::step(double duration) {
    if (!shm_) return;
    shm_->request_duration = duration;
    shm_->request_cycles = 0;
    shm_->command = 0; // 0 = step
    sem_post(&shm_->sem_req);
    wait_step_done();
}

void BridgeShmClient::set_frequency(double hz) {
    if (shm_) shm_->clock_frequency = hz;
}

void BridgeShmClient::wait_step_done() {
    if (!shm_) return;
    sem_wait(&shm_->sem_ack);
}

void BridgeShmClient::set_digital_input(uint8_t pin, bool high) {
    if (shm_ && pin < 128) {
        shm_->digital_inputs[pin] = high ? 1 : 0;
    }
}

bool BridgeShmClient::get_digital_output(uint8_t pin) const {
    if (shm_ && pin < 128) {
        return shm_->digital_outputs[pin] != 0;
    }
    return false;
}

void BridgeShmClient::set_analog_input(uint8_t channel, float voltage) {
    if (shm_ && channel < 32) {
        shm_->analog_inputs[channel] = voltage;
    }
}

float BridgeShmClient::get_analog_output(uint8_t channel) const {
    if (shm_ && channel < 32) {
        return shm_->analog_outputs[channel];
    }
    return 0.0f;
}

void BridgeShmClient::set_thresholds(uint8_t pin, float vih, float vil) {
    if (shm_ && pin < 128) {
        shm_->vih_threshold[pin] = vih;
        shm_->vil_threshold[pin] = vil;
    }
}

AvrCpuState BridgeShmClient::get_cpu_state() const {
    if (shm_) return shm_->cpu_state;
    return {};
}

} // namespace vioavr::core
