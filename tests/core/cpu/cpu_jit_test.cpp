#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

#ifndef _WIN32
#include "vioavr/core/avr_cpu.hpp"

namespace {

using namespace vioavr::core;
using namespace vioavr::core::devices;

constexpr u16 encode_ldi(u8 dst, u8 imm) {
    return static_cast<u16>(0xE000U | ((imm & 0xF0U) << 4U) | ((dst - 16U) << 4U) | (imm & 0x0FU));
}
constexpr u16 encode_mov(u8 dst, u8 src) {
    return static_cast<u16>(0x2C00U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
}
constexpr u16 encode_add(u8 dst, u8 src) {
    return static_cast<u16>(0x0C00U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
}
constexpr u16 encode_sub(u8 dst, u8 src) {
    return static_cast<u16>(0x1800U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
}
constexpr u16 encode_and(u8 dst, u8 src) {
    return static_cast<u16>(0x2000U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
}
constexpr u16 encode_or(u8 dst, u8 src) {
    return static_cast<u16>(0x2800U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
}
constexpr u16 encode_eor(u8 dst, u8 src) {
    return static_cast<u16>(0x2400U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
}
constexpr u16 encode_subi(u8 dst, u8 imm) {
    return static_cast<u16>(0x5000U | ((imm & 0xF0U) << 4U) | ((dst - 16U) << 4U) | (imm & 0x0FU));
}
constexpr u16 encode_cpi(u8 dst, u8 imm) {
    return static_cast<u16>(0x3000U | ((imm & 0xF0U) << 4U) | ((dst - 16U) << 4U) | (imm & 0x0FU));
}
constexpr u16 encode_andi(u8 dst, u8 imm) {
    return static_cast<u16>(0x7000U | ((imm & 0xF0U) << 4U) | ((dst - 16U) << 4U) | (imm & 0x0FU));
}
constexpr u16 encode_ori(u8 dst, u8 imm) {
    return static_cast<u16>(0x6000U | ((imm & 0xF0U) << 4U) | ((dst - 16U) << 4U) | (imm & 0x0FU));
}
constexpr u16 encode_lds(u8 dst) {
    return static_cast<u16>(0x9000U | (dst << 4U));
}
constexpr u16 encode_sts(u8 src) {
    return static_cast<u16>(0x9200U | (src << 4U));
}
constexpr u16 encode_push(u8 src) {
    return static_cast<u16>(0x920FU | (src << 4U));
}
constexpr u16 encode_pop(u8 dst) {
    return static_cast<u16>(0x900FU | (dst << 4U));
}
constexpr u16 encode_in(u8 dst, u8 io_offs) {
    return static_cast<u16>(0xB000U | (dst << 4U) | ((io_offs & 0x30U) << 5U) | (io_offs & 0x0FU));
}
constexpr u16 encode_out(u8 src, u8 io_offs) {
    return static_cast<u16>(0xB800U | (src << 4U) | ((io_offs & 0x30U) << 5U) | (io_offs & 0x0FU));
}
constexpr u16 encode_lsr(u8 dst) {
    return static_cast<u16>(0x9406U | (dst << 4U));
}
constexpr u16 encode_asr(u8 dst) {
    return static_cast<u16>(0x9405U | (dst << 4U));
}
constexpr u16 encode_ror(u8 dst) {
    return static_cast<u16>(0x9407U | (dst << 4U));
}
constexpr u16 encode_rjmp(int16_t disp) {
    return static_cast<u16>(0xC000U | (disp & 0x0FFFU));
}
constexpr u16 encode_rcall(int16_t disp) {
    return static_cast<u16>(0xD000U | (disp & 0x0FFFU));
}
constexpr u16 encode_brbs(int8_t disp, u8 bit) {
    return static_cast<u16>(0xF400U | (bit << 3U) | (disp & 0x3FU));
}
constexpr u16 encode_brbc(int8_t disp, u8 bit) {
    return static_cast<u16>(0xF000U | (bit << 3U) | (disp & 0x3FU));
}
constexpr u16 encode_sbrc(u8 reg, u8 bit) {
    return static_cast<u16>(0xFC00U | (reg << 4U) | bit);
}
constexpr u16 encode_sbrs(u8 reg, u8 bit) {
    return static_cast<u16>(0xFE00U | (reg << 4U) | bit);
}
constexpr u16 encode_movw(u8 dst, u8 src) {
    return static_cast<u16>(0x0100U | ((dst / 2) << 4U) | (src / 2));
}
constexpr u16 encode_adc(u8 dst, u8 src) {
    return static_cast<u16>(0x1C00U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
}
constexpr u16 encode_sbc(u8 dst, u8 src) {
    return static_cast<u16>(0x0800U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
}
constexpr u16 encode_cpc(u8 dst, u8 src) {
    return static_cast<u16>(0x0400U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
}
constexpr u16 encode_com(u8 dst) {
    return static_cast<u16>(0x9400U | (dst << 4U));
}
constexpr u16 encode_neg(u8 dst) {
    return static_cast<u16>(0x9401U | (dst << 4U));
}
constexpr u16 encode_inc(u8 dst) {
    return static_cast<u16>(0x9403U | (dst << 4U));
}
constexpr u16 encode_dec(u8 dst) {
    return static_cast<u16>(0x940AU | (dst << 4U));
}
constexpr u16 encode_swap(u8 dst) {
    return static_cast<u16>(0x9402U | (dst << 4U));
}
constexpr u16 encode_bset(u8 bit) {
    return static_cast<u16>(0x9408U | (bit << 4U));
}
constexpr u16 encode_bclr(u8 bit) {
    return static_cast<u16>(0x9488U | (bit << 4U));
}

// Run the same firmware with and without JIT, compare snapshots
static void run_with_and_without_jit(const HexImage& image, u32 steps) {
    // Run without JIT (interpreter only)
    MemoryBus bus_ref{atmega328p};
    AvrCpu cpu_ref{bus_ref};
    bus_ref.load_image(image);
    cpu_ref.run(steps);
    auto ref = cpu_ref.snapshot();

    // Run with JIT
    MemoryBus bus_jit{atmega328p};
    AvrCpu cpu_jit{bus_jit};
    cpu_jit.enable_jit(true);
    bus_jit.load_image(image);
    cpu_jit.run(steps);
    auto jit = cpu_jit.snapshot();

    CHECK_MESSAGE(ref.gpr == jit.gpr, "GPR mismatch at " << image.entry_word);
    CHECK(ref.sreg == jit.sreg);
    CHECK(ref.program_counter == jit.program_counter);
    CHECK(ref.stack_pointer == jit.stack_pointer);
    CHECK(ref.cycles == jit.cycles);
}

TEST_CASE("JIT Basic Instructions") {
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0xABU),
            encode_ldi(17U, 0xCDU),
            encode_mov(18U, 16U),
            encode_add(16U, 17U),
            encode_sub(17U, 18U),
            encode_and(19U, 16U),
            0x0000U, 0x0000U, 0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 20);
}

TEST_CASE("JIT Arithmetic and Flags") {
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0x01U),
            encode_ldi(17U, 0x80U),
            encode_add(16U, 17U),
            encode_sub(17U, 16U),
            encode_andi(16U, 0xF0U),
            encode_ori(17U, 0x0FU),
            encode_cpi(16U, 0x80U),
            0x0000U, 0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 20);
}

TEST_CASE("JIT Branch Instructions") {
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0x05U),
            encode_ldi(17U, 0x03U),
            encode_sub(16U, 17U),
            encode_brbc(2, 2),
            encode_ldi(18U, 0xFFU),
            encode_ldi(18U, 0xAAU),
            encode_rjmp(2),
            encode_ldi(19U, 0xFFU),
            encode_ldi(19U, 0xBBU),
            encode_rjmp(-2),
            0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 30);
}

TEST_CASE("JIT Register Operations") {
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0xAAU),
            encode_ldi(17U, 0x55U),
            encode_ldi(18U, 0xFFU),
            encode_movw(20U, 16U),
            encode_eor(16U, 17U),
            encode_or(17U, 18U),
            encode_and(18U, 20U),
            0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 30);
}

TEST_CASE("JIT Stack Operations") {
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0x12U),
            encode_ldi(17U, 0x34U),
            encode_push(16U),
            encode_push(17U),
            encode_pop(18U),
            encode_pop(19U),
            0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 20);
}

