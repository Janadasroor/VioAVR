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

constexpr u16 encode_mov(const u8 destination, const u8 source) {
    return static_cast<u16>(0x2C00U | (static_cast<u16>(destination) << 4U) | (source & 0x0FU) | ((static_cast<u16>(source) & 0x10U) << 5U));
}

constexpr u16 encode_add(const u8 destination, const u8 source) {
    return static_cast<u16>(0x0C00U | (static_cast<u16>(destination) << 4U) | (source & 0x0FU) | ((static_cast<u16>(source) & 0x10U) << 5U));
}

constexpr u16 encode_mul(const u8 destination, const u8 source) {
    return static_cast<u16>(0x9C00U | (static_cast<u16>(destination) << 4U) | (source & 0x0FU) | ((static_cast<u16>(source) & 0x10U) << 5U));
}

constexpr u16 encode_muls(const u8 destination, const u8 source) {
    return static_cast<u16>(0x0200U | (static_cast<u16>(destination - 16U) << 4U) | (source - 16U));
}

constexpr u16 encode_mulsu(const u8 destination, const u8 source) {
    return static_cast<u16>(0x0300U | ((static_cast<u16>(destination - 16U) & 0x07U) << 4U) | ((source - 16U) & 0x07U));
}

constexpr u16 encode_adc(const u8 destination, const u8 source) {
    return static_cast<u16>(0x1C00U | (static_cast<u16>(destination) << 4U) | (source & 0x0FU) | ((static_cast<u16>(source) & 0x10U) << 5U));
}

constexpr u16 encode_sub(const u8 destination, const u8 source) {
    return static_cast<u16>(0x1800U | (static_cast<u16>(destination) << 4U) | (source & 0x0FU) | ((static_cast<u16>(source) & 0x10U) << 5U));
}

