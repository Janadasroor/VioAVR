#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/pin_mux.hpp"

using namespace vioavr::core;

TEST_CASE("Power & Clock Management Fidelity") {
    DeviceDescriptor desc{};
    desc.flash_words = 1024;
    desc.sram_bytes = 1024;
    desc.sram_start = 0x100;
    desc.spl_address = 0x5D;
    desc.sph_address = 0x5E;
    desc.sreg_address = 0x5F;
    desc.smcr_address = 0x53;
    desc.mcusr_address = 0x54;
    desc.clkpr_address = 0x61;
    desc.prr_address = 0x64;
    desc.smcr_se_mask = 0x01;
    desc.smcr_sm_mask = 0x0E;

    Timer16Descriptor t16_desc{};
    t16_desc.tcnt_address = 0x84;
    t16_desc.tccrb_address = 0x81;
    t16_desc.pr_address = 0x64;
    t16_desc.pr_bit = 3; // PRTIM1
    t16_desc.cs_mask = 0x07;

    MemoryBus bus(desc);
    AvrCpu cpu(bus);
    Timer16 t16("Timer1", t16_desc, nullptr);
    bus.attach_peripheral(t16);

    cpu.reset();

    SUBCASE("CLKPR Protection Mechanism") {
        // 1. Initial state
        CHECK(cpu.cpu_control().clock_prescaler() == 1);

        // 2. Attempt to write CLKPS without CLKPCE
        bus.write_data(0x61, 0x03); // Prescaler 8
        CHECK(cpu.cpu_control().clock_prescaler() == 1); // Should not change

        // 3. Write CLKPCE
        bus.write_data(0x61, 0x80);
        
        // 4. Write CLKPS within 4 cycles
        bus.write_data(0x61, 0x03); // Prescaler 8
        CHECK(cpu.cpu_control().clock_prescaler() == 8);
    }

    SUBCASE("PRR Integration - Timer16") {
        // 1. Setup Timer16 to tick
        bus.write_data(0x81, 0x01); // CS=1 (No prescaler)
        bus.tick_peripherals(100);
        CHECK(bus.read_data(0x84) == 100);

        // 2. Disable Timer16 via PRR
        bus.write_data(0x64, 0x08); // PRTIM1 = 1
        bus.tick_peripherals(100);
        CHECK(bus.read_data(0x84) == 100); // Should NOT have ticked

        // 3. Re-enable Timer16
        bus.write_data(0x64, 0x00); // PRTIM1 = 0
        bus.tick_peripherals(100);
        CHECK(bus.read_data(0x84) == 200); // Should have ticked again
    }
    SUBCASE("Sleep Mode & Wake Fidelity") {
        // 1. Setup INT0 (ExtInterrupt)
        ExtInterruptDescriptor ext_desc {};
        ext_desc.eicra_address = 0x69;
        ext_desc.eimsk_address = 0x3D;
        ext_desc.eifr_address = 0x3C;
        ext_desc.vector_indices[0] = 2; // INT0 vector
        
        PinMux pin_mux(3);
        ExtInterrupt int0_peri("INT0", ext_desc, pin_mux, 0);
        bus.attach_peripheral(int0_peri);
        
        // 2. Setup Timer1
        Timer16 t1("Timer1", t16_desc);
        bus.attach_peripheral(t1);
        
        // Execute SLEEP (0x9588)
        const HexImage sleep_img = { .flash_words = { 0x9588U, 0x0000U }, .entry_word = 0 };
        bus.load_image(sleep_img);
        cpu.reset();
        
        bus.write_data(0x3D, 0x01); // EIMSK: Enable INT0
        bus.write_data(0x81, 0x01); // TCCR1B: Clock 1:1
        cpu.write_sreg(0x80);       // SEI
        
        // 3. Enter Power Down
        bus.write_data(0x53, 0x01 | (2 << 1)); // SMCR: SE=1, SM=Power Down
        
        cpu.run(1); // Execute SLEEP
        
        CHECK(cpu.state() == CpuState::sleeping);
        CHECK(cpu.cpu_control().current_sleep_mode() == SleepMode::power_down);
        
        // 4. Tick and verify Timer1 is gated
        const u8 initial_tcnt = bus.read_data(0x84);
        cpu.run(100);
        CHECK(cpu.state() == CpuState::sleeping);
        CHECK(bus.read_data(0x84) == initial_tcnt); // Should NOT have changed
        
        // 5. Trigger INT0 and verify wake
        int0_peri.set_int0_level(false); // Low level triggers INT0
        cpu.run(10); // Should wake and enter ISR
        
        CHECK(cpu.state() == CpuState::running);
        CHECK(cpu.snapshot().program_counter == 2); // INT0 vector
        CHECK(!cpu.cpu_control().is_sleeping());
    }
}
