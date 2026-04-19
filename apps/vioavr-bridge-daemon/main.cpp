#include "vioavr/core/bridge_shm_server.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include <iostream>
#include <csignal>

using namespace vioavr::core;

BridgeShmServer* global_server = nullptr;

void signal_handler(int signal) {
    if (global_server) {
        std::cout << "\nTerminating bridge server..." << std::endl;
        global_server->stop();
    }
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        std::cout << "--- VioAVR Shared Memory Bridge Daemon ---" << std::endl;
        std::cout << "Bridge Struct Size: " << sizeof(VioBridgeShm) << " bytes" << std::endl;
        
        // 1. Setup Device
        // In a real app, this would be parameterized (e.g. --mcu atmega4809)
        BridgeShmServer server(devices::atmega4809, "mega4809");
        global_server = &server;

        // 2. Run
        server.run_loop();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
