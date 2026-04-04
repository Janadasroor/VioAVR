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

constexpr u16 encode_cpi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0x3000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_cpc(const u8 destination, const u8 source) {
    return static_cast<u16>(0x0400U | (static_cast<u16>(destination) << 4U) | (source & 0x0FU) | ((static_cast<u16>(source) & 0x10U) << 5U));
}

constexpr u16 encode_subi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0x5000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_sbci(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0x4000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_brcs(const int displacement) { return static_cast<u16>(0xF000U | ((displacement & 0x7F) << 3U)); }
constexpr u16 encode_brcc(const int displacement) { return static_cast<u16>(0xF400U | ((displacement & 0x7F) << 3U)); }

bool flag_is_set(const u8 sreg, const SregFlag bit) {
    return (sreg & (1U << static_cast<u8>(bit))) != 0U;
}

void step_to(AvrCpu& cpu, u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) {
        cpu.step();
    }
}

} // namespace

TEST_CASE("CPU Multi-byte Comparison and Subtraction Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x34U),         // 0
            encode_ldi(17U, 0x12U),         // 1
            encode_cpi(16U, 0x34U),         // 2
            encode_ldi(18U, 0x12U),         // 3
            encode_cpc(17U, 18U),           // 4: Compare 0x1234 with 0x1234
            encode_brcc(1),                 // 5: Taken if not carry (equal)
            encode_ldi(19U, 0xAAU),         // 6: Skipped
            encode_ldi(19U, 0x11U),         // 7
            
            encode_ldi(20U, 0x33U),         // 8
            encode_ldi(21U, 0x12U),         // 9
            encode_cpi(20U, 0x34U),         // 10
            encode_ldi(22U, 0x12U),         // 11
            encode_cpc(21U, 22U),           // 12: Compare 0x1233 with 0x1234 (Less than)
            encode_brcs(1),                 // 13: Taken if carry (less)
            encode_ldi(23U, 0xBBU),         // 14: Skipped
            encode_ldi(23U, 0x22U),         // 15
            
            encode_ldi(24U, 0x34U),         // 16
            encode_ldi(25U, 0x12U),         // 17
            encode_subi(24U, 0x34U),        // 18
            encode_sbci(25U, 0x12U),        // 19: Subtract 0x1234 from 0x1234
            encode_brcc(1),                 // 20: Taken if not carry (equal)
            encode_ldi(26U, 0xCCU),         // 21: Skipped
            encode_ldi(26U, 0x44U),         // 22
            
            encode_ldi(28U, 0x33U),         // 23
            encode_ldi(29U, 0x12U),         // 24
            encode_subi(28U, 0x34U),        // 25
            encode_sbci(29U, 0x12U),        // 26: Subtract 0x1234 from 0x1233 (Underflow)
            encode_brcs(1),                 // 27: Taken if carry (less)
            encode_ldi(30U, 0xDDU),         // 28: Skipped
            encode_ldi(30U, 0x55U),         // 29
            
            0x0000U                         // 30
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("Multi-byte Comparison (CPI, CPC)") {
        cpu.run(5); // Up to PC=4
        auto s = cpu.snapshot();
        CHECK(flag_is_set(s.sreg, SregFlag::zero));
        CHECK_FALSE(flag_is_set(s.sreg, SregFlag::carry));

        step_to(cpu, 3U); // Branch and next LDI
        CHECK(cpu.snapshot().gpr[19] == 0x11U);
        
        cpu.run(5); // Next Comparison (0x1233 vs 0x1234)
        s = cpu.snapshot();
        CHECK_FALSE(flag_is_set(s.sreg, SregFlag::zero));
        CHECK(flag_is_set(s.sreg, SregFlag::carry));
        
        step_to(cpu, 3U); // Branch and next LDI
        CHECK(cpu.snapshot().gpr[23] == 0x22U);
    }

    SUBCASE("Multi-byte Subtraction (SUBI, SBCI)") {
        step_to(cpu, 20U); // Reach PC=19
        auto s = cpu.snapshot();
        CHECK(flag_is_set(s.sreg, SregFlag::zero));
        CHECK_FALSE(flag_is_set(s.sreg, SregFlag::carry));
        
        step_to(cpu, 3U); // Branch and next LDI
        CHECK(cpu.snapshot().gpr[26] == 0x44U);
        
        cpu.run(4); // Next Subtraction (underflow)
        s = cpu.snapshot();
        CHECK_FALSE(flag_is_set(s.sreg, SregFlag::zero));
        CHECK(flag_is_set(s.sreg, SregFlag::carry));
        
        step_to(cpu, 3U); // Branch and next LDI
        CHECK(cpu.snapshot().gpr[30] == 0x55U);
    }
}
