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

constexpr u16 encode_adiw(const u8 low_register, const u8 immediate) {
    const auto pair_selector = static_cast<u16>((low_register - 24U) / 2U);
    return static_cast<u16>(0x9600U | ((static_cast<u16>(immediate & 0x30U)) << 2U) | (pair_selector << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_sbiw(const u8 low_register, const u8 immediate) {
    const auto pair_selector = static_cast<u16>((low_register - 24U) / 2U);
    return static_cast<u16>(0x9700U | ((static_cast<u16>(immediate & 0x30U)) << 2U) | (pair_selector << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_movw(const u8 destination_low, const u8 source_low) {
    return static_cast<u16>(0x0100U | ((static_cast<u16>(destination_low / 2U) & 0x0FU) << 4U) | (static_cast<u16>(source_low / 2U) & 0x0FU));
}

u16 pair_value(const CpuSnapshot& snapshot, const u8 low_register) {
    return static_cast<u16>(snapshot.gpr[low_register] | (static_cast<u16>(snapshot.gpr[low_register + 1U]) << 8U));
}

bool flag_is_set(const u8 sreg, const SregFlag bit) {
    return (sreg & (1U << static_cast<u8>(bit))) != 0U;
}

} // namespace

TEST_CASE("CPU Word Operations Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(24U, 0xFFU),                     // 0
            encode_ldi(25U, 0x7FU),                     // 1
            encode_adiw(24U, 1U),                       // 2: 0x7FFF + 1 = 0x8000

            encode_ldi(26U, 0xFFU),                     // 3
            encode_ldi(27U, 0xFFU),                     // 4
            encode_adiw(26U, 1U),                       // 5: 0xFFFF + 1 = 0x0000

            encode_ldi(28U, 0x00U),                     // 6
            encode_ldi(29U, 0x00U),                     // 7
            encode_sbiw(28U, 1U),                       // 8: 0x0000 - 1 = 0xFFFF

            encode_ldi(30U, 0x01U),                     // 9
            encode_ldi(31U, 0x80U),                     // 10
            encode_sbiw(30U, 1U),                       // 11: 0x8001 - 1 = 0x8000

            encode_ldi(20U, 0x34U),                     // 12
            encode_ldi(21U, 0x12U),                     // 13
            encode_movw(16U, 20U),                      // 14: r17:r16 = r21:r20 (0x1234)

            0x0000U                                     // 15
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("ADIW Operations") {
        cpu.run(3); // Up to PC=2
        auto s = cpu.snapshot();
        CHECK(pair_value(s, 24) == 0x8000U);
        CHECK(flag_is_set(s.sreg, SregFlag::negative));
        CHECK(flag_is_set(s.sreg, SregFlag::overflow));

        cpu.run(3); // PC=4, 5
        s = cpu.snapshot();
        CHECK(pair_value(s, 26) == 0x0000U);
        CHECK(flag_is_set(s.sreg, SregFlag::zero));
        CHECK(flag_is_set(s.sreg, SregFlag::carry));
    }

    SUBCASE("SBIW Operations") {
        cpu.run(9); // Setup r28, r29 and SBIW
        auto s = cpu.snapshot();
        CHECK(pair_value(s, 28) == 0xFFFFU);
        CHECK(flag_is_set(s.sreg, SregFlag::carry));
        CHECK(flag_is_set(s.sreg, SregFlag::negative));

        cpu.run(3); // Setup r30, r31 and SBIW
        s = cpu.snapshot();
        CHECK(pair_value(s, 30) == 0x8000U);
        CHECK(flag_is_set(s.sreg, SregFlag::negative));
    }

    SUBCASE("MOVW Operations") {
        cpu.run(15); // Setup r20, r21 and MOVW
        auto s = cpu.snapshot();
        CHECK(pair_value(s, 16) == 0x1234U);
        CHECK(pair_value(s, 20) == 0x1234U);
    }
}
