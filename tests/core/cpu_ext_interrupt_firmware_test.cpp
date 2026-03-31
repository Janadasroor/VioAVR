#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_sts(const u8 source) {
    return static_cast<u16>(0x9200U | (static_cast<u16>(source) << 4U));
}

constexpr u16 kSei = 0x9478U;
constexpr u16 kReti = 0x9518U;
constexpr u16 kNop = 0x0000U;

} // namespace

TEST_CASE("External Interrupt (INT0) Firmware Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    constexpr u8 int0_vector = 2U;

    MemoryBus bus {atmega328};
    ExtInterrupt exti {"EXTINT", atmega328, 4U};
    bus.attach_peripheral(exti);
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            kNop,                     // 0
            kNop,                     // 1
            encode_ldi(19U, 0xA5U),   // 2: ISR body (Vector 2)
            kReti,                    // 3
            kNop,                     // 4
            encode_ldi(16U, 0x02U),   // 5: Entry point: EICRA = falling edge sense
            encode_sts(16U), atmega328.ext_interrupt.eicra_address, // 6, 7
            encode_ldi(17U, 0x01U),   // 8: EIMSK = enable INT0
            encode_sts(17U), atmega328.ext_interrupt.eimsk_address, // 9, 10
            kSei,                     // 11: Enable global interrupts
            kNop,                     // 12: Interrupted point
            encode_ldi(18U, 0x55U),   // 13: Mainline after ISR
            kNop                      // 14
        },
        .entry_word = 5U
    });
    cpu.reset();

    SUBCASE("INT0 Falling Edge Trigger and ISR") {
        cpu.run(5); // Run through initialization until PC=12 (kNop after SEI)
        auto s = cpu.snapshot();
        CHECK(s.program_counter == 12U);
        CHECK((s.sreg & (1U << static_cast<u8>(SregFlag::interrupt))) != 0U);

        // Trigger INT0 falling edge
        exti.set_int0_level(true);
        exti.set_int0_level(false);

        // Next step should enter ISR
        cpu.step();
        s = cpu.snapshot();
        CHECK(s.program_counter == int0_vector);
        CHECK(s.stack_pointer == static_cast<u16>(atmega328.sram_range().end - 2U));
        CHECK(s.in_interrupt_handler);

        // Step through ISR body
        cpu.step(); // LDI r19, 0xA5
        CHECK(cpu.snapshot().gpr[19] == 0xA5U);

        cpu.step(); // RETI
        s = cpu.snapshot();
        CHECK(s.program_counter == 12U);
        CHECK(s.stack_pointer == atmega328.sram_range().end);
        CHECK_FALSE(s.in_interrupt_handler);

        // Mainline continues
        cpu.step(); // kNop at 12
        cpu.step(); // LDI r18, 0x55
        CHECK(cpu.snapshot().gpr[18] == 0x55U);
        
        // EIFR should be cleared by hardware on ISR entry
        CHECK(bus.read_data(atmega328.ext_interrupt.eifr_address) == 0x00U);
    }
}
