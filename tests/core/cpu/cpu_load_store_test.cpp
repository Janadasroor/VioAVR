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

constexpr u16 encode_ld_x(const u8 destination) { return static_cast<u16>(0x900CU | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_ld_x_postinc(const u8 destination) { return static_cast<u16>(0x900DU | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_ld_y(const u8 destination) { return static_cast<u16>(0x8008U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_ld_y_postinc(const u8 destination) { return static_cast<u16>(0x9009U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_ld_z(const u8 destination) { return static_cast<u16>(0x8000U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_ld_z_postinc(const u8 destination) { return static_cast<u16>(0x9001U | (static_cast<u16>(destination) << 4U)); }

constexpr u16 encode_st_x(const u8 source) { return static_cast<u16>(0x920CU | (static_cast<u16>(source) << 4U)); }
constexpr u16 encode_st_x_postinc(const u8 source) { return static_cast<u16>(0x920DU | (static_cast<u16>(source) << 4U)); }
constexpr u16 encode_st_y(const u8 source) { return static_cast<u16>(0x8208U | (static_cast<u16>(source) << 4U)); }
constexpr u16 encode_st_y_postinc(const u8 source) { return static_cast<u16>(0x9209U | (static_cast<u16>(source) << 4U)); }
constexpr u16 encode_st_z(const u8 source) { return static_cast<u16>(0x8200U | (static_cast<u16>(source) << 4U)); }
constexpr u16 encode_st_z_postinc(const u8 source) { return static_cast<u16>(0x9201U | (static_cast<u16>(source) << 4U)); }

u16 pair_value(const CpuSnapshot& snapshot, const u8 low_register) {
    return static_cast<u16>(snapshot.gpr[low_register] | (static_cast<u16>(snapshot.gpr[low_register + 1U]) << 8U));
}

void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
} // namespace

TEST_CASE("CPU Load and Store Instructions Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    const auto sram_base = atmega328p.sram_range().begin;

    bus.load_image(HexImage {
        .flash_words = {
            // X register (r26:r27)
            encode_ldi(26U, static_cast<u8>(sram_base & 0x00FFU)),
            encode_ldi(27U, static_cast<u8>((sram_base >> 8U) & 0x00FFU)),
            encode_ldi(16U, 0x11U),
            encode_st_x(16U),
            encode_ld_x(17U),
            encode_ldi(16U, 0x22U),
            encode_st_x_postinc(16U),
            encode_ld_x(18U),
            encode_ld_x_postinc(19U),

            // Y register (r28:r29)
            encode_ldi(28U, static_cast<u8>((sram_base + 0x10U) & 0x00FFU)),
            encode_ldi(29U, static_cast<u8>(((sram_base + 0x10U) >> 8U) & 0x00FFU)),
            encode_ldi(20U, 0x33U),
            encode_st_y(20U),
            encode_ld_y(21U),
            encode_ldi(20U, 0x44U),
            encode_st_y_postinc(20U),
            encode_ld_y(22U),
            encode_ld_y_postinc(23U),

            // Z register (r30:r31)
            encode_ldi(30U, static_cast<u8>((sram_base + 0x20U) & 0x00FFU)),
            encode_ldi(31U, static_cast<u8>(((sram_base + 0x20U) >> 8U) & 0x00FFU)),
            encode_ldi(24U, 0x55U),
            encode_st_z(24U),
            encode_ld_z(25U),
            encode_ldi(24U, 0x66U),
            encode_st_z_postinc(24U),
            encode_ld_z(2U),
            encode_ld_z_postinc(3U),

            0x0000U
        },
        .entry_word = 0U
    });
    cpu.reset();
    
    // Pre-fill some memory locations for LD checks
    bus.write_data(static_cast<u16>(sram_base + 1U), 0xA1U);
    bus.write_data(static_cast<u16>(sram_base + 0x11U), 0xB2U);
    bus.write_data(static_cast<u16>(sram_base + 0x21U), 0xC3U);

    SUBCASE("X-register Address Space (SRAM base)") {
        step_to(cpu, 14U);
        auto s = cpu.snapshot();
        CHECK(bus.read_data(sram_base) == 0x22U);
        CHECK(s.gpr[17] == 0x11U);
        CHECK(s.gpr[18] == 0xA1U);
        CHECK(s.gpr[19] == 0xA1U);
        CHECK(pair_value(s, 26U) == static_cast<u16>(sram_base + 2U));
    }

    SUBCASE("Y-register Address Space (SRAM base + 0x10)") {
        step_to(cpu, 28U);
        auto s = cpu.snapshot();
        CHECK(bus.read_data(sram_base + 0x10U) == 0x44U);
        CHECK(s.gpr[21] == 0x33U);
        CHECK(s.gpr[22] == 0xB2U);
        CHECK(s.gpr[23] == 0xB2U);
        CHECK(pair_value(s, 28U) == static_cast<u16>(sram_base + 0x12U));
    }

    SUBCASE("Z-register Address Space (SRAM base + 0x20)") {
        step_to(cpu, 42U);
        auto s = cpu.snapshot();
        CHECK(bus.read_data(sram_base + 0x20U) == 0x66U);
        CHECK(s.gpr[25] == 0x55U);
        CHECK(s.gpr[2] == 0xC3U);
        CHECK(s.gpr[3] == 0xC3U);
        CHECK(pair_value(s, 30U) == static_cast<u16>(sram_base + 0x22U));
    }
}
