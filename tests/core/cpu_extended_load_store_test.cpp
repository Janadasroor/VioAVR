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

constexpr u16 encode_ld_x_predec(const u8 destination) { return static_cast<u16>(0x900EU | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_st_x_predec(const u8 source) { return static_cast<u16>(0x920EU | (static_cast<u16>(source) << 4U)); }
constexpr u16 encode_ld_y_predec(const u8 destination) { return static_cast<u16>(0x900AU | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_st_y_predec(const u8 source) { return static_cast<u16>(0x920AU | (static_cast<u16>(source) << 4U)); }
constexpr u16 encode_ld_z_predec(const u8 destination) { return static_cast<u16>(0x9002U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_st_z_predec(const u8 source) { return static_cast<u16>(0x9202U | (static_cast<u16>(source) << 4U)); }

constexpr u16 encode_ldd_y(const u8 destination, const u8 disp) {
    return static_cast<u16>(0x8008U | ((static_cast<u16>(destination) & 0x1FU) << 4U) | ((static_cast<u16>(disp) & 0x20U) << 8U) | ((static_cast<u16>(disp) & 0x18U) << 7U) | (disp & 0x07U));
}
constexpr u16 encode_std_y(const u8 source, const u8 disp) {
    return static_cast<u16>(0x8208U | ((static_cast<u16>(source) & 0x1FU) << 4U) | ((static_cast<u16>(disp) & 0x20U) << 8U) | ((static_cast<u16>(disp) & 0x18U) << 7U) | (disp & 0x07U));
}
constexpr u16 encode_ldd_z(const u8 destination, const u8 disp) {
    return static_cast<u16>(0x8000U | ((static_cast<u16>(destination) & 0x1FU) << 4U) | ((static_cast<u16>(disp) & 0x20U) << 8U) | ((static_cast<u16>(disp) & 0x18U) << 7U) | (disp & 0x07U));
}
constexpr u16 encode_std_z(const u8 source, const u8 disp) {
    return static_cast<u16>(0x8200U | ((static_cast<u16>(source) & 0x1FU) << 4U) | ((static_cast<u16>(disp) & 0x20U) << 8U) | ((static_cast<u16>(disp) & 0x18U) << 7U) | (disp & 0x07U));
}

constexpr u16 encode_lds(const u8 destination) { return static_cast<u16>(0x9000U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_sts(const u8 source) { return static_cast<u16>(0x9200U | (static_cast<u16>(source) << 4U)); }

u16 pair_value(const CpuSnapshot& snapshot, const u8 low_register) {
    return static_cast<u16>(snapshot.gpr[low_register] | (static_cast<u16>(snapshot.gpr[low_register + 1U]) << 8U));
}

void step_to(AvrCpu& cpu, u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) {
        cpu.step();
    }
}

} // namespace

TEST_CASE("CPU Extended Load and Store Instructions Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    const auto sram_base = atmega328p.sram_range().begin;

    bus.load_image(HexImage {
        .flash_words = {
            // X Pre-decrement
            encode_ldi(26U, static_cast<u8>((sram_base + 2U) & 0xFFU)),
            encode_ldi(27U, static_cast<u8>((sram_base + 2U) >> 8U)),
            encode_ldi(16U, 0x5AU),
            encode_st_x_predec(16U),                    // X -> sram_base+1
            encode_ld_x_predec(2U),                     // X -> sram_base, r2 = [sram_base]

            // Y Pre-decrement and Displacement
            encode_ldi(28U, static_cast<u8>((sram_base + 0x12U) & 0xFFU)),
            encode_ldi(29U, static_cast<u8>((sram_base + 0x12U) >> 8U)),
            encode_ldi(18U, 0x6BU),
            encode_st_y_predec(18U),                    // Y -> sram_base+0x11
            encode_ld_y_predec(19U),                    // Y -> sram_base+0x10
            encode_ldi(20U, 0x7CU),
            encode_std_y(20U, 3U),                      // [Y+3] = 0x7C -> sram_base+0x13
            encode_ldd_y(21U, 3U),                      // r21 = [Y+3]

            // Z Pre-decrement and Displacement
            encode_ldi(30U, static_cast<u8>((sram_base + 0x22U) & 0xFFU)),
            encode_ldi(31U, static_cast<u8>((sram_base + 0x22U) >> 8U)),
            encode_ldi(22U, 0x8DU),
            encode_st_z_predec(22U),                    // Z -> sram_base+0x21
            encode_ld_z_predec(23U),                    // Z -> sram_base+0x20
            encode_ldi(24U, 0x9EU),
            encode_std_z(24U, 5U),                      // [Z+5] = 0x9E -> sram_base+0x25
            encode_ldd_z(25U, 5U),                      // r25 = [Z+5]

            // Direct LDS/STS
            encode_ldi(16U, 0xAFU),
            encode_sts(16U), static_cast<u16>(sram_base + 0x30U),
            encode_lds(17U), static_cast<u16>(sram_base + 0x30U),

            0x0000U
        },
        .entry_word = 0U
    });
    cpu.reset();

    // Pre-fill memory for pre-decrement LD tests
    bus.write_data(static_cast<u16>(sram_base), 0xE1U);
    bus.write_data(static_cast<u16>(sram_base + 0x10U), 0xE2U);
    bus.write_data(static_cast<u16>(sram_base + 0x20U), 0xE3U);

    SUBCASE("Execution and Indirect Register Behavior") {
        step_to(cpu, 100U); // Run to halt
        auto s = cpu.snapshot();
        
        // X
        CHECK(bus.read_data(sram_base + 1U) == 0x5AU);
        CHECK(s.gpr[2] == 0xE1U);
        CHECK(pair_value(s, 26U) == sram_base);

        // Y
        CHECK(bus.read_data(sram_base + 0x11U) == 0x6BU);
        CHECK(bus.read_data(sram_base + 0x13U) == 0x7CU);
        CHECK(s.gpr[19] == 0xE2U);
        CHECK(s.gpr[21] == 0x7CU);
        CHECK(pair_value(s, 28U) == static_cast<u16>(sram_base + 0x10U));

        // Z
        CHECK(bus.read_data(sram_base + 0x21U) == 0x8DU);
        CHECK(bus.read_data(sram_base + 0x25U) == 0x9EU);
        CHECK(s.gpr[23] == 0xE3U);
        CHECK(s.gpr[25] == 0x9EU);
        CHECK(pair_value(s, 30U) == static_cast<u16>(sram_base + 0x20U));

        // Direct
        CHECK(bus.read_data(sram_base + 0x30U) == 0xAFU);
        CHECK(s.gpr[17] == 0xAFU);
    }
}
