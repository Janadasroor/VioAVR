#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

using namespace vioavr::core;

TEST_CASE("SPM: Page Erase and Write Flow via Flash Instructions") {
    MemoryBus bus {devices::atmega32u4};
    AvrCpu cpu {bus};
    
    // address 0x1000 in bytes
    const u32 target_byte_addr = 0x1000;
    
    // We will load a program that:
    // 1. Sets Z = 0x1000
    // 2. Sets R1:R0 = 0xAA55
    // 3. Fills page buffer
    // 4. Erases page
    // 5. Writes page
    
    std::vector<u16> prog = {
        // Z = 0x1000
        0xE0E0, // LDI R30, 0x00
        0xE1F0, // LDI R31, 0x10
        // R1:R0 = 0xAA55
        0xE505, // LDI R16, 0x55
        0x2E00, // MOV R0, R16
        0xEA0A, // LDI R16, 0xAA
        0x2E10, // MOV R1, R16
        
        // Fill Page Buffer
        0xE001, // LDI R16, 0x01 (SPMEN)
        0xBF07, // OUT 0x37, R16 (Lockout starts here)
        0x95E8, // SPM (Fill)
        
        // Erase Page
        0xE003, // LDI R16, 0x03 (PGERS | SPMEN)
        0xBF07, // OUT 0x37, R16
        0x95E8, // SPM (Erase)
        
        // RE-ENABLE RWW
        0xE101, // LDI R16, 0x11 (RWWSRE | SPMEN)
        0xBF07, // OUT 0x37, R16
        0x95E8, // SPM (RWWSRE)

        // Write Page
        0xE005, // LDI R16, 0x05 (PGWRT | SPMEN)
        0xBF07, // OUT 0x37, R16
        0x95E8, // SPM (Write)
        
        // RE-ENABLE RWW again for reading
        0xE101, // LDI R16, 0x11
        0xBF07,
        0x95E8,

        0x94F8, // BREAK
    };
    
    bus.load_flash(prog);
    cpu.reset();
    
    // Step-by-step to be sure
    // Initial LDI/MOV (6 instructions)
    for (int i = 0; i < 6; ++i) cpu.step();
    
    // Fill
    cpu.step(); // LDI
    cpu.step(); // OUT (Sets lockout)
    cpu.step(); // SPM (Fill)
    
    // Erase
    cpu.step(); // LDI
    cpu.step(); // OUT
    cpu.step(); // SPM (Erase)
    bus.tick_peripherals(64000);
    
    CHECK(bus.read_program_byte(target_byte_addr) == 0xFF);
    CHECK(bus.read_program_byte(target_byte_addr + 1) == 0xFF);
    // RWWSRE sequence
    cpu.step(); // LDI
    cpu.step(); // OUT
    cpu.step(); // SPM

    // Write sequence
    cpu.step(); // LDI
    cpu.step(); // OUT
    cpu.step(); // SPM (Write)
    bus.tick_peripherals(64000);
    
    // Final RWWSRE to allow reading
    cpu.step(); // LDI
    cpu.step(); // OUT
    cpu.step(); // SPM

    CHECK(bus.read_program_byte(target_byte_addr) == 0x55);
    CHECK(bus.read_program_byte(target_byte_addr + 1) == 0xAA);
}
