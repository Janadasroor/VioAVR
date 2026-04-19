#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <vector>

using namespace vioavr::core;

void send_rsp(int fd, const std::string& data) {
    uint8_t sum = 0;
    for (char c : data) sum += (uint8_t)c;
    char sum_hex[8];
    sprintf(sum_hex, "%02x", sum);
    std::string packet = "$" + data + "#" + sum_hex;
    if (write(fd, packet.c_str(), packet.length()) != (ssize_t)packet.length()) {
        std::cerr << "Write failed" << std::endl;
    }
}

std::string recv_rsp(int fd) {
    char ch;
    // Wait for ack or packet start
    while(read(fd, &ch, 1) == 1) {
        if (ch == '$') break;
        if (ch == '+') continue; 
    }
    std::string res;
    while(read(fd, &ch, 1) == 1 && ch != '#') res += ch;
    char sum[2];
    if (read(fd, sum, 2) != 2) return "";
    char ack = '+'; 
    if (write(fd, &ack, 1) != 1) {
        std::cerr << "ACK write failed" << std::endl;
    }
    return res;
}

int main() {
    std::cout << "Starting GDB Stub Functional Test..." << std::endl;
    
    auto machine = Machine::create_for_device("ATmega328P");
    if (!machine) {
        std::cerr << "Failed to create machine" << std::endl;
        return 1;
    }

    // atmega328p flash words
    // Word 0: LDI R16, 0xAA (0xEA0A)
    // Word 1: LDI R17, 0x55 (0xE515)
    // Word 2: ADD R16, R17 (0x0F01) <- Break here (Byte 4)
    // Word 3: NOP (0x0000)
    // Word 4: RJMP .-2 (0xCFFD)
    
    std::vector<u16> program = {0xEA0A, 0xE515, 0x0F01, 0x0000, 0xCFFB};
    machine->bus().load_flash(program);

    uint16_t test_port = 12345;
    machine->enable_gdb(test_port);
    machine->reset();
    usleep(100000); // 100ms

    // Connect as GDB client BEFORE starting MCU
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(test_port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); 
        return 1;
    }
    std::cout << "1. Connected to GDB stub." << std::endl;

    // Set breakpoint at byte address 4 (word 2)
    send_rsp(sock, "Z0,4,2");
    if (recv_rsp(sock) != "OK") {
        std::cerr << "Failed to set breakpoint" << std::endl;
        return 1;
    }
    std::cout << "2. Breakpoint set at 0x4." << std::endl;

    // START MCU thread now
    std::atomic<bool> run_mcu{true};
    std::thread mcu_thread([&]() {
        while(run_mcu) {
            machine->run(100);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    // Continue
    send_rsp(sock, "c");
    std::cout << "3. Continuing execution..." << std::endl;

    // Wait for stop (S05)
    std::string stop = recv_rsp(sock);
    std::cout << "4. Received stop signal: " << stop << std::endl;
    run_mcu = false;
    if (mcu_thread.joinable()) mcu_thread.join();

    if (stop != "S05") {
        std::cerr << "Expected S05 stop" << std::endl;
        return 1;
    }

    // Verify PC is 4
    send_rsp(sock, "p22"); 
    std::string pc_hex = recv_rsp(sock);
    std::cout << "5. PC Register (hex): " << pc_hex << std::endl;
    // uint32_t 4 in little-endian hex: 04000000
    if (pc_hex != "04000000") {
        std::cerr << "Unexpected PC value: " << pc_hex << " (Expected 04000000)" << std::endl;
        return 1;
    }

    // Verify R16 is 0xAA
    send_rsp(sock, "p10");
    std::string r16_hex = recv_rsp(sock);
    std::cout << "6. R16 Register: " << r16_hex << std::endl;
    if (r16_hex != "aa") {
        std::cerr << "Unexpected R16 value: " << r16_hex << std::endl;
        return 1;
    }

    // Verify R17 is 0x55
    send_rsp(sock, "p11");
    std::string r17_hex = recv_rsp(sock);
    std::cout << "7. R17 Register: " << r17_hex << std::endl;
    if (r17_hex != "55") {
        std::cerr << "Unexpected R17 value: " << r17_hex << std::endl;
        return 1;
    }

    std::cout << "GDB STUB TEST PASSED!" << std::endl;
    
    close(sock);
    run_mcu = false;
    if (mcu_thread.joinable()) mcu_thread.join();
    machine->disable_gdb();

    return 0;
}
