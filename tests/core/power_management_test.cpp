#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

using namespace vioavr::core;

TEST_CASE("Power: Idle Mode Wake") {
    MemoryBus bus {devices::atmega32u4};
    AvrCpu cpu {bus};
    Timer8 timer0 {"TIMER0", devices::atmega32u4.timers8[0]};
    bus.attach_peripheral(timer0);
    
    // Program:
    // 0: SEI
    // 1: SLEEP
    // 2: NOP
    // 3: RJMP -1
    bus.load_image(HexImage {
        .flash_words = { 0x9478, 0x9588, 0x0000, 0x0000, 0xCFFF },
        .entry_word = 0
    });
    
    cpu.reset(); 
    
    bus.write_data(0x53, 0x01); // SMCR: SE=1, Idle
    bus.write_data(0x46, 0x80); // TCNT = 128
    bus.write_data(0x6E, 0x01); // TOIE0 = 1
    bus.write_data(0x45, 0x01); // CS=1
    
    cpu.step(); // SEI (TCNT 128 -> 129)
    cpu.step(); // SLEEP (TCNT 129 -> 130)
    CHECK(cpu.snapshot().state == CpuState::sleeping);
    
    // Tick to overflow (126 more cycles)
    for(int i=0; i<125; ++i) {
        cpu.step();
        CHECK(cpu.snapshot().state == CpuState::sleeping);
    }
    
    cpu.step(); // Overflow!
    
    auto s = cpu.snapshot();
    CHECK(s.state == CpuState::running);
}

TEST_CASE("Power: Sleep without GIE") {
    MemoryBus bus {devices::atmega32u4};
    AvrCpu cpu {bus};
    Timer8 timer0 {"TIMER0", devices::atmega32u4.timers8[0]};
    bus.attach_peripheral(timer0);
    
    bus.load_image(HexImage {
        .flash_words = { 0x94F8, 0x9588, 0x0000, 0x0000, 0xCFFF },
        .entry_word = 0
    });
    
    cpu.reset(); 
    
    bus.write_data(0x53, 0x01); 
    bus.write_data(0x46, 0x80); 
    bus.write_data(0x6E, 0x01); 
    bus.write_data(0x45, 0x01); 
    
    cpu.step(); // CLI
    cpu.step(); // SLEEP
    CHECK(cpu.snapshot().state == CpuState::sleeping);
    
    for(int i=0; i<125; ++i) {
        cpu.step();
    }
    
    cpu.step(); // Overflow!
    
    auto s = cpu.snapshot();
    CHECK(s.state == CpuState::running);
    CHECK(s.program_counter == 2); 
}

TEST_CASE("Power: Power Down Mode") {
    MemoryBus bus {devices::atmega32u4};
    AvrCpu cpu {bus};
    Timer8 timer0 {"TIMER0", devices::atmega32u4.timers8[0]};
    bus.attach_peripheral(timer0);
    
    bus.load_image(HexImage {
        .flash_words = { 0x9478, 0x9588, 0x0000, 0x0000, 0xCFFF },
        .entry_word = 0
    });
    
    cpu.reset(); 
    
    bus.write_data(0x53, 0x05); // Power Down
    bus.write_data(0x46, 0x80); 
    bus.write_data(0x6E, 0x01); 
    bus.write_data(0x45, 0x01); 
    
    cpu.step(); // SEI
    cpu.step(); // SLEEP
    CHECK(cpu.snapshot().state == CpuState::sleeping);
    
    for(int i=0; i<100; ++i) {
        cpu.step();
        CHECK(cpu.snapshot().state == CpuState::sleeping);
    }
    
    // TCNT should still be 130 (128 + 1 (SEI) + 1 (SLEEP))
    // Because Timer0 clock is gated in Power Down
    CHECK(bus.read_data(0x46) == 130);
}
