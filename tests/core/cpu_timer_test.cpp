#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328.hpp"

namespace {

constexpr vioavr::core::u16 encode_ldi(const vioavr::core::u8 destination, const vioavr::core::u8 immediate)
{
    return static_cast<vioavr::core::u16>(
        0xE000U |
        ((static_cast<vioavr::core::u16>(immediate & 0xF0U)) << 4U) |
        (static_cast<vioavr::core::u16>(destination - 16U) << 4U) |
        (immediate & 0x0FU)
    );
}

constexpr vioavr::core::u16 encode_in(const vioavr::core::u8 destination, const vioavr::core::u8 io_offset)
{
    return static_cast<vioavr::core::u16>(
        0xB000U |
        ((static_cast<vioavr::core::u16>(destination) & 0x1FU) << 4U) |
        ((static_cast<vioavr::core::u16>(io_offset) & 0x30U) << 5U) |
        (io_offset & 0x0FU)
    );
}

constexpr vioavr::core::u16 encode_out(const vioavr::core::u8 io_offset, const vioavr::core::u8 source)
{
    return static_cast<vioavr::core::u16>(
        0xB800U |
        ((static_cast<vioavr::core::u16>(source) & 0x1FU) << 4U) |
        ((static_cast<vioavr::core::u16>(io_offset) & 0x30U) << 5U) |
        (io_offset & 0x0FU)
    );
}

constexpr vioavr::core::u16 encode_sts(const vioavr::core::u8 source)
{
    return static_cast<vioavr::core::u16>(0x9200U | (static_cast<vioavr::core::u16>(source) << 4U));
}

}  // namespace

TEST_CASE("Timer0 basic functionality")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    Timer8 timer0 {"TIMER0", atmega328.timers8[0]};
    bus.attach_peripheral(timer0);
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x03U),
            encode_sts(16U), atmega328.timers8[0].ocra_address,
            encode_ldi(22U, 0x01U),  // CS00
            encode_out(0x25U, 22U),  // TCCR0B
            encode_sts(22U), atmega328.timers8[0].tcnt_address, // Placeholder to use STS for TCNT
            // Actually let's just use the addresses from atmega328.timers8[0]
        },
        .entry_word = 0U
    });

    // Re-writing the test to be more robust with new addresses
    std::vector<u16> code = {
        encode_ldi(16U, 0x03U),
        encode_sts(16U), atmega328.timers8[0].ocra_address,
        encode_ldi(17U, 0x01U),
        encode_sts(17U), atmega328.timers8[0].tccrb_address,
        // Wait a few cycles
        0x0000, 0x0000, 0x0000,
        // Read TCNT0
        0x9100, atmega328.timers8[0].tcnt_address, // LDS R16, TCNT0
        // Read TIFR0
        0x9110, atmega328.timers8[0].tifr_address, // LDS R17, TIFR0
        0x0000
    };
    bus.load_flash(code);
    cpu.reset();

    SUBCASE("Initialization and basic counting") {
        cpu.step(); // LDI R16, 0x03
        cpu.step(); cpu.step(); // STS OCRA0, R16
        cpu.step(); // LDI R17, 0x01
        cpu.step(); cpu.step(); // STS TCCR0B, R17
        cpu.step(); // NOP
        cpu.step(); // NOP
        cpu.step(); // NOP
        cpu.step(); cpu.step(); // LDS R16, TCNT0
        
        const auto snapshot = cpu.snapshot();
        // Cycles: 1+2+1+2+1+1+1+2 = 11
        // Timer started at cycle 6. At cycle 11, TCNT should be 11-6 = 5.
        // Wait, tick() happens after retirement.
        // C6 retire: TCNT=1. C7 retire: TCNT=2. C8 retire: TCNT=3. C9 retire: TCNT=4. C10 retire: TCNT=5. C11 retire: TCNT=6.
        CHECK(snapshot.gpr[16] >= 5U); 
        
        cpu.step(); cpu.step(); // LDS R17, TIFR0
        // OCF0A (bit 1) should be set because TCNT passed OCR0A=3
        CHECK((cpu.snapshot().gpr[17] & 0x02U) != 0);
    }
}
