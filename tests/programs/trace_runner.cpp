/*
 * trace_runner.cpp - Load ELF/binary and dump register trace for SimAVR comparison
 * 
 * Usage: ./trace_runner <binary.bin> [--steps N]
 * 
 * Outputs CSV: step,pc,cycles,r0,r1,...,r31,sreg,sp
 */

#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"
#include "vioavr/core/logger.hpp"

#include <fstream>
#include <vector>
#include <iomanip>
#include <iostream>
#include <cstdlib>

using namespace vioavr::core;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: trace_runner <binary.bin> [--steps N]" << std::endl;
        return 1;
    }
    
    // Suppress Logger output
    Logger::set_callback([](LogLevel, std::string_view) {});
    
    std::string bin_path = argv[1];
    int max_steps = 100;
    
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--steps" && i + 1 < argc) {
            max_steps = std::atoi(argv[i + 1]);
        }
    }
    
    // Load binary
    std::ifstream ifs(bin_path, std::ios::binary);
    if (!ifs) {
        std::cerr << "Cannot open " << bin_path << std::endl;
        return 1;
    }
    
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(ifs)),
                                std::istreambuf_iterator<char>());
    
    if (bytes.empty()) {
        std::cerr << "Empty binary file" << std::endl;
        return 1;
    }
    
    // Convert to 16-bit words (little-endian)
    if (bytes.size() % 2 != 0) bytes.push_back(0);
    
    std::vector<u16> flash_words;
    for (size_t i = 0; i < bytes.size(); i += 2) {
        u16 word = bytes[i] | (static_cast<u16>(bytes[i + 1]) << 8);
        flash_words.push_back(word);
    }
    
    // Pad with NOPs
    while (flash_words.size() < static_cast<size_t>(max_steps + 10)) {
        flash_words.push_back(0x0000);
    }
    
    MemoryBus bus{devices::atmega328};
    AvrCpu cpu{bus};
    
    HexImage image{
        .flash_words = flash_words,
        .entry_word = 0U
    };
    bus.load_image(image);
    cpu.reset();
    
    // Output CSV header (matching SimAVR format: step,pc,sreg,sp,r0,...,r31)
    std::cout << "step,pc,sreg,sp";
    for (int i = 0; i < 32; ++i) std::cout << ",r" << i;
    std::cout << std::endl;

    // Dump initial state
    auto snap = cpu.snapshot();
    std::cout << "0," << snap.program_counter << "," << static_cast<int>(snap.sreg) << "," << snap.stack_pointer;
    for (int i = 0; i < 32; ++i) std::cout << "," << static_cast<int>(snap.gpr[i]);
    std::cout << std::endl;

    // Step through and dump
    for (int step = 1; step <= max_steps; ++step) {
        if (cpu.state() == CpuState::halted) break;

        cpu.step();
        snap = cpu.snapshot();

        std::cout << step << "," << snap.program_counter << "," << static_cast<int>(snap.sreg) << "," << snap.stack_pointer;
        for (int i = 0; i < 32; ++i) std::cout << "," << static_cast<int>(snap.gpr[i]);
        std::cout << std::endl;
    }
    
    return 0;
}