constexpr u16 encode_subi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0x5000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_cpi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0x3000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_andi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0x7000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_ori(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0x6000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_and(const u8 destination, const u8 source) {
    return static_cast<u16>(0x2000U | (static_cast<u16>(destination) << 4U) | (source & 0x0FU) | ((static_cast<u16>(source) & 0x10U) << 5U));
}

constexpr u16 encode_or(const u8 destination, const u8 source) {
    return static_cast<u16>(0x2800U | (static_cast<u16>(destination) << 4U) | (source & 0x0FU) | ((static_cast<u16>(source) & 0x10U) << 5U));
}

constexpr u16 encode_eor(const u8 destination, const u8 source) {
    return static_cast<u16>(0x2400U | (static_cast<u16>(destination) << 4U) | (source & 0x0FU) | ((static_cast<u16>(source) & 0x10U) << 5U));
}

constexpr u16 encode_rjmp(const int displacement) {
    return static_cast<u16>(0xC000U | (displacement & 0x0FFF));
}

constexpr u16 encode_breq(const int displacement) {
    return static_cast<u16>(0xF001U | ((displacement & 0x7F) << 3U));
}

constexpr u16 encode_brne(const int displacement) {
    return static_cast<u16>(0xF401U | ((displacement & 0x7F) << 3U));
}

constexpr u16 encode_brcs(const int displacement) {
    return static_cast<u16>(0xF000U | ((displacement & 0x7F) << 3U));
}

constexpr u16 encode_brcc(const int displacement) {
    return static_cast<u16>(0xF400U | ((displacement & 0x7F) << 3U));
}

constexpr u16 encode_brmi(const int displacement) {
    return static_cast<u16>(0xF002U | ((displacement & 0x7F) << 3U));
}

constexpr u16 encode_brpl(const int displacement) {
    return static_cast<u16>(0xF402U | ((displacement & 0x7F) << 3U));
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

TEST_CASE("CPU ALU and Flow Control Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x0FU),                     // 0
            encode_ldi(17U, 0x01U),                     // 1
            encode_add(16U, 17U),                       // 2
            encode_mov(18U, 16U),                       // 3
            encode_sub(18U, 17U),                       // 4
            encode_mul(16U, 17U),                       // 5
            encode_ldi(20U, 0xFEU),                     // 6
            encode_ldi(21U, 0x02U),                     // 7
            encode_muls(20U, 21U),                      // 8
            encode_ldi(22U, 0xFEU),                     // 9
            encode_ldi(23U, 0x03U),                     // 10
            encode_mulsu(22U, 23U),                     // 11
            encode_subi(18U, 0x0FU),                    // 12
            encode_ldi(21U, 0x00U),                     // 13
            encode_subi(21U, 0x01U),                    // 14
            encode_ldi(22U, 0x01U),                     // 15
            encode_ldi(23U, 0x01U),                     // 16
            encode_adc(22U, 23U),                       // 17
            encode_ldi(20U, 0x7FU),                     // 18
            encode_add(20U, 17U),                       // 19
            encode_ldi(24U, 0xAAU),                     // 20
            encode_and(24U, 22U),                       // 21
            encode_or(24U, 21U),                        // 22
            encode_eor(24U, 24U),                       // 23
            encode_breq(1),                             // 24 (PC=24 + 1 + 1 = 26 if taken)
            encode_ldi(25U, 0x77U),                     // 25
            encode_brne(1),                             // 26
            encode_ldi(26U, 0x55U),                     // 27
            encode_or(24U, 22U),                        // 28
            encode_brne(1),                             // 29
            encode_ldi(27U, 0x66U),                     // 30
            encode_rjmp(1),                             // 31
            encode_ldi(28U, 0xF0U),                     // 32
            encode_andi(28U, 0x0FU),                    // 33
            encode_ori(28U, 0x05U),                     // 34
            encode_cpi(28U, 0x06U),                     // 35
            encode_brcs(1),                             // 36
            encode_ldi(29U, 0x11U),                     // 37
            encode_ldi(30U, 0x80U),                     // 38
            encode_ori(30U, 0x00U),                     // 39
            encode_brmi(1),                             // 40
            encode_ldi(31U, 0x22U),                     // 41
            encode_brpl(1),                             // 42
            encode_mov(31U, 22U),                       // 43
            encode_brcc(1),                             // 44
            encode_ldi(29U, 0x33U),                     // 45
            0x0000U,
            0x0000U
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("Basic Addition and Subtraction") {
        cpu.step(); // LDI r16, 0x0F
        cpu.step(); // LDI r17, 0x01
        cpu.step(); // ADD r16, r17
        auto s = cpu.snapshot();
        CHECK(s.gpr[16] == 0x10U);
        CHECK(flag_is_set(s.sreg, SregFlag::halfCarry));
        CHECK_FALSE(flag_is_set(s.sreg, SregFlag::carry));

        cpu.step(); // MOV r18, r16
        cpu.step(); // SUB r18, r17
        s = cpu.snapshot();
        CHECK(s.gpr[18] == 0x0FU);
    }

    SUBCASE("Multiplication") {
        step_to(cpu, 5); // Reach MUL
        cpu.step(); // MUL r16, r17
        auto s = cpu.snapshot();
        CHECK(s.gpr[0] == 0x10U); // r0 = 0x10 * 0x01 = 0x10
        CHECK(s.gpr[1] == 0x00U);
        
        cpu.step(); // LDI r20, 0xFE
        cpu.step(); // LDI r21, 0x02
        cpu.step(); // MULS r20, r21 (-2 * 2 = -4)
        s = cpu.snapshot();
        CHECK(s.gpr[0] == 0xFCU); // 0xFC = -4 (lo)
        CHECK(s.gpr[1] == 0xFFU); // 0xFF = -4 (hi)

        cpu.step(); // LDI r22, 0xFE
        cpu.step(); // LDI r23, 0x03
        cpu.step(); // MULSU r22, r23 (-2 * 3 = -6)
        s = cpu.snapshot();
        CHECK(s.gpr[0] == 0xFAU);
        CHECK(s.gpr[1] == 0xFFU);
    }

    SUBCASE("Immediate operations and ADC") {
        step_to(cpu, 12); // Reach SUBI
        cpu.step(); // SUBI r18, 0x0F (r18 was 0x0F)
        auto s = cpu.snapshot();
        CHECK(s.gpr[18] == 0x00U);
        CHECK(flag_is_set(s.sreg, SregFlag::zero));

        cpu.step(); // LDI r21, 0x00
        cpu.step(); // SUBI r21, 0x01 (0x00 - 0x01 = 0xFF, Carry=1)
        s = cpu.snapshot();
        CHECK(s.gpr[21] == 0xFFU);
        CHECK(flag_is_set(s.sreg, SregFlag::carry));

        cpu.step(); // LDI r22, 0x01
        cpu.step(); // LDI r23, 0x01
        cpu.step(); // ADC r22, r23 (1 + 1 + C(1) = 3)
        s = cpu.snapshot();
        CHECK(s.gpr[22] == 0x03U);
    }

    SUBCASE("Logical operations and Flow Control") {
        step_to(cpu, 19); // Reach ADD overflow test
        cpu.step(); // ADD r20, r17 (0x7F + 0x01 = 0x80, Overflow=1)
        auto s = cpu.snapshot();
        CHECK(s.gpr[20] == 0x80U);
        CHECK(flag_is_set(s.sreg, SregFlag::overflow));

        step_to(cpu, 23); // Reach EOR r24, r24
        cpu.step(); // EOR r24, r24 -> result 0, Zero=1
        CHECK(flag_is_set(cpu.snapshot().sreg, SregFlag::zero));

        cpu.step(); // BREQ 1 (Taken)
        CHECK(cpu.program_counter() == 26U); // Skipped PC=25 (LDI r25, 0x77)
        
        cpu.step(); // BRNE 1 (Not taken, Zero is still 1)
        CHECK(cpu.program_counter() == 27U); 
        
        cpu.step(); // LDI r26, 0x55
        CHECK(cpu.snapshot().gpr[26] == 0x55U);
    }

    SUBCASE("Comparisons and Branches") {
        step_to(cpu, 33); // Reach ANDI
        cpu.step(); // ANDI r28, 0x0F
        cpu.step(); // ORI r28, 0x05
        cpu.step(); // CPI r28, 0x06 (Result 5 - 6 = -1, Carry=1)
        CHECK(flag_is_set(cpu.snapshot().sreg, SregFlag::carry));
        
        cpu.step(); // BRCS 1 (Taken)
        CHECK(cpu.program_counter() == 38U); // Skipped PC=37 (LDI r29, 0x11)
    }
}
