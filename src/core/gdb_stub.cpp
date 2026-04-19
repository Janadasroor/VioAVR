#include "vioavr/core/gdb_stub.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/machine.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <poll.h>

namespace vioavr::core {

GdbStub::GdbStub(AvrCpu& cpu, MemoryBus& bus) : cpu_(cpu), bus_(bus) {}

GdbStub::~GdbStub() {
    stop();
}

void GdbStub::start(uint16_t port) {
    if (running_) return;

    port_ = port;
    running_ = true;
    listen_thread_ = std::thread(&GdbStub::listen_loop, this);
}

void GdbStub::stop() {
    running_ = false;
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }
    if (client_fd_ != -1) {
        close(client_fd_);
        client_fd_ = -1;
    }
    if (listen_thread_.joinable()) {
        listen_thread_.join();
    }
}

void GdbStub::listen_loop() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) return;

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return;
    }

    if (listen(server_fd_, 1) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return;
    }

    std::cout << "GDB Stub listening on port " << port_ << std::endl;

    while (running_) {
        struct pollfd pfd {};
        pfd.fd = server_fd_;
        pfd.events = POLLIN;

        if (poll(&pfd, 1, 1000) > 0) {
            client_fd_ = accept(server_fd_, nullptr, nullptr);
            if (client_fd_ >= 0) {
                std::cout << "GDB Client connected" << std::endl;
                client_loop();
                close(client_fd_);
                client_fd_ = -1;
            }
        }
    }
}

void GdbStub::client_loop() {
    while (running_ && client_fd_ != -1) {
        std::string packet = receive_packet();
        if (packet.empty()) break; // Connection closed or error

        handle_packet(packet);
    }
}

std::string GdbStub::receive_packet() {
    char ch;
    std::string packet;

    // Wait for '$'
    while (read(client_fd_, &ch, 1) == 1) {
        if (ch == '$') break;
        if (ch == 0x03) { // Ctrl-C
            cpu_.halt();
            send_packet("S05"); // Signal 5 = TRAP
        }
    }

    // Read until '#'
    while (read(client_fd_, &ch, 1) == 1) {
        if (ch == '#') break;
        packet += ch;
    }

    // Read checksum (2 bytes)
    char sum[2];
    if (read(client_fd_, sum, 2) != 2) return "";

    // Send ACK
    const char ack = '+';
    write(client_fd_, &ack, 1);

    return packet;
}

void GdbStub::send_packet(const std::string& data) {
    if (client_fd_ == -1) return;
    
    std::string packet = "$" + data + "#";
    uint8_t sum = checksum(data);
    
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(sum);
    packet += ss.str();

    write(client_fd_, packet.c_str(), packet.length());
}

void GdbStub::handle_packet(const std::string& packet) {
    if (packet.empty()) return;

    char cmd = packet[0];
    switch (cmd) {
        case '?': // Last signal
            send_packet("S05");
            break;
        case 'g': // Read all registers
            handle_register_read_all();
            break;
        case 'm': // Read memory
            handle_memory_read(packet);
            break;
        case 'M': // Write memory
            handle_memory_write(packet);
            break;
        case 'p': // Read single register
            handle_register_read_single(packet);
            break;
        case 'P': // Write single register
            handle_register_write_single(packet);
            break;
        case 'c': // Continue
            handle_continue();
            break;
        case 's': // Step
            handle_step();
            break;
        case 'Z': // Set breakpoint
            handle_breakpoint_set(packet);
            break;
        case 'z': // Remove breakpoint
            handle_breakpoint_remove(packet);
            break;
        case 'v': // vCont or other v packet
            if (packet.find("vCont") == 0) {
                handle_vcont(packet);
            } else {
                send_packet("");
            }
            break;
        case 'q': // Query
            handle_query(packet);
            break;
        default:
            send_packet(""); // Not supported
            break;
    }
}

