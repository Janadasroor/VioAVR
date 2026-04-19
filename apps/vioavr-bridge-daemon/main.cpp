#include "vioavr/core/bridge_shm_server.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/gdb_stub.hpp"
#include "vioavr/core/machine.hpp"
#include <iostream>
#include <csignal>
#include <string>
#include <vector>

using namespace vioavr::core;

BridgeShmServer* global_server = nullptr;

void signal_handler(int signal) {
    if (global_server) {
        std::cout << "\nTerminating bridge server..." << std::endl;
        global_server->stop();
    }
}

void print_help(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "Options:\n"
              << "  --mcu <name>      Select MCU (default: atmega328p)\n"
              << "  --instance <id>   SHM instance name (default: default)\n"
              << "  --gdb <port>      Start GDB stub on port\n"
              << "  --help            Show this help\n";
}

int main(int argc, char** argv) {
    std::string mcu_name = "atmega328p";
    std::string instance_name = "default";
    int gdb_port = -1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--mcu" && i + 1 < argc) mcu_name = argv[++i];
        else if (arg == "--instance" && i + 1 < argc) instance_name = argv[++i];
        else if (arg == "--gdb" && i + 1 < argc) gdb_port = std::stoi(argv[++i]);
        else if (arg == "--help") { print_help(argv[0]); return 0; }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        std::cout << "--- VioAVR Shared Memory Bridge Daemon ---" << std::endl;
        
        auto device = DeviceCatalog::find(mcu_name);
        if (!device) {
            std::cerr << "Error: MCU '" << mcu_name << "' not found in catalog." << std::endl;
            return 1;
        }

        std::cout << "MCU:      " << mcu_name << std::endl;
        std::cout << "Instance: " << instance_name << std::endl;

        BridgeShmServer server(*device, instance_name);
        global_server = &server;

        // Start GDB Stub if requested
        if (gdb_port > 0) {
            std::cout << "GDB Stub:  Internal (port " << gdb_port << ")" << std::endl;
            server.start_gdb(static_cast<uint16_t>(gdb_port));
        }

        server.run_loop();
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
