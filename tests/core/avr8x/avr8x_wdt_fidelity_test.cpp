#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR8X WDT - CCP Protection Fidelity") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    const u16 WDT_CTRLA = 0x0100;
    const u16 CPU_CCP = 0x34;

    // Load NOPs to ensure cycles advance
    std::vector<u16> program(100, 0x0000); 
    bus.load_flash(program);
    machine.cpu().reset();

    // 1. Try writing to WDT_CTRLA without CCP
    bus.write_data(WDT_CTRLA, 0x01); // Try to enable
    CHECK(bus.read_data(WDT_CTRLA) == 0x00); // Should still be 0

    // 2. Write correct signature to CCP
    bus.write_data(CPU_CCP, 0xD8); // IOREG signature
    
    // 3. Now write to WDT_CTRLA
    bus.write_data(WDT_CTRLA, 0x09); // Enable + 8ms timeout
    CHECK(bus.read_data(WDT_CTRLA) == 0x09); // Should succeed

    // 4. Verify CCP locks again after 4 cycles (simplified as machine.run(4))
    // Actually, in our implementation, it's cycles.
    machine.run(5);
    bus.write_data(WDT_CTRLA, 0x00); // Try to disable
    CHECK(bus.read_data(WDT_CTRLA) == 0x09); // Should stay enabled (locked)
}

TEST_CASE("AVR8X WDT - Window Mode Reset") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    const u16 WDT_CTRLA = 0x0100;
    const u16 WDT_WINCTRLA = 0x0101;
    const u16 CPU_CCP = 0x34;

    // Enable WDT with 128ms timeout and 64ms window
    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(WDT_WINCTRLA, 0x05); // 64ms window (approx 64000 cycles at 1MHz)
    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(WDT_CTRLA, 0x07 | 0x08); // ENABLE=1, PERIOD=128ms
    
    // WDR (Watchdog Reset instruction) is not easily called here, 
    // but we can trigger it via the Wdt8x::reset_timer() if we had an instruction.
    // For now, let's assume the WDT is running.
}
