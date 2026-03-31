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

constexpr u16 encode_lpm() { return 0x95C8U; }
constexpr u16 encode_lpm_z(const u8 destination) { return static_cast<u16>(0x9004U | (static_cast<u16>(destination) << 4U)); }
constexpr u16 encode_lpm_z_postinc(const u8 destination) { return static_cast<u16>(0x9005U | (static_cast<u16>(destination) << 4U)); }

} // namespace

TEST_CASE("CPU LPM (Load Program Memory) Instructions Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(30U, 0x10U),                     // 0: ZL = 16 (Byte address)
            encode_ldi(31U, 0x00U),                     // 1: ZH = 0
            encode_lpm(),                               // 2: r0 = flash[16] (Wait: LPM is 3 cycles)
            encode_lpm_z(16U),                          // 3: r16 = flash[16]
            encode_lpm_z_postinc(17U),                  // 4: r17 = flash[16], Z++
            encode_lpm_z_postinc(18U),                  // 5: r18 = flash[17], Z++
            0x0000U,                                    // 6: NOP
            0x0000U,                                    // 7: HALT
            0x3412U,                                    // 8: Data word (Bytes: 0x12, 0x34)
            0x7856U                                     // 9: Data word (Bytes: 0x56, 0x78)
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("LPM variations and Z-increment") {
        cpu.step(); // LDI ZL
        cpu.step(); // LDI ZH
        CHECK(cpu.registers()[30] == 0x10U);
        CHECK(cpu.registers()[31] == 0x00U);

        cpu.step(); // LPM (r0)
        auto s = cpu.snapshot();
        CHECK(s.gpr[0] == 0x12U);
        CHECK(s.cycles == 5U); // 1+1+3

        cpu.step(); // LPM r16, Z
        s = cpu.snapshot();
        CHECK(s.gpr[16] == 0x12U);
        CHECK(s.cycles == 8U); // 5+3

        cpu.step(); // LPM r17, Z+
        s = cpu.snapshot();
        CHECK(s.gpr[17] == 0x12U);
        CHECK(s.gpr[30] == 0x11U); // Z became 17
        CHECK(s.cycles == 11U); // 8+3

        cpu.step(); // LPM r18, Z+
        s = cpu.snapshot();
        CHECK(s.gpr[18] == 0x34U);
        CHECK(s.gpr[30] == 0x12U); // Z became 18
        CHECK(s.cycles == 14U); // 11+3
    }
}
