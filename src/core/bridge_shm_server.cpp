#include "vioavr/core/bridge_shm_server.hpp"
#include "vioavr/core/gdb_stub.hpp"
#include "vioavr/core/lcd_controller.hpp"
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
    shm_->vih_threshold.fill(3.0f); // Default 3.0V for 5V systems
    shm_->vil_threshold.fill(1.5f); // Default 1.5V for 5V systems
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

void BridgeShmServer::start_gdb(uint16_t port) {
    if (!gdb_stub_) {
        gdb_stub_ = std::make_unique<GdbStub>(avr_.cpu(), avr_.bus());
        avr_.add_trace_hook(gdb_stub_.get());
    }
    gdb_stub_->start(port);
}

void BridgeShmServer::stop() {
    running_ = false;
    // Release any waiting clients or self
    sem_post(&shm_->sem_req);
}

void BridgeShmServer::run_loop() {
    running_ = true;
    shm_->status.store(BridgeStatus::Running);

    while (running_) {
        // Wait for request from Client (Simulator)
        if (sem_wait(&shm_->sem_req) == -1) {
            if (errno == EINTR) continue;
            break;
        }

        if (!running_) break;

        // Check for Quit status from client
        if (shm_->status.load() == BridgeStatus::Quit) {
            running_ = false;
            break;
        }

        // Process based on command or step
        uint32_t cmd = shm_->command.exchange(0);
        if (cmd != 0) {
            handle_command(cmd);
        } else {
            handle_step();
        }

        // Signal completion
        shm_->sync_counter.fetch_add(1);
        sem_post(&shm_->sem_ack);
    }

    shm_->status.store(BridgeStatus::Idle);
}

void BridgeShmServer::handle_step() {
    // 1. Sync inputs from SHM to VioSpice
    for (uint32_t i = 0; i < 128; ++i) {
        // High-fidelity threshold evaluation (Hysteresis/Schmitt Trigger)
        float v = shm_->analog_inputs[i];
        float vih = shm_->vih_threshold[i];
        float vil = shm_->vil_threshold[i];
        
        // If vih/vil are non-zero, we perform analog-to-digital mapping
        if (vih > 0.0f || vil > 0.0f) {
            if (v >= vih) shm_->digital_inputs[i] = 1;
            else if (v <= vil) shm_->digital_inputs[i] = 0;
            // Between vil and vih, status remains unchanged (hysteresis)
        }

        avr_.set_external_pin(i, shm_->digital_inputs[i] ? PinLevel::high : PinLevel::low);
    }
    
    for (uint32_t i = 0; i < 128; ++i) {
        // Direct analog inputs (ADC/Signal Bank)
        avr_.set_external_voltage(i, shm_->analog_inputs[i]);
    }

    // Direct analog outputs (DAC)
    auto analog_outs = avr_.get_analog_outputs();
    for (size_t i = 0; i < analog_outs.size() && i < 32; ++i) {
        shm_->analog_outputs[i] = analog_outs[i];
    }

    // 2. Step the CPU
    if (shm_->clock_frequency > 0) {
        avr_.set_frequency(shm_->clock_frequency);
    }

    // Use request_duration if provided, else request_cycles
    if (shm_->request_duration > 0) {
        avr_.step_duration(shm_->request_duration);
    } else if (shm_->request_cycles > 0) {
        double freq = shm_->clock_frequency > 0 ? shm_->clock_frequency : 16000000.0;
        avr_.step_duration(static_cast<double>(shm_->request_cycles) / freq);
    } else {
        // Default quantum if no cycles specified
        avr_.step_duration(1000.0 / 16000000.0);
    }

    // 3. Sync outputs from VioSpice to SHM
    // We consume changes and update the output array
    auto changes = avr_.consume_pin_changes();
    for (const auto& ch : changes) {
        auto ext_id = avr_.get_external_id(ch.port_name, ch.bit_index);
        if (ext_id && *ext_id < 128) {
            shm_->digital_outputs[*ext_id] = ch.level ? 1 : 0;
        }
    }
    
    // Update VCD if active
    if (vcd_writer_) {
        vcd_writer_->next_timestamp(avr_.cpu().cycles() * (1000000000.0 / avr_.frequency()));
        for (uint32_t i = 0; i < 32; ++i) { // Log first 32 pins for now
            vcd_writer_->update_signal("p" + std::to_string(i), shm_->digital_outputs[i]);
        }
        vcd_writer_->update_signal("pc", avr_.cpu().program_counter());
    }

    // Update CPU State for UI
    update_state_to_shm();
}

void BridgeShmServer::handle_command(uint32_t cmd) {
    if (cmd & 0x01) { // RESET
        avr_.reset();
    }
    if (cmd & 0x02) { // LOAD_HEX
        std::string hex_path(shm_->command_arg);
        if (!hex_path.empty()) {
            avr_.load_hex(hex_path);
        }
    }
    if (cmd & (1 << 16)) { // TOGGLE_VCD
        if (!vcd_writer_) {
            vcd_writer_ = std::make_unique<VcdWriter>("trace.vcd");
            for (int i = 0; i < 32; i++) {
                vcd_writer_->add_signal("pin_" + std::to_string(i), 1, "p" + std::to_string(i));
            }
            vcd_writer_->add_signal("pc", 16, "pc");
            vcd_writer_->write_header();
            std::cout << "VCD Tracing enabled -> trace.vcd" << std::endl;
        } else {
            vcd_writer_.reset();
            std::cout << "VCD Tracing disabled" << std::endl;
        }
    }
    update_state_to_shm();
}

void BridgeShmServer::update_state_to_shm() {
    auto& cp = avr_.cpu();
    shm_->cpu_state.pc = cp.program_counter();
    shm_->cpu_state.last_pc = 0; // Not tracked yet
    shm_->cpu_state.sp = cp.stack_pointer();
    shm_->cpu_state.sreg = cp.sreg();
    shm_->cpu_state.rampz = cp.rampz();
    shm_->cpu_state.eind = cp.eind();
    
    auto regs = cp.registers();
    for (int i = 0; i < 32; i++) {
        shm_->cpu_state.gprs[i] = regs[i];
    }

    shm_->telemetry.total_cycles = cp.cycles();
    shm_->telemetry.total_time_ns = 0; // Needs clock conversion
    shm_->telemetry.core_state = static_cast<uint8_t>(cp.state());
    shm_->telemetry.current_instruction_word = avr_.bus().read_program_word(cp.program_counter());
    
    // Update Flags
    shm_->telemetry.flags = (cp.state() == CpuState::halted) ? TELEMETRY_FLAG_BREAKPOINT : 0;
    if (avr_.bus().analysis_freeze_requested()) {
        shm_->telemetry.flags |= TELEMETRY_FLAG_ANALYSIS_FREEZE;
        avr_.bus().clear_analysis_freeze_request();
    }

    // LCD Sync
    if (auto* lcd = avr_.lcd()) {
        shm_->lcd.enabled = lcd->is_enabled();
        shm_->lcd.duty = lcd->duty_cycle();
        shm_->lcd.segments = lcd->active_segments();
        
        const auto& data = lcd->display_data();
        for (size_t i = 0; i < data.size() && i < 20; ++i) {
            shm_->lcd.display_data[i] = data[i];
        }
    } else {
        shm_->lcd.enabled = 0;
    }
}

} // namespace vioavr::core
