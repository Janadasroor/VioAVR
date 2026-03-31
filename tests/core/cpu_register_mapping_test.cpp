#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_lds(const u8 destination) {
    // Word 1: 1001 000d dddd 0000 (LDS Rd, k)
    return static_cast<u16>(0x9000U | (static_cast<u16>(destination) << 4U));
}

constexpr u16 encode_sts(const u8 source) {
    // Word 1: 1001 001d dddd 0000 (STS k, Rr)
    return static_cast<u16>(0x9200U | (static_cast<u16>(source) << 4U));
}

} // namespace

TEST_CASE("CPU GPR Data-Space Mapping Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};

    // 0x0010 is the address of R16 in data space
    // 0x0011 is the address of R17 in data space

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16, 0xAA),
            encode_lds(18), 0x0010U, // LDS R18, 0x10 (should load R16)
            encode_ldi(19, 0x55),
            encode_sts(19), 0x0011U, // STS 0x11, R19 (should store to R17)
            0x0000U
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("Read register via data-space address") {
        cpu.step(); // LDI R16, 0xAA
        cpu.step(); // LDS R18, 0x10 (2 words, 2 cycles)
        
        CHECK(cpu.registers()[16] == 0xAAU);
        CHECK(cpu.registers()[18] == 0xAAU);
    }

    SUBCASE("Write register via data-space address") {
        cpu.run(2); // Skip LDI R16, LDS R18
        cpu.step(); // LDI R19, 0x55
        cpu.step(); // STS 0x11, R19 (2 words, 2 cycles)
        
        CHECK(cpu.registers()[19] == 0x55U);
        CHECK(cpu.registers()[17] == 0x55U);
    }
}
