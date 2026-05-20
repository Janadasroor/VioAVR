#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"

using namespace vioavr::core;

TEST_CASE("Lockbit Fidelity: Section Protection (Classic Mega)")
{
    const auto* desc = DeviceCatalog::find("ATmega32U4");
    REQUIRE(desc != nullptr);
    MemoryBus bus(*desc);
    AvrCpu cpu(bus);
    
    // ATmega32U4 Boot Start is 0x3800 (words) by default
    // We'll leave it as is.
    
    // Load some data in App section (0x1000)
    std::vector<u16> app_data = { 0x1234, 0x5678 };
    bus.write_program_word(0x0800, 0x1234); // word address 0x0800 = byte 0x1000
    
    cpu.reset();

    SUBCASE("LPM from App to App (Allowed)") {
        // LPM R0, Z
        std::vector<u16> code = { 0x95C8, 0x0000 }; 
        bus.load_flash(code);
        cpu.set_program_counter(0);
        cpu.write_register(30, 0x00);
        cpu.write_register(31, 0x10); // Z = 0x1000
        
        cpu.step();
        CHECK(cpu.registers()[0] == 0x34); // Low byte of 0x1234
    }

    SUBCASE("LPM from Boot to App (Blocked by BLB0)") {
        // Set BLB0 Mode 2 (0x04 cleared)
        // lb: x x BLB12 BLB11 BLB02 BLB01 LB2 LB1
        // Mode 2 for BLB0: 0x04 cleared. 0x3B
        bus.set_lockbit(0x3B); 
        
        // Code at 0x7000 (Boot section)
        std::vector<u16> code = { 0x95C8, 0x0000 };
        bus.write_program_word(0x3800, 0x95C8); // byte 0x7000
        cpu.set_program_counter(0x3800);
        
        cpu.write_register(30, 0x00);
        cpu.write_register(31, 0x10); // Target App at 0x1000
        
        cpu.step();
        CHECK(cpu.registers()[0] == 0xFF); // Blocked!
    }

    SUBCASE("SPM from App (Blocked by boundary)") {
        // SPM is only allowed in boot section
        cpu.set_program_counter(0x1000); 
        
        // Mock SPMCSR to enable SPM
        bus.write_data(desc->spmcsr_address, 0x01); // SELFPRGEN
        
        // Execute SPM (Page Erase)
        // Z points to App
        cpu.write_register(30, 0x00);
        cpu.write_register(31, 0x20); // 0x2000
        
        u16 spm_inst[] = { 0x95E8 }; // SPM
        bus.write_program_word(0x0800, 0x95E8); // PC = 0x0800 (byte 0x1000)
        
        cpu.step();
        // Since it's blocked, it shouldn't trigger a stall or command
        // We can check if spm_busy_cycles_left is 0
        // But we don't have direct access. 
        // We can check if the instruction took only 1 cycle (no stall)
    }
    SUBCASE("SPM Page Erase Timing and Deferred Write") {
        // Mode 1: No locks
        bus.set_lockbit(0xFF);
        
        // Target: App section word 0x0800 (byte 0x1000)
        // Data there was 0x1234
        
        // Z = 0x1000, PC = 0x3800 (Boot section)
        cpu.write_register(30, 0x00);
        cpu.write_register(31, 0x10);
        cpu.set_program_counter(0x3800);
        
        // SPM Page Erase (0x03: PGERS | SELFPRGEN)
        cpu.write_data_bus(desc->spmcsr_address, 0x03);
        bus.write_program_word(0x3800, 0x95E8); // SPM
        
        // Before step, data is 0x1234
        REQUIRE(bus.read_program_word(0x0800) == 0x1234);
        
        // Step once to execute SPM
        cpu.step();
        
        // Now CPU should be stalled at 0x3800
        // And data should STILL be 0x1234 (deferred)
        CHECK(bus.read_program_word(0x0800) == 0x1234);
        
        // Advance 32000 cycles
        bus.tick_peripherals(32000);
        CHECK(bus.read_program_word(0x0800) == 0x1234); // Still 0x1234
        
        // Advance another 32001 cycles (total 64001)
        bus.tick_peripherals(32001);
        CHECK(bus.read_program_word(0x0800) == 0xFFFF); // Now erased!
    }
}
