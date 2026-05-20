#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/spi.hpp"
#include "vioavr/core/twi.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

#include <vector>

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("Reset State Verification")
{
    MemoryBus bus {atmega328p};
    Timer8 timer0 {"TIMER0", atmega328p.timers8[0]};
    Timer16 timer1 {"TIMER1", atmega328p.timers16[0]};
    Timer8 timer2 {"TIMER2", atmega328p.timers8[1]};
    PinMux mux{1}; Uart uart {"UART", atmega328p.uarts[0], mux};
    Spi spi {"SPI", atmega328p.spis[0]};
    Twi twi {"TWI", atmega328p.twis[0]};

    bus.attach_peripheral(timer0);
    bus.attach_peripheral(timer1);
    bus.attach_peripheral(timer2);
    bus.attach_peripheral(uart);
    bus.attach_peripheral(spi);
    bus.attach_peripheral(twi);

    AvrCpu cpu {bus};

    // Load minimal program to keep CPU running
    bus.load_image(HexImage {
        .flash_words = {0x0000U}, // NOP at reset vector
        .entry_word = 0U
    });

    cpu.reset();

    SUBCASE("Program Counter at Reset Vector") {
        CHECK(cpu.program_counter() == 0U);
    }

    SUBCASE("Stack Pointer at RAMEND") {
        // ATmega328P has 2KB SRAM, RAMEND = 0x08FF
        const u16 ramend = atmega328p.sram_range().end;
        CHECK(cpu.stack_pointer() == ramend);
        CHECK(ramend == 0x08FFU);
    }

    SUBCASE("SREG Cleared After Reset") {
        // All flags cleared, including I-bit (global interrupt enable)
        CHECK(cpu.sreg() == 0x00U);
        // I-bit must be cleared to prevent spurious interrupts
        CHECK((cpu.sreg() & 0x80U) == 0U);
    }

    SUBCASE("General Purpose Registers Zeroed") {
        for (size_t i = 0; i < cpu.registers().size(); ++i) {
            CHECK(cpu.registers()[i] == 0U);
        }
    }

    SUBCASE("CPU State Running After Reset") {
        CHECK(cpu.state() == CpuState::running);
    }

    SUBCASE("No Pending Interrupts After Reset") {
        CHECK_FALSE(cpu.snapshot().interrupt_pending);
        CHECK_FALSE(cpu.snapshot().in_interrupt_handler);
    }

    SUBCASE("Timer0 Reset State") {
        // TCNT, TCCR, OCR should be zeroed by reset
        CHECK(bus.read_data(atmega328p.timers8[0].tcnt_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers8[0].tccra_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers8[0].tccrb_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers8[0].timsk_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers8[0].tifr_address) == 0U);
    }

    SUBCASE("Timer1 Reset State") {
        CHECK(bus.read_data(atmega328p.timers16[0].tcnt_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers16[0].tcnt_address + 1U) == 0U);
        CHECK(bus.read_data(atmega328p.timers16[0].tccra_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers16[0].tccrb_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers16[0].timsk_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers16[0].tifr_address) == 0U);
    }

    SUBCASE("Timer2 Reset State") {
        CHECK(bus.read_data(atmega328p.timers8[1].tcnt_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers8[1].tccra_address) == 0U);
        CHECK(bus.read_data(atmega328p.timers8[1].tccrb_address) == 0U);
    }

    SUBCASE("UART0 Reset State") {
        // UCSRA: UDRE (bit 5) should be set at reset (TX buffer empty)
        CHECK((bus.read_data(atmega328p.uarts[0].ucsra_address) & 0x20U) != 0U);
        // UCSRB: all interrupts disabled
        CHECK(bus.read_data(atmega328p.uarts[0].ucsrb_address) == 0U);
        // UDR should be zero
        CHECK(bus.read_data(atmega328p.uarts[0].udr_address) == 0U);
    }

    SUBCASE("External Interrupt Reset State") {
        // EIMSK: no external interrupts enabled
        CHECK(bus.read_data(atmega328p.ext_interrupts[0].eimsk_address) == 0U);
        // EIFR: no pending flags
        CHECK(bus.read_data(atmega328p.ext_interrupts[0].eifr_address) == 0U);
    }
}

