#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <vector>

using namespace vioavr::core;

TEST_CASE("Instruction Timing Fidelity: 2-byte PC (ATmega328P) vs 3-byte PC (ATmega2560)")
{
    // 1. ATmega328P (PC=16 bit, flash=32KB)
    const auto* m328p_desc = DeviceCatalog::find("ATmega328P");
    REQUIRE(m328p_desc != nullptr);
    MemoryBus bus328p(*m328p_desc);
    AvrCpu cpu328p(bus328p);
    cpu328p.reset();
    cpu328p.set_stack_pointer(0x08FF);

    // 2. ATmega2560 (PC=22 bit, flash=256KB)
    const auto* m2560_desc = DeviceCatalog::find("ATmega2560");
    REQUIRE(m2560_desc != nullptr);
    MemoryBus bus2560(*m2560_desc);
    AvrCpu cpu2560(bus2560);
    cpu2560.reset();
    cpu2560.set_stack_pointer(0x21FF);

    SUBCASE("Instruction: CALL") {
        // CALL target
        // m328p: 4 cycles
        // m2560: 5 cycles
        u16 code[] = { 0x940E, 0x0100 }; // CALL 0x0100
        bus328p.load_flash(code);
        bus2560.load_flash(code);

        u64 start_cycles = cpu328p.cycles();
        cpu328p.step();
        CHECK(cpu328p.cycles() - start_cycles == 4);

        start_cycles = cpu2560.cycles();
        cpu2560.step();
        CHECK(cpu2560.cycles() - start_cycles == 5);
    }

    SUBCASE("Instruction: RET") {
        // m328p: 4 cycles
        // m2560: 5 cycles
        u16 code[] = { 0x9508 }; // RET
        bus328p.load_flash(code);
        bus2560.load_flash(code);

        // Pre-push some dummy address
        // m328p pushes 2 bytes
        // m2560 pushes 3 bytes
        cpu328p.set_program_counter(0);
        cpu2560.set_program_counter(0);
        
        cpu328p.set_stack_pointer(0x08FF);
        bus328p.write_data(0x08FF, 0x00);
        bus328p.write_data(0x08FE, 0x10);
        cpu328p.set_stack_pointer(0x08FD);

        cpu2560.set_stack_pointer(0x21FF);
        bus2560.write_data(0x21FF, 0x00);
        bus2560.write_data(0x21FE, 0x10);
        bus2560.write_data(0x21FD, 0x00);
        cpu2560.set_stack_pointer(0x21FC);

        u64 start_cycles = cpu328p.cycles();
        cpu328p.step();
        CHECK(cpu328p.cycles() - start_cycles == 4);

        start_cycles = cpu2560.cycles();
        cpu2560.step();
        CHECK(cpu2560.cycles() - start_cycles == 5);
    }

    SUBCASE("Instruction: RCALL") {
        // m328p: 3 cycles
        // m2560: 4 cycles
        u16 code[] = { 0xD001 }; // RCALL +1
        bus328p.load_flash(code);
        bus2560.load_flash(code);

        u64 start_cycles = cpu328p.cycles();
        cpu328p.step();
        CHECK(cpu328p.cycles() - start_cycles == 3);

        start_cycles = cpu2560.cycles();
        cpu2560.step();
        CHECK(cpu2560.cycles() - start_cycles == 4);
    }

    SUBCASE("Instruction: ICALL") {
        // m328p: 3 cycles
        // m2560: 4 cycles
        u16 code[] = { 0x9509 }; // ICALL
        bus328p.load_flash(code);
        bus2560.load_flash(code);
        
        cpu328p.set_program_counter(0);
        cpu328p.write_register(30, 0x10);
        cpu328p.write_register(31, 0x01); // Z=0x0110

        cpu2560.set_program_counter(0);
        cpu2560.write_register(30, 0x10);
        cpu2560.write_register(31, 0x01); // Z=0x0110

        u64 start_cycles = cpu328p.cycles();
        cpu328p.step();
        CHECK(cpu328p.cycles() - start_cycles == 3);

        start_cycles = cpu2560.cycles();
        cpu2560.step();
        CHECK(cpu2560.cycles() - start_cycles == 4);
    }

    SUBCASE("Interrupt Latency") {
        // m328p: 4 cycles
        // m2560: 5 cycles
        // Simulating an interrupt trigger (reset state)
        // We'll manually trigger it
        
        // Setup SEI first
        u16 sei_code[] = { 0x9478 }; 
        bus328p.load_flash(sei_code);
        bus2560.load_flash(sei_code);
        cpu328p.step();
        cpu2560.step();

        // Now we need to mock a periph that requests int
        // Actually we can just call service_interrupt_if_needed directly if it was public...
        // but it's private.
        // We can use a device that has a flag we can set.
        
        // Instead of mocking a whole periph, let's just test that the helper method works.
        // But we want end-to-end.
    }
}
