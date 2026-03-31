#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/watchdog_timer.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;

TEST_CASE("Watchdog Timer: Reset Mode") {
    MemoryBus bus {devices::atmega328};
    AvrCpu cpu {bus};
    WatchdogTimer wdt {"WDT", devices::atmega328, cpu};
    bus.attach_peripheral(wdt);
    cpu.set_watchdog_timer(&wdt);

    // LDI r16, 0x08 (WDE=1, WDP=0 -> 16ms)
    // STS 0x60, r16
    // loop: RJMP loop
    std::vector<u16> program = {
        0xE008, // LDI r16, 0x08
        0x9300, 0x0060, // STS 0x60, r16
        0xCFFF  // RJMP -1
    };

    bus.load_flash(program);
    cpu.reset();

    // Run for a bit, should be running
    for (int i = 0; i < 100; ++i) {
        cpu.step();
        CHECK(cpu.state() == CpuState::running);
    }

    // Run until timeout (16ms @ 16MHz = 256,000 cycles)
    // We use a manual loop instead of cpu.run() because reset() resets cycles to 0,
    // which would cause an infinite loop in cpu.run() if it never reaches the target.
    bool reset_detected = false;
    for (int i = 0; i < 1000000; ++i) {
        u64 prev_cycles = cpu.cycles();
        cpu.step();
        if (cpu.cycles() < prev_cycles && i > 1000) {
            reset_detected = true;
            break;
        }
    }

    CHECK(reset_detected);
    CHECK(cpu.program_counter() == bus.reset_word_address());
}

TEST_CASE("Watchdog Timer: WDR resets timer") {
    MemoryBus bus {devices::atmega328};
    AvrCpu cpu {bus};
    WatchdogTimer wdt {"WDT", devices::atmega328, cpu};
    bus.attach_peripheral(wdt);
    cpu.set_watchdog_timer(&wdt);

    // LDI r16, 0x08 (WDE=1)
    // STS WDTCSR, r16
    // loop: 
    //   WDR
    //   RJMP loop
    std::vector<u16> program = {
        0xE008, // LDI r16, 0x08
        0x9300, 0x0060, // STS 0x60, r16
        0x95A8, // WDR
        0xCFFE  // RJMP -2
    };

    bus.load_flash(program);
    cpu.reset();

    // Run for more than timeout cycles, but with WDR
    // 500k cycles is well past the 256k timeout
    for (int i = 0; i < 500000; ++i) {
        u64 prev_cycles = cpu.cycles();
        cpu.step();
        if (cpu.cycles() < prev_cycles && i > 1000) {
             FAIL("CPU reset unexpectedly at cycle " << prev_cycles);
             break;
        }
    }

    CHECK(cpu.state() == CpuState::running);
    CHECK(cpu.cycles() >= 500000);
}

TEST_CASE("Watchdog Timer: Interrupt Mode") {
    MemoryBus bus {devices::atmega328};
    AvrCpu cpu {bus};
    WatchdogTimer wdt {"WDT", devices::atmega328, cpu};
    bus.attach_peripheral(wdt);
    cpu.set_watchdog_timer(&wdt);

    // LDI r16, 0x40 (WDIE=1)
    // STS WDTCSR, r16
    // loop: RJMP loop
    std::vector<u16> program = {
        0xE400, // LDI r16, 0x40
        0x9300, 0x0060, // STS 0x60, r16
        0xCFFF  // RJMP -1
    };

    bus.load_flash(program);
    cpu.reset();

    // Run past timeout with interrupts DISABLED
    cpu.run(300000);
    
    // WDIF should be set (bit 7)
    CHECK((wdt.read(0x60) & 0x80) != 0);
    // PC should still be in the loop (not vector 25)
    CHECK(cpu.program_counter() != 25);

    // Now enable interrupts and step once to trigger ISR
    cpu.write_sreg(0x80); // SEI
    cpu.step();

    // PC should be at vector 25 (0x19)
    CHECK(cpu.program_counter() == 25);
    // WDIF should be cleared now by hardware entry
    CHECK((wdt.read(0x60) & 0x80) == 0);
}
