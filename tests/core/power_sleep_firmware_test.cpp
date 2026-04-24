#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 r, const u8 k) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(k & 0xF0U)) << 4U) | (static_cast<u16>(r - 16U) << 4U) | (k & 0x0FU));
}

constexpr u16 encode_sts(const u8 r) {
    return static_cast<u16>(0x9200U | (static_cast<u16>(r) << 4U));
}

constexpr u16 encode_lds(const u8 r) {
    return static_cast<u16>(0x9000U | (static_cast<u16>(r) << 4U));
}

constexpr u16 encode_sei() { return 0x9478U; }
constexpr u16 encode_sleep() { return 0x9588U; }
constexpr u16 encode_rjmp(const i16 offset) { return static_cast<u16>(0xC000U | (static_cast<u16>(offset) & 0x0FFFU)); }
constexpr u16 encode_nop() { return 0x0000U; }

} // namespace

TEST_CASE("Power and Sleep Firmware Test") {
    using namespace vioavr::core::devices;

    MemoryBus bus{atmega328p};
    AvrCpu cpu{bus};
    
    // Peripherals
    Timer16 timer1{"Timer1", atmega328p.timers16[0]};
    bus.attach_peripheral(timer1);
    
    PinMux pin_mux(3);
    ExtInterrupt int0_peri{"INT0", atmega328p.ext_interrupts[0], pin_mux, 0};
    bus.attach_peripheral(int0_peri);

    SUBCASE("PRR: Peripheral Gating Firmware") {
        // Program:
        // 0: LDI R16, 1 (Clock 1:1)
        // 1: STS TCCR1B, R16
        // 3: NOP
        // 4: NOP
        // 5: LDI R16, 0x08 (PRTIM1)
        // 6: STS PRR, R16
        // 8: NOP
        
        bus.load_image(HexImage {
            .flash_words = {
                encode_ldi(16, 0x01),
                encode_sts(16), atmega328p.timers16[0].tccrb_address,
                encode_nop(),
                encode_nop(),
                encode_ldi(16, 0x08),
                encode_sts(16), atmega328p.prr_address,
                encode_nop(),
                encode_nop()
            },
            .entry_word = 0U
        });
        
        cpu.reset();
        
        // 1. Enable Timer
        cpu.step(); // LDI
        cpu.step(); // STS TCCR1B
        CHECK(bus.read_data(atmega328p.timers16[0].tccrb_address) == 0x01);
        
        // 2. Wait and verify it ticks
        cpu.step(); // NOP
        cpu.step(); // NOP
        CHECK(bus.read_data(atmega328p.timers16[0].tcnt_address) > 0);
        // 3. Disable via PRR
        cpu.step(); // LDI R16, 0x08
        u8 tcnt_before = bus.read_data(atmega328p.timers16[0].tcnt_address);
        cpu.step(); // STS PRR
        CHECK(bus.read_data(atmega328p.prr_address) == 0x08);
        
        // 4. Wait and verify it STOPS
        cpu.step(); // NOP
        cpu.step(); // NOP
        CHECK(bus.read_data(atmega328p.timers16[0].tcnt_address) == tcnt_before);
    }

    SUBCASE("Sleep: Power Down and INT0 Wake Firmware") {
        // Program:
        // Word 0: RJMP START (4)
        // Word 1: NOP
        // Word 2: INT0 vector: RJMP ISR (14)
        // Word 3: NOP
        // Word 4: START: LDI R16, 1
        // Word 5,6: STS EIMSK, R16
        // Word 7: LDI R16, smcr_val
        // Word 8,9: STS SMCR, R16
        // Word 10: SEI
        // Word 11: SLEEP
        // Word 12: LDI R20, 0xAA
        // Word 13: RJMP 13
        // Word 14: ISR: LDI R21, 0x55
        // Word 15: RETI
        
        u8 smcr_val = 0x01 | (2 << 1); // SE=1, SM=010 (Power Down)
        
        bus.load_image(HexImage {
            .flash_words = {
                encode_rjmp(3),          // 0: RJMP 4 (START)
                encode_nop(),           // 1: 
                encode_rjmp(11),         // 2: RJMP 14 (ISR) - Vector 1
                encode_nop(),           // 3: 
                encode_ldi(16, 0x01),    // 4: START
                encode_sts(16), 0x3DU,   // 5: EIMSK
                encode_ldi(16, smcr_val), // 7:
                encode_sts(16), 0x53U,   // 8: SMCR
                encode_sei(),           // 10:
                encode_sleep(),         // 11:
                encode_ldi(20, 0xAA),   // 12:
                encode_rjmp(-1),        // 13:
                encode_ldi(21, 0x55),   // 14: ISR
                0x9508U                 // 15: RETI
            },
            .entry_word = 0U
        });
        
        cpu.reset();
        
        // Run until SLEEP
        // 4 (START) + 2 (STS) + 1 (LDI) + 2 (STS) + 1 (SEI) + 1 (SLEEP) = 11 steps? 
        // No, let's just run until state is sleeping.
        while(cpu.state() != CpuState::sleeping && cpu.cycles() < 100) {
            cpu.step();
        }
        
        CHECK(cpu.state() == CpuState::sleeping);
        
        // 2. Inject INT0
        int0_peri.set_int0_level(false); // Low level triggers INT0 on 328P by default if EICRA=0
        
        // 3. Wake up
        cpu.step(); // Wake and jump to vector
        CHECK(cpu.state() == CpuState::running);
        CHECK(cpu.program_counter() == 2); // INT0 vector
        
        // 4. Run ISR
        cpu.step(); // RJMP 14
        cpu.step(); // LDI R21, 0x55
        CHECK(cpu.snapshot().gpr[21] == 0x55);
        cpu.step(); // RETI
        
        // 5. Back to main loop
        CHECK(cpu.program_counter() == 12);
        cpu.step(); // LDI R20, 0xAA
        CHECK(cpu.snapshot().gpr[20] == 0xAA);
    }
}
