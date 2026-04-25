#include "vioavr/core/bridge_shm_server.hpp"
#include <iostream>
#include <csignal>

using namespace vioavr::core;

bool keep_running = true;

void signal_handler(int) {
    keep_running = false;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    DeviceDescriptor desc {};
    // ATmega169/329-like setup for LCD
    desc.lcd_count = 1;
    desc.lcds[0].lcdcra_address = 0xE4;
    desc.lcds[0].lcdcrb_address = 0xE5;
    desc.lcds[0].lcdfrr_address = 0xE6;
    desc.lcds[0].lcdccr_address = 0xE7;
    desc.lcds[0].lcddr_base_address = 0xEC;
    desc.lcds[0].lcddr_count = 20;
    desc.lcds[0].vector_index = 22;
    
    try {
        BridgeShmServer server(desc, "default");
        std::cout << "VioSpice Daemon running... [Ctrl+C to exit]" << std::endl;
        
        // Simulate a small background tick to keep SHM alive even without client
        while (keep_running) {
            server.run_loop(); // This will block until a command or step
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
