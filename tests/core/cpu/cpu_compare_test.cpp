#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_rr(const u16 base, const u8 destination, const u8 source) {
    return static_cast<u16>(base | (static_cast<u16>(destination) << 4U) | (source & 0x0FU) | ((static_cast<u16>(source) & 0x10U) << 5U));
}

constexpr u16 encode_cp(const u8 destination, const u8 source) { return encode_rr(0x1400U, destination, source); }
constexpr u16 encode_cpc(const u8 destination, const u8 source) { return encode_rr(0x0400U, destination, source); }
constexpr u16 encode_cpse(const u8 destination, const u8 source) { return encode_rr(0x1000U, destination, source); }
constexpr u16 encode_sbc(const u8 destination, const u8 source) { return encode_rr(0x0800U, destination, source); }

constexpr u16 encode_sbci(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0x4000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_andi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0x7000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_ori(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0x6000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

bool flag_is_set(const u8 sreg, const SregFlag bit) {
    return (sreg & (1U << static_cast<u8>(bit))) != 0U;
}

void step_to(AvrCpu& cpu, u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) {
        cpu.step();
    }
}

} // namespace

TEST_CASE("CPU Comparison and Conditional Skip Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x12U),         // 0
            encode_ldi(17U, 0x12U),         // 1
            encode_cp(16U, 17U),            // 2: Compare 0x12, 0x12 -> Zero=1
            encode_cpse(16U, 17U),          // 3: Skip if equal (Taken)
            encode_ldi(18U, 0xAAU),         // 4: Skipped
            encode_cpse(16U, 17U),          // 5: Skip if equal (Taken)
            0x940CU, 0x000AU,               // 6, 7: JMP 10 (32-bit skip! Taken skip skips both words)
            encode_ldi(19U, 0x55U),         // 8: PC=8
            encode_ldi(20U, 0x00U),         // 9
            encode_ldi(21U, 0x01U),         // 10
            encode_cp(20U, 21U),            // 11: 0 - 1 = -1 (Carry=1, Negative=1, Sign=0, Zero=0)
            encode_ldi(22U, 0x00U),         // 12
            encode_ldi(23U, 0x00U),         // 13
            encode_cpc(22U, 23U),           // 14: 0 - 0 - C(1) = 0xFF (Carry=1, Negative=1, Zero remains 0)
            encode_ldi(24U, 0x00U),         // 15
            encode_ldi(25U, 0x01U),         // 16
            encode_sbc(24U, 25U),           // 17: 0 - 1 - C(1) = 0xFE (Wait, prior carry was set by CPC at 14)
                                            // Actually CPC at 14 sets Carry. So 0 - 1 - 1 = 0xFE. Correct.
            encode_ldi(26U, 0x00U),         // 18
            encode_sbci(26U, 0x00U),        // 19: 0 - 0 - C(1) = 0xFF
            encode_ldi(27U, 0xFFU),         // 20
            encode_ori(27U, 0x00U),         // 21: SBR alias (ORI r27, 0)
            encode_andi(27U, 0x0FU),        // 22: CBR alias (ANDI r27, 0x0F)
            0x0000U                         // 23
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("Comparison and Skip instructions") {
        step_to(cpu, 3U); // Up to instruction at PC=2
        CHECK(flag_is_set(cpu.snapshot().sreg, SregFlag::zero));
        CHECK_FALSE(flag_is_set(cpu.snapshot().sreg, SregFlag::carry));

        cpu.step(); // CPSE (Taken, skips PC=4)
        CHECK(cpu.program_counter() == 5U);
        CHECK(cpu.snapshot().gpr[18] == 0U);

        cpu.step(); // CPSE (Taken, skips 32-bit JMP at 6,7)
        CHECK(cpu.program_counter() == 8U);
        
        cpu.step(); // LDI r19, 0x55
        CHECK(cpu.snapshot().gpr[19] == 0x55U);
    }

    SUBCASE("Multi-byte Arithmetic (CPC, SBC)") {
        step_to(cpu, 12U); // Execute through CP at PC=11
        auto s = cpu.snapshot();
        CHECK(flag_is_set(s.sreg, SregFlag::carry));
        CHECK(flag_is_set(s.sreg, SregFlag::negative));

        step_to(cpu, 15U); // Execute through LDI x2, CPC
        s = cpu.snapshot();
        CHECK(s.gpr[22] == 0x00U); // CPC doesn't write back
        CHECK(flag_is_set(s.sreg, SregFlag::carry)); // 0 - 0 - 1 = -1
        CHECK(flag_is_set(s.sreg, SregFlag::negative));

        step_to(cpu, 18U); // Execute through LDI x2, SBC
        s = cpu.snapshot();
        CHECK(s.gpr[24] == 0xFEU); // 0 - 1 - 1 = 0xFE
        CHECK(flag_is_set(s.sreg, SregFlag::carry));
        
        step_to(cpu, 20U); // Execute through LDI, SBCI
        s = cpu.snapshot();
        CHECK(s.gpr[26] == 0xFFU); // 0 - 0 - 1 = 0xFF
        CHECK(flag_is_set(s.sreg, SregFlag::carry));
    }

    SUBCASE("Logical Aliases (SBR, CBR)") {
        step_to(cpu, 20U); // Reach LDI r27, 0xFF
        cpu.step(); // LDI
        cpu.step(); // ORI (SBR)
        cpu.step(); // ANDI (CBR 0x0F)
        auto s = cpu.snapshot();
        CHECK(s.gpr[27] == 0x0FU);
    }
}