TEST_CASE("JIT Shift Operations") {
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0x8AU),
            encode_lsr(16U),
            encode_ldi(17U, 0x83U),
            encode_asr(17U),
            encode_ldi(18U, 0x01U),
            encode_ror(18U),
            0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 20);
}

TEST_CASE("JIT Subroutine Call and Return") {
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0x42U),
            encode_rcall(3),
            encode_ldi(17U, 0x99U),
            0x0000U,
            encode_ldi(18U, 0x77U),
            static_cast<u16>(0x9508U),
            0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 20);
}

TEST_CASE("JIT Skip Instructions") {
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0x00U),
            encode_ldi(17U, 0xFFU),
            encode_sbrc(16U, 0),
            encode_ldi(18U, 0x01U),
            encode_sbrs(17U, 7),
            encode_ldi(19U, 0x02U),
            encode_sbrc(17U, 0),
            encode_ldi(20U, 0x03U),
            encode_sbrs(16U, 0),
            encode_ldi(21U, 0x04U),
            0x0000U, 0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 30);
}

TEST_CASE("JIT LDS STS") {
    uint16_t data_addr = 0x0100;
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0x42U),
            encode_sts(16U),
            static_cast<u16>(data_addr),
            encode_lds(17U),
            static_cast<u16>(data_addr),
            0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 20);
}

TEST_CASE("JIT IN OUT") {
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0xAAU),
            encode_out(16U, 0x05U),
            encode_in(17U, 0x05U),
            0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 15);
}

TEST_CASE("JIT ADC SBC CPC COM NEG INC DEC SWAP") {
    auto image = HexImage{
        .flash_words = {
            encode_ldi(16U, 0x01U),
            encode_ldi(17U, 0x01U),
            encode_adc(16U, 17U),

            encode_bset(0),
            encode_adc(16U, 17U),
            encode_bclr(0),

            encode_sbc(16U, 17U),

            encode_cpc(17U, 17U),

            encode_ldi(18U, 0xABU),
            encode_com(18U),

            encode_ldi(19U, 0x01U),
            encode_neg(19U),

            encode_ldi(20U, 0x05U),
            encode_inc(20U),

            encode_ldi(21U, 0x05U),
            encode_dec(21U),

            encode_ldi(22U, 0xABU),
            encode_swap(22U),

            encode_ldi(23U, 0x80U),
            encode_neg(23U),
            0x0000U,
        },
        .entry_word = 0U
    };
    run_with_and_without_jit(image, 30);
}

} // namespace
#else
TEST_CASE("JIT skipped on Windows") {}
#endif
