#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/sync_engine.hpp"
#include "vioavr/core/devices/atmega328.hpp"

#include <thread>
#include <chrono>
#include <atomic>

TEST_CASE("SyncEngine Quantum and Pause/Resume Test")
{
    using namespace vioavr::core;
    
    MemoryBus bus {devices::atmega328};
    AvrCpu cpu {bus};
    
    // Create a sync engine with a quantum of 100 cycles
    auto sync_engine = create_fixed_quantum_sync_engine(100);
    cpu.set_sync_engine(sync_engine.get());

    // Load a simple NOP loop
    std::vector<u16> program(300, 0x0000); 
    bus.load_flash(program);
    cpu.reset();

    SUBCASE("Single Quantum Pause") {
        std::atomic<bool> cpu_finished {false};
        std::thread cpu_thread([&]() {
            cpu.run(150); 
            cpu_finished = true;
        });

        // Wait for CPU to hit the first quantum
        int retries = 0;
        while (cpu.cycles() < 100 && retries < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            retries++;
        }

        CHECK(cpu.cycles() == 100);
        CHECK(cpu.state() == CpuState::paused);

        sync_engine->resume();
        cpu_thread.join();
        
        CHECK(cpu.cycles() == 150);
        CHECK(cpu_finished);
    }

    SUBCASE("Multi-Quantum Pause") {
        cpu.reset();
        std::atomic<bool> cpu_finished {false};
        std::thread cpu_thread([&]() {
            cpu.run(250); 
            cpu_finished = true;
        });

        // Pause 1 at 100
        int retries = 0;
        while (cpu.cycles() < 100 && retries < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            retries++;
        }
        CHECK(cpu.cycles() == 100);
        CHECK(cpu.state() == CpuState::paused);
        sync_engine->resume();

        // Pause 2 at 200
        retries = 0;
        while (cpu.cycles() < 200 && retries < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            retries++;
        }
        CHECK(cpu.cycles() == 200);
        CHECK(cpu.state() == CpuState::paused);
        sync_engine->resume();

        cpu_thread.join();
        CHECK(cpu.cycles() == 250);
        CHECK(cpu_finished);
    }
}
