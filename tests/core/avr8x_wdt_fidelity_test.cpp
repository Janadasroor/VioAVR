#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR8X WDT8X - Window Mode Fidelity") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    std::vector<u16> program = {0x95A8U, 0xCFFF}; // WDR, RJMP -1
    bus.load_flash(program);
    machine.cpu().reset();

    // 4809 WDT: CTRLA=0x100, STATUS=0x101
    // CTRLA bits 7:4 is WINDOW, 3:0 is PERIOD.
    // Set PERIOD=8 (1s), WINDOW=4 (64ms -> 1M cycles)
    bus.write_data(0x100, (0x4U << 4U) | 0x08U);

    // 0 is definitely < 1M cycles.
    machine.step(); // Execute WDR
    
    // PC should be 0 because WDR caused immediate reset
    CHECK(machine.cpu().program_counter() == 0);
}

TEST_CASE("AVR8X WDT8X - Window Mode Success") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    // PERIOD=8 (1s), WINDOW=4 (64ms -> 1M cycles)
    bus.write_data(0x100, (0x4U << 4U) | 0x08U);
    
    std::vector<u16> program = {0x95A8U, 0xCFFF}; // WDR, RJMP -1
    bus.load_flash(program);
    machine.cpu().reset();

    // 1. Run past window (e.g. 1.5M cycles)
    machine.run(1500000);
    
    // PC should be at 1 (executing RJMP)
    CHECK(machine.cpu().program_counter() == 1);
    
    // No reset should have happened
    // (If reset happened, PC would be 0 or stuck in init)
}