void GdbStub::handle_register_read_all() {
    auto snap = cpu_.snapshot();
    std::string res;
    
    // R0..R31 (32 bytes)
    res += hex_encode(snap.gpr.data(), 32);
    
    // SREG (1 byte)
    res += hex_encode(&snap.sreg, 1);
    
    // SP (2 bytes)
    uint16_t sp = snap.stack_pointer;
    res += hex_encode(&sp, 2);
    
    // PC (4 bytes)
    uint32_t pc = snap.program_counter * 2; // GDB likes byte-addresses for PC
    res += hex_encode(&pc, 4);

    send_packet(res);
}

void GdbStub::handle_register_read_single(const std::string& cmd) {
    // p<hex_index>
    int reg_idx = std::stoi(cmd.substr(1), nullptr, 16);
    auto snap = cpu_.snapshot();

    if (reg_idx < 32) {
        send_packet(hex_encode(&snap.gpr[reg_idx], 1));
    } else if (reg_idx == 32) {
        send_packet(hex_encode(&snap.sreg, 1));
    } else if (reg_idx == 33) {
        uint16_t sp = snap.stack_pointer;
        send_packet(hex_encode(&sp, 2));
    } else if (reg_idx == 34) {
        uint32_t pc = snap.program_counter * 2;
        send_packet(hex_encode(&pc, 4));
    } else {
        send_packet("E01");
    }
}

void GdbStub::handle_memory_read(const std::string& cmd) {
    // m<addr>,<len>
    size_t comma = cmd.find(',');
    if (comma == std::string::npos) {
        send_packet("E01");
        return;
    }
    uint32_t addr = std::stoul(cmd.substr(1, comma - 1), nullptr, 16);
    uint32_t len = std::stoul(cmd.substr(comma + 1), nullptr, 16);

    std::vector<uint8_t> data(len);
    for (uint32_t i = 0; i < len; ++i) {
        uint32_t curr_addr = addr + i;
        if (curr_addr < 0x800000) {
            // Flash Space (0x0..0x7FFFFF)
            data[i] = bus_.read_program_byte(curr_addr);
        } else if (curr_addr >= 0x800000 && curr_addr < 0x810000) {
            // Data Space (0x800000..0x80FFFF)
            data[i] = bus_.read_data(static_cast<uint16_t>(curr_addr - 0x800000));
        } else {
            // Unsupported or Reserved
            data[i] = 0;
        }
    }

    send_packet(hex_encode(data.data(), data.size()));
}

void GdbStub::handle_memory_write(const std::string& cmd) {
    // M<addr>,<len>:<data_hex>
    size_t comma = cmd.find(',');
    size_t colon = cmd.find(':');
    if (comma == std::string::npos || colon == std::string::npos) {
        send_packet("E01");
        return;
    }

    uint32_t addr = std::stoul(cmd.substr(1, comma - 1), nullptr, 16);
    uint32_t len = std::stoul(cmd.substr(comma + 1, colon - comma - 1), nullptr, 16);
    std::string hex_data = cmd.substr(colon + 1);

    if (hex_data.length() < len * 2) {
        send_packet("E01");
        return;
    }

    for (uint32_t i = 0; i < len; ++i) {
        std::string byte_hex = hex_data.substr(i * 2, 2);
        uint8_t val = static_cast<uint8_t>(std::stoul(byte_hex, nullptr, 16));
        uint32_t curr_addr = addr + i;

        if (curr_addr < 0x800000) {
            // Flash write (requires NVM/SPM usually, but GDB might want to force it for patches)
            // For now, only support SRAM writes
        } else if (curr_addr >= 0x800000 && curr_addr < 0x810000) {
            bus_.write_data(static_cast<uint16_t>(curr_addr - 0x800000), val);
        }
    }

    send_packet("OK");
}

void GdbStub::handle_continue() {
    cpu_.resume();
}

void GdbStub::handle_vcont(const std::string& cmd) {
    if (cmd == "vCont?") {
        send_packet("vCont;c;s;C;S");
        return;
    }

    // vCont;c or vCont;s
    if (cmd.find("vCont;c") == 0) {
        handle_continue();
    } else if (cmd.find("vCont;s") == 0) {
        handle_step();
    } else {
        send_packet("");
    }
}

