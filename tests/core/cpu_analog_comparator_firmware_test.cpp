#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
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

void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
} // namespace

using namespace vioavr::core;
TEST_CASE("Analog Comparator Firmware Interrupt Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    constexpr u16 acsr_addr = 0x50U;
    constexpr u8 comparator_vector = 8U;

    MemoryBus bus {atmega328};
    AnalogComparator comparator {"AC", acsr_addr, comparator_vector, 9U, 0.04};
    comparator.set_negative_input_voltage(0.80);
    comparator.set_positive_input_voltage(0.20);
    bus.attach_peripheral(comparator);
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            kNop,                     // 0
            kNop,                     // 1
            kNop,                     // 2
            kNop,                     // 3
            kNop,                     // 4
            kNop,                     // 5
            kNop,                     // 6
            kNop,                     // 7
            encode_ldi(19U, 0xA5U),   // 8: ISR body (Vector 8)
            kReti,                    // 9
            encode_ldi(16U, 0x0BU),   // 10: Entry point: ACIE | rising edge
            encode_sts(16U), acsr_addr, // 11, 12: Write to ACSR
            kSei,                     // 13: Enable interrupts
            kNop,                     // 14: Point to be interrupted
            encode_ldi(18U, 0x55U),   // 15: Mainline after ISR
            kNop                      // 16
        },
        .entry_word = 10U
    });
    cpu.reset();

    SUBCASE("Interrupt Trigger and ISR Execution") {
        step_to(cpu, 4U);
        auto s = cpu.snapshot();
        CHECK(s.program_counter == 14U);
        CHECK((s.sreg & (1U << static_cast<u8>(SregFlag::interrupt))) != 0U);

        // Trigger comparator
        comparator.set_positive_input_voltage(0.83);

        // Next step should enter ISR
        cpu.step();
        s = cpu.snapshot();
        CHECK(s.program_counter == comparator_vector); // PC=8
        CHECK(s.stack_pointer == static_cast<u16>(atmega328.sram_range().end - 2U));
        CHECK(s.in_interrupt_handler);

        // Step through ISR body
        cpu.step(); // LDI r19, 0xA5
        CHECK(cpu.snapshot().gpr[19] == 0xA5U);

        cpu.step(); // RETI
        s = cpu.snapshot();
        CHECK(s.program_counter == 14U); // Back to NOP
        CHECK(s.stack_pointer == atmega328.sram_range().end);
        CHECK_FALSE(s.in_interrupt_handler);

        // Mainline continues
        cpu.step(); // NOP at 14
        cpu.step(); // LDI r18, 0x55
        CHECK(cpu.snapshot().gpr[18] == 0x55U);
        
        // ACI should be cleared by hardware on ISR entry
        CHECK((bus.read_data(acsr_addr) & 0x10U) == 0U);
    }
}
