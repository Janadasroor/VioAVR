#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/timer8.hpp"

using namespace vioavr::core;

TEST_CASE("TC8_ASYNC: Asynchronous Timer2 Fidelity") {
    auto device = DeviceCatalog::find("ATmega328P");
    REQUIRE(device != nullptr);
    
    // Silence logger for performance
    Logger::set_callback([](LogLevel, std::string_view){});

    Machine machine(*device);
    auto& bus = machine.bus();
    auto& cpu = machine.cpu();

    // Load a NOP to keep the CPU running
    bus.write_program_word(0, 0x0000); // NOP
    bus.write_program_word(1, 0x940C); // JMP 0 (jump to self to stay in loop)
    bus.write_program_word(2, 0x0000); 
    
    // Find Timer2 (should be a Timer8 instance)
    Timer8* timer2 = nullptr;
    for (auto* p : bus.peripherals()) {
        if (p->name() == "TIMER2") {
            timer2 = dynamic_cast<Timer8*>(p);
            break;
        }
    }
    REQUIRE(timer2 != nullptr);

    const auto* desc = &device->timers8[1]; // Timer2 is usually the second Timer8

    SUBCASE("ASSR: Busy Flag Synchronization") {
        // 1. Enable Async Mode (AS2 = 1)
        bus.write_data(desc->assr_address, 0x20); // AS2
        CHECK(timer2->async_status() == 0x20);

        // 2. Write to TCNT2
        bus.write_data(desc->tcnt_address, 0x55);
        // TCN2UB should be set immediately
        CHECK((timer2->async_status() & 0x10) != 0); // TCN2UB

        // 3. Step a few cycles - should still be busy
        machine.run(100);
        CHECK((timer2->async_status() & 0x10) != 0);

        // 4. Step past 1 TOSC cycle (~488 cycles at 16MHz)
        machine.run(500);
        // Busy bit should be cleared
        CHECK((timer2->async_status() & 0x10) == 0);
        CHECK(timer2->counter() == 0x55);
    }

    SUBCASE("RTC: 32.768kHz Internal Crystal Simulation") {
        // 1. Enable Async Mode, No EXCLK (Internal Crystal)
        bus.write_data(desc->assr_address, 0x20); // AS2
        
        // 2. Start Timer, Prescaler = 1 (CS = 1)
        bus.write_data(desc->tccrb_address, 0x01);

        // 3. Run for 10ms of "simulated" 32.768kHz time
        // 10ms at 16MHz = 160,000 cycles.
        // At 32.768kHz, this is ~327 ticks.
        machine.run(160000);

        // TCNT should have incremented ~327 times.
        // 327 % 256 = 71.
        CHECK(timer2->counter() >= 70); 
        CHECK(timer2->counter() <= 75);
    }

    SUBCASE("Power-save: Wake-up from Sleep") {
        // 1. Enable Async Mode, Start Timer
        bus.write_data(desc->assr_address, 0x20);
        bus.write_data(desc->tccrb_address, 0x01); // CS=1
        
        // 2. Enable Overflow Interrupt
        bus.write_data(desc->timsk_address, 0x01); // TOIE2
        
        // 3. Set CPU to Power-save mode
        // SM = 3 (Power-save)
        bus.write_data(device->smcr_address, (3 << 1) | 0x01); // SE | SM=3
        
        // 4. Run step - CPU should go to sleep
        machine.step(); 
        
        // 5. Run for 10ms of simulated time
        // This should trigger at least one overflow if we start TCNT high
        bus.write_data(desc->tcnt_address, 0xF0); 
        machine.run(160000);
        
        // Should have woken up and executed interrupt
        CHECK(cpu.snapshot().state == CpuState::running);
    }
}
