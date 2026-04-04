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

constexpr u16 encode_inc(const u8 destination) { return static_cast<u16>(0x9403U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_com(const u8 destination) { return static_cast<u16>(0x9400U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_neg(const u8 destination) { return static_cast<u16>(0x9401U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_swap(const u8 destination) { return static_cast<u16>(0x9402U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_dec(const u8 destination) { return static_cast<u16>(0x940AU | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_asr(const u8 destination) { return static_cast<u16>(0x9405U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_lsr(const u8 destination) { return static_cast<u16>(0x9406U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_ror(const u8 destination) { return static_cast<u16>(0x9407U | (static_cast<u16>(destination) << 4U)); }

constexpr u16 encode_lsl(const u8 destination) {
    return static_cast<u16>(0x0C00U | (static_cast<u16>(destination) << 4U) | (destination & 0x0FU) | ((static_cast<u16>(destination) & 0x10U) << 5U));
}
constexpr u16 encode_rol(const u8 destination) {
    return static_cast<u16>(0x1C00U | (static_cast<u16>(destination) << 4U) | (destination & 0x0FU) | ((static_cast<u16>(destination) & 0x10U) << 5U));
}
constexpr u16 encode_tst(const u8 destination) {
    return static_cast<u16>(0x2000U | (static_cast<u16>(destination) << 4U) | (destination & 0x0FU) | ((static_cast<u16>(destination) & 0x10U) << 5U));
}
constexpr u16 encode_clr(const u8 destination) {
    return static_cast<u16>(0x2400U | (static_cast<u16>(destination) << 4U) | (destination & 0x0FU) | ((static_cast<u16>(destination) & 0x10U) << 5U));
}

constexpr u16 encode_ser(const u8 destination) { return static_cast<u16>(0xEF0FU | (static_cast<u16>(destination - 16U) << 4U)); }
constexpr u16 encode_bld(const u8 destination, const u8 bit) { return static_cast<u16>(0xF800U | (static_cast<u16>(destination) << 4U) | (bit & 0x07U)); }
constexpr u16 encode_bst(const u8 destination, const u8 bit) { return static_cast<u16>(0xFA00U | (static_cast<u16>(destination) << 4U) | (bit & 0x07U)); }

constexpr u16 encode_rcall(const int displacement) { return static_cast<u16>(0xD000U | (displacement & 0x0FFF)); }
constexpr u16 encode_ret() { return 0x9508U; }

constexpr u16 encode_rjmp(const int displacement) {
    return static_cast<u16>(0xC000U | (displacement & 0x0FFF));
}

constexpr u16 encode_bset(const SregFlag flag_bit) {
    return static_cast<u16>(0x9408U | (static_cast<u16>(flag_bit) << 4U));
}

constexpr u16 encode_bclr(const SregFlag flag_bit) {
    return static_cast<u16>(0x9488U | (static_cast<u16>(flag_bit) << 4U));
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

TEST_CASE("CPU Small Operations and Shifts Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};
    const auto initial_sp = atmega328.sram_range().end;

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0xFFU),                     // 0
            encode_inc(16U),                            // 1
            encode_ldi(17U, 0x7FU),                     // 2
            encode_inc(17U),                            // 3
            encode_ldi(18U, 0x80U),                     // 4
            encode_dec(18U),                            // 5
            encode_ldi(19U, 0x00U),                     // 6
            encode_tst(19U),                            // 7
            encode_ser(20U),                            // 8
            encode_clr(20U),                            // 9
            encode_ldi(22U, 0x55U),                     // 10
            encode_com(22U),                            // 11 -> 0xAA
            encode_ldi(23U, 0x01U),                     // 12
            encode_neg(23U),                            // 13 -> 0xFF
            encode_swap(22U),                           // 14 -> 0xAA (Nibbles swapped)
            encode_ldi(24U, 0x03U),                     // 15
            encode_lsr(24U),                            // 16 -> 0x01, Carry=1
            encode_ror(24U),                            // 17 -> 0x80, Carry=1 (from previous rotate)
            encode_ldi(25U, 0x81U),                     // 18
            encode_asr(25U),                            // 19 -> 0xC0 (-64), Carry=1
            encode_bst(22U, 1U),                        // 20
            encode_ldi(26U, 0x00U),                     // 21
            encode_bld(26U, 4U),                        // 22
            encode_bst(23U, 0U),                        // 23
            encode_bld(26U, 0U),                        // 24
            encode_rcall(2),                            // 25 -> PC=28
            encode_ldi(20U, 0xAAU),                     // 26 (Skipped after RET)
            encode_rjmp(2),                             // 27 (End)
            encode_ldi(21U, 0x55U),                     // 28: Subroutine
            encode_ret(),                               // 29
            0x0000U,                                    // 30
            encode_bset(SregFlag::carry),               // 31
            encode_bset(SregFlag::zero),                // 32
            encode_bclr(SregFlag::carry),               // 33
            encode_bclr(SregFlag::zero),                // 34
            encode_bset(SregFlag::negative),            // 35
            encode_bset(SregFlag::overflow),            // 36
            encode_bset(SregFlag::sign),                // 37
            encode_bset(SregFlag::halfCarry),           // 38
            encode_bset(SregFlag::transfer),            // 39
            encode_bclr(SregFlag::negative),            // 40
            encode_bclr(SregFlag::overflow),            // 41
            encode_bclr(SregFlag::sign),                // 42
            encode_bclr(SregFlag::halfCarry),           // 43
            encode_bclr(SregFlag::transfer),            // 44
            encode_ldi(18U, 0x81U),                     // 45
            encode_lsl(18U),                            // 46
            encode_ldi(19U, 0x80U),                     // 47
            encode_rol(19U),                            // 48
            0x0000U                                     // 49
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("Inc, Dec, and Unary operations") {
        cpu.run(2); // LDI, INC
        auto s = cpu.snapshot();
        CHECK(s.gpr[16] == 0x00U);
        CHECK(flag_is_set(s.sreg, SregFlag::zero));

        cpu.run(2); // LDI, INC
        s = cpu.snapshot();
        CHECK(s.gpr[17] == 0x80U);
        CHECK(flag_is_set(s.sreg, SregFlag::overflow));

        cpu.run(2); // LDI, DEC
        s = cpu.snapshot();
        CHECK(s.gpr[18] == 0x7FU);
        CHECK(flag_is_set(s.sreg, SregFlag::overflow));
    }

    SUBCASE("Logical and bit manipulation") {
        step_to(cpu, 7U); // Reach TST
        cpu.step(); // TST r19 (0)
        CHECK(flag_is_set(cpu.snapshot().sreg, SregFlag::zero));

        step_to(cpu, 9U); cpu.step(); // Execute through SER, CLR
        CHECK(cpu.snapshot().gpr[20] == 0x00U);
        CHECK(flag_is_set(cpu.snapshot().sreg, SregFlag::zero));

        step_to(cpu, 11U); cpu.step(); // Execute through LDI, COM
        CHECK(cpu.snapshot().gpr[22] == 0xAAU);

        step_to(cpu, 13U); cpu.step(); // Execute through LDI, NEG
        CHECK(cpu.snapshot().gpr[23] == 0xFFU);
    }

    SUBCASE("Shifts and Rotates") {
        step_to(cpu, 16U); cpu.step(); // LSR r24 (3 -> 1, Carry=1)
        auto s = cpu.snapshot();
        CHECK(s.gpr[24] == 0x01U);
        CHECK(flag_is_set(s.sreg, SregFlag::carry));

        cpu.step(); // ROR r24 (1 -> 0x80, Carry=1)
        s = cpu.snapshot();
        CHECK(s.gpr[24] == 0x80U);
        CHECK(flag_is_set(s.sreg, SregFlag::carry));

        step_to(cpu, 19U); cpu.step(); // Execute through LDI, ASR
        s = cpu.snapshot();
        CHECK(s.gpr[25] == 0xC0U);
        CHECK(flag_is_set(s.sreg, SregFlag::carry));
    }

    SUBCASE("BST, BLD and SREG manipulation") {
        step_to(cpu, 20U); // BST r22, 1 (r22=0xAA (binary ...1010), bit 1 is 1)
        cpu.step();
        CHECK(flag_is_set(cpu.snapshot().sreg, SregFlag::transfer));

        step_to(cpu, 22U); cpu.step(); // Execute through LDI, BLD
        CHECK(cpu.snapshot().gpr[26] == 0x10U);

        step_to(cpu, 45U); // Run through BSET/BCLR
        auto final_sreg = cpu.snapshot().sreg;
        CHECK(final_sreg == 0U);
    }
}
