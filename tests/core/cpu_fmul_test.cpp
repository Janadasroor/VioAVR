#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
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

constexpr u16 encode_fmul(const u8 destination, const u8 source) {
    return static_cast<u16>(0x0308U | ((static_cast<u16>(destination - 16U) & 0x07U) << 4U) | ((source - 16U) & 0x07U));
}

constexpr u16 encode_fmuls(const u8 destination, const u8 source) {
    return static_cast<u16>(0x0380U | ((static_cast<u16>(destination - 16U) & 0x07U) << 4U) | ((source - 16U) & 0x07U));
}

constexpr u16 encode_fmulsu(const u8 destination, const u8 source) {
    return static_cast<u16>(0x0388U | ((static_cast<u16>(destination - 16U) & 0x07U) << 4U) | ((source - 16U) & 0x07U));
}

bool flag_is_set(const u8 sreg, const SregFlag bit) {
    return (sreg & (1U << static_cast<u8>(bit))) != 0U;
}

} // namespace

TEST_CASE("CPU Fractional Multiplication (FMUL) Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x40U),                     // 0
            encode_ldi(17U, 0x40U),                     // 1
            encode_fmul(16U, 17U),                      // 2: (0x40 * 0x40) << 1 = 0x1000 << 1 = 0x2000
            
            encode_ldi(18U, 0x80U),                     // 3
            encode_ldi(19U, 0x40U),                     // 4
            encode_fmuls(18U, 19U),                     // 5: (-128 * 64) << 1 = -8192 << 1 = -16384 = 0xC000
            
            encode_ldi(20U, 0x80U),                     // 6
            encode_ldi(21U, 0x40U),                     // 7
            encode_fmulsu(20U, 21U),                    // 8: (-128 * 64) << 1 = -16384 = 0xC000
            
            encode_ldi(22U, 0x00U),                     // 9
            encode_ldi(23U, 0x55U),                     // 10
            encode_fmul(22U, 23U),                      // 11: 0 * 0x55 = 0
            
            0x0000U                                     // 12
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("FMUL Unsigned") {
        cpu.run(3);
        auto s = cpu.snapshot();
        CHECK(s.gpr[0] == 0x00U);
        CHECK(s.gpr[1] == 0x20U); // 0x2000
        CHECK_FALSE(flag_is_set(s.sreg, SregFlag::zero));
        CHECK_FALSE(flag_is_set(s.sreg, SregFlag::carry));
    }

    SUBCASE("FMULS Signed") {
        cpu.run(6);
        auto s = cpu.snapshot();
        CHECK(s.gpr[0] == 0x00U);
        CHECK(s.gpr[1] == 0xC0U); // 0xC000
        CHECK(flag_is_set(s.sreg, SregFlag::carry)); // Result is negative -> Carry=1 for FMULs? 
        // AVR manual says Carry = MSB of result (bit 15 of product before shift, or bit 16?)
        // Actually for FMULS, Carry is set if the product is negative.
    }

    SUBCASE("FMULSU Signed/Unsigned") {
        cpu.run(9);
        auto s = cpu.snapshot();
        CHECK(s.gpr[0] == 0x00U);
        CHECK(s.gpr[1] == 0xC0U); // 0xC000
        CHECK(flag_is_set(s.sreg, SregFlag::carry));
    }

    SUBCASE("FMUL Zero") {
        cpu.run(12);
        auto s = cpu.snapshot();
        CHECK(s.gpr[0] == 0x00U);
        CHECK(s.gpr[1] == 0x00U);
        CHECK(flag_is_set(s.sreg, SregFlag::zero));
    }
}
