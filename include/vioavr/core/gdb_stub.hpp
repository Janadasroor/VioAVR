#pragma once

#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/tracing.hpp"
#include <string>
#include <vector>
#include <set>
#include <thread>
#include <atomic>
#include <mutex>
#include <netinet/in.h>

namespace vioavr::core {

class Machine;

/**
 * @brief GDB Remote Serial Protocol (RSP) Stub for VioAVR.
 * 
 * Allows avr-gdb to attach to the emulator via TCP.
 */
class GdbStub final : public ITraceHook {
public:
    explicit GdbStub(AvrCpu& cpu, MemoryBus& bus);
    ~GdbStub() override;

    /**
     * @brief Start the GDB server on the specified port.
     * Starts a background thread.
     */
    void start(uint16_t port);
    
    /**
     * @brief Stop the GDB server.
     */
    void stop();

    [[nodiscard]] bool is_connected() const { return client_fd_ != -1; }
    [[nodiscard]] uint16_t port() const { return port_; }

    // ITraceHook implementation
    void on_instruction(u32 address, u16 opcode, std::string_view mnemonic) override;
    void on_register_write([[maybe_unused]] u8 index, [[maybe_unused]] u8 value) override {}
    void on_sreg_write([[maybe_unused]] u8 value) override {}
    void on_memory_read([[maybe_unused]] u16 address, [[maybe_unused]] u8 value) override {}
    void on_memory_write([[maybe_unused]] u16 address, [[maybe_unused]] u8 value) override {}
    void on_interrupt([[maybe_unused]] u8 vector) override {}

private:
    void listen_loop();
    void client_loop();
    
    // Packet Handling
    std::string receive_packet();
    void send_packet(const std::string& data);
    void handle_packet(const std::string& packet);

    // Command Handlers
    void handle_query(const std::string& cmd);
    void handle_register_read_all();
    void handle_register_read_single(const std::string& cmd);
    void handle_register_write_single(const std::string& cmd);
    void handle_memory_read(const std::string& cmd);
    void handle_memory_write(const std::string& cmd);
    void handle_step();
    void handle_continue();
    void handle_vcont(const std::string& cmd);
    void handle_breakpoint_set(const std::string& cmd);
    void handle_breakpoint_remove(const std::string& cmd);

    // Helpers
    static uint8_t checksum(const std::string& data);
    static std::string hex_encode(const void* data, size_t len);
    static std::vector<uint8_t> hex_decode(const std::string& hex);
    void send_trap();

    AvrCpu& cpu_;
    MemoryBus& bus_;
    uint16_t port_ {0};
    int server_fd_ {-1};
    int client_fd_ {-1};
    
    std::thread listen_thread_;
    std::atomic<bool> running_ {false};
    std::set<u32> breakpoints_;
    mutable std::mutex mutex_;
};

} // namespace vioavr::core
