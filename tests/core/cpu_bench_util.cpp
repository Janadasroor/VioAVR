#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/logger.hpp"
#include <iostream>
#include <chrono>
#include <string>

using namespace vioavr::core;

int main(int argc, char** argv) {
    // Suppress all logs immediately
    Logger::set_callback([](LogLevel, std::string_view) {});

    if (argc < 4) {
        std::cerr << "Usage: cpu_bench_util <device_name> <hex_file> <cycles>" << std::endl;
        return 1;
    }

    std::string device_name = argv[1];
    std::string hex_file = argv[2];
    u64 cycle_limit = std::stoull(argv[3]);

    const auto* dev = DeviceCatalog::find(device_name);
    if (!dev) {
        std::cerr << "Error: device " << device_name << " not found" << std::endl;
        return 1;
    }

    Machine machine(*dev);
    
    try {
        HexImage image = HexImageLoader::load_file(hex_file, *dev);
        machine.bus().load_image(image);
    } catch (const std::exception& e) {
        std::cerr << "Error loading hex: " << e.what() << std::endl;
        return 1;
    }

    AvrCpu& cpu = machine.cpu();
    cpu.reset();

    auto start = std::chrono::high_resolution_clock::now();
    
    // Efficiently run cycles
    cpu.run(cycle_limit);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double seconds = duration.count() / 1000000.0;
    double mhz = (cycle_limit / seconds) / 1000000.0;

    std::cout << "Cycles: " << cycle_limit << std::endl;
    std::cout << "Time: " << seconds << " s" << std::endl;
    std::cout << "Speed: " << mhz << " MHz" << std::endl;

    return 0;
}