void GdbStub::handle_step() {
    cpu_.resume();
    cpu_.step();
    cpu_.halt();
    send_packet("S05");
}

void GdbStub::handle_query(const std::string& cmd) {
    if (cmd == "qSupported") {
        send_packet("PacketSize=1024;hwbreak+;swbreak+;vCont+");
    } else if (cmd.find("qAttached") == 0) {
        send_packet("1");
    } else if (cmd.find("qfThreadInfo") == 0) {
        send_packet("m1");
    } else if (cmd.find("qsThreadInfo") == 0) {
        send_packet("l");
    } else if (cmd.find("qC") == 0) {
        send_packet("QC1");
    } else {
        send_packet("");
    }
}

void GdbStub::handle_register_write_single(const std::string& cmd) {
    // P<reg>=<val_hex>
    size_t eq = cmd.find('=');
    if (eq == std::string::npos) {
        send_packet("E01");
        return;
    }
    uint32_t reg_idx = std::stoul(cmd.substr(1, eq - 1), nullptr, 16);
    std::string hex_val = cmd.substr(eq + 1);
    
    // Convert hex to bytes (little endian)
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex_val.length(); i += 2) {
        bytes.push_back(static_cast<uint8_t>(std::stoul(hex_val.substr(i, 2), nullptr, 16)));
    }

    if (bytes.empty()) {
        send_packet("E01");
        return;
    }

    if (reg_idx < 32) {
        cpu_.registers()[reg_idx] = bytes[0];
    } else if (reg_idx == 32) { // PC (4 bytes)
        uint32_t pc = 0;
        if (bytes.size() >= 4) {
            pc = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
        } else if (bytes.size() >= 2) {
             pc = bytes[0] | (bytes[1] << 8);
        }
        cpu_.set_program_counter(pc / 2);
    } else if (reg_idx == 33) { // SP (2 bytes)
        uint16_t sp = bytes[0] | (bytes[1] << 8);
        cpu_.set_stack_pointer(sp);
    } else if (reg_idx == 34) { // SREG (1 byte)
        cpu_.set_sreg(bytes[0]);
    }

    send_packet("OK");
}

void GdbStub::handle_breakpoint_set(const std::string& cmd) {
    // Z0,addr,kind
    size_t comma1 = cmd.find(',');
    size_t comma2 = cmd.find(',', comma1 + 1);
    uint32_t addr = std::stoul(cmd.substr(comma1 + 1, comma2 - comma1 - 1), nullptr, 16);
    
    std::lock_guard<std::mutex> lock(mutex_);
    breakpoints_.insert(addr);
    send_packet("OK");
}

void GdbStub::handle_breakpoint_remove(const std::string& cmd) {
    // z0,addr,kind
    size_t comma1 = cmd.find(',');
    size_t comma2 = cmd.find(',', comma1 + 1);
    uint32_t addr = std::stoul(cmd.substr(comma1 + 1, comma2 - comma1 - 1), nullptr, 16);
    
    std::lock_guard<std::mutex> lock(mutex_);
    breakpoints_.erase(addr);
    send_packet("OK");
}

void GdbStub::on_instruction(u32 address, u16 opcode, std::string_view mnemonic) {
    (void)opcode;
    (void)mnemonic;
    
    uint32_t byte_addr = address * 2;
    std::lock_guard<std::mutex> lock(mutex_);
    if (breakpoints_.count(byte_addr)) {
        cpu_.halt();
        send_packet("S05");
    }
}

uint8_t GdbStub::checksum(const std::string& data) {
    uint8_t sum = 0;
    for (char c : data) sum += static_cast<uint8_t>(c);
    return sum;
}

std::string GdbStub::hex_encode(const void* data, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    std::stringstream ss;
    for (size_t i = 0; i < len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(p[i]);
    }
    return ss.str();
}

} // namespace vioavr::core
