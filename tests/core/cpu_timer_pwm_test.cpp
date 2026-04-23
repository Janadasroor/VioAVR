#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;

namespace {
void run_to_cycle(AvrCpu& cpu, u64 target) {
    while (cpu.cycles() < target) {
        cpu.step();
    }
}
}

TEST_CASE("Timer8: CTC Mode Precise") {
    MemoryBus bus {devices::atmega328p};
    Timer8 timer0 {"TIMER0", devices::atmega328p.timers8[0]};
    bus.attach_peripheral(timer0);
    AvrCpu cpu {bus};

    // Setup: 9 cycles
    std::vector<u16> program = {
        0xE00A, // LDI r16, 10
        0x9300, 0x0047, // STS OCR0A, r16
        0xE002, // LDI r16, 0x02
        0x9300, 0x0044, // STS TCCR0A, r16
        0xE001, // LDI r16, 0x01
        0x9300, 0x0045, // STS TCCR0B, r16 (starts timer)
    };
    // Add many NOPs to see every cycle
    for (int i = 0; i < 20; ++i) program.push_back(0x0000);

    bus.load_flash(program);
    cpu.reset();

    run_to_cycle(cpu, 9);
    CHECK(timer0.counter() == 2); 

    cpu.step(); CHECK(cpu.cycles() == 10); CHECK(timer0.counter() == 3);
    cpu.step(); CHECK(cpu.cycles() == 11); CHECK(timer0.counter() == 4);
    cpu.step(); CHECK(cpu.cycles() == 12); CHECK(timer0.counter() == 5);
    cpu.step(); CHECK(cpu.cycles() == 13); CHECK(timer0.counter() == 6);
    cpu.step(); CHECK(cpu.cycles() == 14); CHECK(timer0.counter() == 7);
    cpu.step(); CHECK(cpu.cycles() == 15); CHECK(timer0.counter() == 8);
    cpu.step(); CHECK(cpu.cycles() == 16); CHECK(timer0.counter() == 9);
    cpu.step(); CHECK(cpu.cycles() == 17); CHECK(timer0.counter() == 10);
    cpu.step(); CHECK(cpu.cycles() == 18); CHECK(timer0.counter() == 0);
    cpu.step(); // Pin update happens here
    CHECK((timer0.interrupt_flags() & 0x02) != 0);
}

TEST_CASE("Timer8: Fast PWM Mode Precise") {
    MemoryBus bus {devices::atmega328p};
    // Infer port index from name (e.g., "PORTB" -> 1)
    vioavr::core::PinMux pm { 10 }; 
    Timer8 timer0 {"TIMER0", devices::atmega328p.timers8[0], &pm};
    bus.attach_peripheral(timer0);
    GpioPort portb { "PORTB", 0x23, 0x24, 0x25, pm };
    bus.attach_peripheral(portb);
    timer0.connect_compare_output_a(portb, 6); 
    AvrCpu cpu {bus};

    std::vector<u16> program = {
        0xE005, // LDI r16, 5
        0x9300, 0x0047, // STS OCR0A, r16
        0xE803, // LDI r16, 0x83 (Fast PWM Mode 3, clear on match)
        0x9300, 0x0044, // STS TCCR0A, r16
        0xE001, // LDI r16, 0x01
        0x9300, 0x0045, // STS TCCR0B, r16
    };
    for (int i = 0; i < 300; ++i) program.push_back(0x0000);

    bus.load_flash(program);
    cpu.reset();
    run_to_cycle(cpu, 9);
    CHECK(timer0.counter() == 2);

    portb.write(portb.port_address(), 0x40);

    cpu.step(); // C10: 3
    cpu.step(); // C11: 4
    cpu.step(); // C12: 5 (Match)
    CHECK(timer0.counter() == 5);
    cpu.step(); // Match pin update happens here
    CHECK((portb.read(0x23) & 0x40) == 0); // Read PINB

    // Run 249 more NOPs to reach 255
    for (int i = 0; i < 249; ++i) cpu.step();
    CHECK(timer0.counter() == 255);
    
    cpu.step(); // C263: Wrap to 0
    CHECK(timer0.counter() == 0);
    cpu.step(); // Pin update happens here
    CHECK((portb.read(0x23) & 0x40) != 0); // Read PINB
}

TEST_CASE("Timer8: Phase Correct PWM Precise") {
    MemoryBus bus {devices::atmega328p};
    Timer8 timer0 {"TIMER0", devices::atmega328p.timers8[0]};
    bus.attach_peripheral(timer0);
    AvrCpu cpu {bus};

    std::vector<u16> program = {
        0xE003, // LDI r16, 3
        0x9300, 0x0047, // STS OCR0A, r16
        0xE001, // LDI r16, 0x01
        0x9300, 0x0044, // STS TCCR0A, r16
        0xE009, // LDI r16, 0x09 (WGM02=1, CS00=1 -> Mode 5)
        0x9300, 0x0045, // STS TCCR0B, r16
    };
    for (int i = 0; i < 20; ++i) program.push_back(0x0000);

    bus.load_flash(program);
    cpu.reset();
    run_to_cycle(cpu, 9);
    CHECK(timer0.counter() == 2);

    // C10: hits 3, stays 3? No, my code becomes 3 then immediate 2 if >= 3.
    // Let's check:
    /*
    if (counting_up_) {
        if (tcnt_ >= top) {
            counting_up_ = false;
            tcnt_--;
        } else {
            tcnt_++;
        }
    }
    */
    // C10: tcnt was 2. counting_up is true. tcnt becomes 3. 
    // BUT! handle_compare_match is called AFTER perform_tick.
    // And get_top() is called inside perform_tick.
    
    cpu.step(); 
    CHECK(timer0.counter() == 3);
    
    cpu.step();
    // C11: tcnt was 3. counting_up was false (from previous step). 3 -> 2.
    CHECK(timer0.counter() == 2);
    
    cpu.step(); // C12: 1
    CHECK(timer0.counter() == 1);
    
    cpu.step(); 
    // C13: tcnt was 1. counting_up is false. tcnt = 0.
    CHECK(timer0.counter() == 0);
    
    cpu.step();
    // C14: tcnt was 0. counting_up is false. counting_up = true. handle_overflow. tcnt = 1.
    CHECK(timer0.counter() == 1);
    cpu.step(); // Interrupt flag set and observable
    CHECK((timer0.interrupt_flags() & 0x01) != 0);
}
