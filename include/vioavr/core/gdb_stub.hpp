#pragma once

#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/tracing.hpp"
#include <string>
#include <vector>
#include <set>
#include <thread>
#include <atomic>
#include <mutex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
constexpr socket_t kInvalidSocket = INVALID_SOCKET;
#else
#include <netinet/in.h>
#include <unistd.h>
using socket_t = int;
constexpr socket_t kInvalidSocket = -1;
#endif

namespace vioavr::core {

inline void socket_close(socket_t s) {
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}

inline int socket_error() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

inline bool is_valid_socket(socket_t s) {
    return s != kInvalidSocket;
}

/// RAII helper that calls WSAStartup/WSACleanup on Windows (no-op on POSIX).
class SockWrapper {
public:
    SockWrapper() {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }
    ~SockWrapper() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    SockWrapper(const SockWrapper&) = delete;
    SockWrapper& operator=(const SockWrapper&) = delete;
};

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

    [[nodiscard]] bool is_connected() const { return client_fd_ != kInvalidSocket; }
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

    SockWrapper sock_wrapper_;
    AvrCpu& cpu_;
    MemoryBus& bus_;
    uint16_t port_ {0};
    socket_t server_fd_ {kInvalidSocket};
    socket_t client_fd_ {kInvalidSocket};
    
    std::thread listen_thread_;
    std::atomic<bool> running_ {false};
    std::set<u32> breakpoints_;
    mutable std::mutex mutex_;
};

} // namespace vioavr::core