TEST_CASE("SEI/RETI Interrupt Latency Verification")
{
    // Per ATmega328P datasheet:
    // - After SEI: one instruction executes before interrupts are serviced
    // - After RETI: one instruction executes before next interrupt is serviced

    MemoryBus bus {atmega328p};
    Timer8 timer0 {"TIMER0", atmega328p.timers8[0]};
    bus.attach_peripheral(timer0);

    constexpr u16 kSei = 0x9478U;
    constexpr u16 kReti = 0x9518U;
    constexpr u16 kNop = 0x0000U;

    auto encode_ldi = [](u8 rd, u8 imm) -> u16 {
        return static_cast<u16>(0xE000U | ((imm & 0xF0U) << 4U) | ((rd - 16U) << 4U) | (imm & 0x0FU));
    };

    constexpr u16 isr_word = static_cast<u16>(atmega328p.timers8[0].compare_a_vector_index * 2);

    // Layout: mainline at 0-7, ISR at vector address
    std::vector<u16> flash(32, kNop);
    flash[0] = kSei;                                      // Enable interrupts
    flash[1] = encode_ldi(19, 0xAA);                      // Post-SEI instruction (must execute first)
    flash[2] = encode_ldi(20, 0xBB);                      // Second post-SEI instruction
    flash[static_cast<size_t>(isr_word)] = encode_ldi(21, 0xCC); // ISR marker
    flash[static_cast<size_t>(isr_word) + 1] = kReti;

    bus.load_image(HexImage {
        .flash_words = std::move(flash),
        .entry_word = 0U
    });

    AvrCpu cpu {bus};
    cpu.reset();

    // Set up timer for immediate compare match after tick
    // TCNT=254, OCR=255: next tick will increment TCNT to 255 == OCR, triggering match
    bus.write_data(atmega328p.timers8[0].timsk_address, 0x02);    // OCIE0A = 1 (enable interrupt)
    bus.write_data(atmega328p.timers8[0].ocra_address, 0xFF);     // OCR0A = 255
    bus.write_data(atmega328p.timers8[0].tcnt_address, 0xFE);     // TCNT0 = 254
    bus.write_data(atmega328p.timers8[0].tccrb_address, 0x01);    // CS00 = 1 (no prescaler)
    bus.tick_peripherals(1);                                 // Tick once: TCNT->255 == OCR, sets OCF0A

    SUBCASE("SEI One-Instruction Delay") {
        // Step 1: SEI executes. Interrupt pending but I-bit was 0, so no service yet.
        // After SEI: I-bit=1, interrupt_delay_=1, PC=1
        cpu.step();
        CHECK(cpu.program_counter() == 1U);
        CHECK((cpu.sreg() & 0x80U) != 0U); // I-bit set
        CHECK(cpu.registers()[19] == 0U);  // R19 not yet set

        // Step 2: interrupt_delay_=1, decrement to 0, execute LDI R19, 0xAA
        // After: R19=0xAA, PC=2
        cpu.step();
        CHECK(cpu.registers()[19] == 0xAAU); // Post-SEI instruction executed
        CHECK(cpu.program_counter() == 2U);

        // Step 3: interrupt_delay_=0, service interrupt, jump to ISR
        // After: PC=isr_word, I-bit=0, return address pushed
        cpu.step();
        CHECK(cpu.program_counter() == isr_word);
        CHECK(cpu.snapshot().in_interrupt_handler);
        CHECK((cpu.sreg() & 0x80U) == 0U); // I-bit cleared on ISR entry
    }

    SUBCASE("RETI One-Instruction Delay") {
        // Get to ISR: SEI + post-SEI + service interrupt
        cpu.step(); // SEI: PC=1, I=1, delay=1
        cpu.step(); // LDI R19: PC=2, R19=0xAA
        cpu.step(); // Service interrupt: PC=isr_word

        // Execute ISR: LDI R21, 0xCC
        cpu.step();
        CHECK(cpu.registers()[21] == 0xCCU);
        CHECK(cpu.program_counter() == static_cast<u16>(isr_word + 1U));

        // RETI: restores PC=2, sets I-bit with delay
        cpu.step();
        CHECK((cpu.sreg() & 0x80U) != 0U); // I-bit restored
        CHECK(cpu.program_counter() == 2U);
        CHECK_FALSE(cpu.snapshot().in_interrupt_handler);

        // Post-RETI instruction should execute before next interrupt
        cpu.step();
        CHECK(cpu.registers()[20] == 0xBBU);
    }
}

TEST_CASE("Interrupt Vector Address Table Verification")
{
    // Verify that each interrupt vector is at the correct word address
    // Per ATmega328P datasheet: each vector is 2 words (4 bytes) apart

    SUBCASE("Vector 0 (RESET) at word 0x0000") {
        CHECK(0U * (atmega328p.interrupt_vector_size / 2U) == 0U);
    }

    SUBCASE("Vector 1 (INT0) at word 0x0002") {
        CHECK(1U * (atmega328p.interrupt_vector_size / 2U) == 2U);
    }

    SUBCASE("Vector 2 (INT1) at word 0x0004") {
        CHECK(2U * (atmega328p.interrupt_vector_size / 2U) == 4U);
    }

    SUBCASE("Vector 6 (WDT) at word 0x000C") {
        CHECK(6U * (atmega328p.interrupt_vector_size / 2U) == 12U);
    }

    SUBCASE("Vector 14 (TIMER0_COMPA) at word 0x001C") {
        CHECK(14U * (atmega328p.interrupt_vector_size / 2U) == 28U);
    }

    SUBCASE("Vector 18 (USART_RX) at word 0x0024") {
        CHECK(18U * (atmega328p.interrupt_vector_size / 2U) == 36U);
    }

    SUBCASE("Vector 23 (ANALOG_COMP) at word 0x002E") {
        CHECK(23U * (atmega328p.interrupt_vector_size / 2U) == 46U);
    }

    SUBCASE("Vector 25 (SPM_READY) at word 0x0032") {
        CHECK(25U * (atmega328p.interrupt_vector_size / 2U) == 50U);
    }

    SUBCASE("All vectors fit within flash") {
        const u16 last_vector_word = static_cast<u16>(
            (atmega328p.interrupt_vector_count - 1U) * (atmega328p.interrupt_vector_size / 2U));
        CHECK(last_vector_word < atmega328p.flash_words);
    }
}
