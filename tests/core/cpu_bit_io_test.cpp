#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_in(const u8 destination, const u8 io_offset) {
    return static_cast<u16>(0xB000U | ((static_cast<u16>(destination) & 0x1FU) << 4U) | ((static_cast<u16>(io_offset) & 0x30U) << 5U) | (io_offset & 0x0FU));
}

constexpr u16 encode_sts(const u8 source) {
    return static_cast<u16>(0x9200U | (static_cast<u16>(source) << 4U));
}

constexpr u16 encode_cbi(const u8 io_offset, const u8 bit) {
    return static_cast<u16>(0x9800U | (static_cast<u16>(io_offset) << 3U) | (bit & 0x07U));
}

constexpr u16 encode_sbic(const u8 io_offset, const u8 bit) {
    return static_cast<u16>(0x9900U | (static_cast<u16>(io_offset) << 3U) | (bit & 0x07U));
}

constexpr u16 encode_sbi(const u8 io_offset, const u8 bit) {
    return static_cast<u16>(0x9A00U | (static_cast<u16>(io_offset) << 3U) | (bit & 0x07U));
}

constexpr u16 encode_sbis(const u8 io_offset, const u8 bit) {
    return static_cast<u16>(0x9B00U | (static_cast<u16>(io_offset) << 3U) | (bit & 0x07U));
}

constexpr u16 encode_sbrc(const u8 destination, const u8 bit) {
    return static_cast<u16>(0xFC00U | (static_cast<u16>(destination) << 4U) | (bit & 0x07U));
}

constexpr u16 encode_sbrs(const u8 destination, const u8 bit) {
    return static_cast<u16>(0xFE00U | (static_cast<u16>(destination) << 4U) | (bit & 0x07U));
}

void step_to(AvrCpu& cpu, u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) {
        cpu.step();
    }
}

} // namespace

TEST_CASE("CPU Bit and I/O Instruction Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    constexpr auto pinb = static_cast<u16>(0x23U);
    constexpr auto ddrb = static_cast<u16>(0x24U);
    constexpr auto portb = static_cast<u16>(0x25U);

    MemoryBus bus {atmega328};
    GpioPort port_b {"PORTB", pinb, ddrb, portb};
    bus.attach_peripheral(port_b);
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x00U),                     // 0
            encode_sbi(0x04U, 0U),                      // 1: DDRB0 = output
            encode_sbi(0x05U, 0U),                      // 2: PORTB0 = high
            encode_sbis(0x03U, 0U),                     // 3: PINB0 set? skip 2-word STS
            encode_sts(16U), portb,                     // 4, 5 (Skipped)
            encode_in(17U, 0x05U),                      // 6: PORTB -> r17
            encode_cbi(0x05U, 0U),                      // 7: PORTB0 = low
            encode_sbic(0x03U, 0U),                     // 8: PINB0 clear? skip 2-word STS
            encode_sts(16U), portb,                     // 9, 10 (Skipped)
            encode_in(18U, 0x05U),                      // 11: PORTB -> r18
            encode_sbic(0x03U, 2U),                     // 12: PB2 low? skip 1-word SBI
            encode_sbi(0x05U, 2U),                      // 13 (Skipped)
            encode_sbis(0x03U, 3U),                     // 14: PB3 high? skip 1-word SBI
            encode_sbi(0x05U, 3U),                      // 15 (Skipped)
            encode_sbrs(20U, 3U),                       // 16: r20.3 set? skip 2-word STS
            encode_sts(16U), portb,                     // 17, 18 (Skipped)
            encode_sbrc(20U, 2U),                       // 19: r20.2 clear? skip 1-word LDI
            encode_ldi(21U, 0xAAU),                     // 20 (Skipped)
            encode_in(19U, 0x05U),                      // 21: PORTB -> r19
            encode_in(20U, 0x03U),                      // 22: PINB -> r20
            0x0000U
        },
        .entry_word = 0U
    });

    cpu.reset();
    port_b.set_input_levels(0x08U); // External PB3 is HIGH

    SUBCASE("Execution and Verification") {
        step_to(cpu, 100U); // Run until halt
        
        auto s = cpu.snapshot();
        // DDRB should be 0x01 (bit 0 output)
        CHECK(port_b.ddr_register() == 0x01U);
        // PORTB should be 0x00 (bit 0 was high then low)
        CHECK(port_b.port_register() == 0x00U);
        
        CHECK(s.gpr[17] == 0x01U); // First IN PORTB
        CHECK(s.gpr[18] == 0x00U); // Second IN PORTB
        CHECK(s.gpr[19] == 0x00U); // Third IN PORTB
        CHECK(s.gpr[20] == 0x08U); // Last IN PINB (reflects input levels)
        CHECK(s.gpr[21] == 0x00U); // Skipped LDI
        
        CHECK(port_b.output_levels() == 0x00U);
        CHECK(cpu.state() == CpuState::halted);
    }
}
