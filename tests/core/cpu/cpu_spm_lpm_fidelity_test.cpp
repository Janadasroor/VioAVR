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

constexpr u16 encode_sts(const u16 address, const u8 source) {
    return static_cast<u16>(0x9200U | (static_cast<u16>(source) << 4U));
}

constexpr u16 encode_lpm_z(const u8 destination) { return static_cast<u16>(0x9004U | (static_cast<u16>(destination) << 4U)); }

} // namespace

TEST_CASE("CPU SPM/LPM Fidelity Test (Signature and Fuse Reading)")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};

    const u16 spmcsr_addr = 0x57;

    SUBCASE("Read Signature Bytes") {
        bus.load_image(HexImage {
            .flash_words = {
                encode_ldi(16, 0x20),        // 0: r16 = 0x20 (SIGRD)
                encode_sts(spmcsr_addr, 16), // 1: STS 0x57, r16
                spmcsr_addr,                 // 2:
                encode_ldi(30, 0x00),        // 3: ZL = 0
                encode_ldi(31, 0x00),        // 4: ZH = 0
                encode_lpm_z(17),            // 5: r17 = Signature[0]
                
                encode_ldi(16, 0x20),        // 6: SIGRD again
                encode_sts(spmcsr_addr, 16), // 7:
                spmcsr_addr,                 // 8:
                encode_ldi(30, 0x02),        // 9: ZL = 2
                encode_lpm_z(18),            // 10: r18 = Signature[1]
                
                encode_ldi(16, 0x20),        // 11: SIGRD again
                encode_sts(spmcsr_addr, 16), // 12:
                spmcsr_addr,                 // 13:
                encode_ldi(30, 0x04),        // 14: ZL = 4
                encode_lpm_z(19),            // 15: r19 = Signature[2]
                0x0000U
            },
            .entry_word = 0U
        });
        cpu.reset();

        cpu.step(); // LDI r16, 0x20
        cpu.step(); // STS 0x57, r16
        cpu.step(); // LDI ZL, 0
        cpu.step(); // LDI ZH, 0
        cpu.step(); // LPM r17, Z
        CHECK(cpu.registers()[17] == 0x1EU);

        cpu.step(); // LDI r16, 0x20
        cpu.step(); // STS 0x57, r16
        cpu.step(); // LDI ZL, 2
        cpu.step(); // LPM r18, Z
        CHECK(cpu.registers()[18] == 0x95U);

        cpu.step(); // LDI r16, 0x20
        cpu.step(); // STS 0x57, r16
        cpu.step(); // LDI ZL, 4
        cpu.step(); // LPM r19, Z
        CHECK(cpu.registers()[19] == 0x0FU);
        
        cpu.step(); // NOP
        CHECK((bus.read_data(spmcsr_addr) & 0x20U) == 0U); // Should be self-cleared
    }

    SUBCASE("Read Fuses and Lockbits") {
        bus.load_image(HexImage {
            .flash_words = {
                encode_ldi(16, 0x08),        // 0: r16 = 0x08 (BLBSET)
                encode_sts(spmcsr_addr, 16), // 1: STS 0x57, r16
                spmcsr_addr,                 // 2:
                encode_ldi(30, 0x00),        // 3: ZL = 0
                encode_ldi(31, 0x00),        // 4: ZH = 0
                encode_lpm_z(17),            // 5: r17 = Low Fuse
                
                encode_ldi(16, 0x08),        // 6: BLBSET again
                encode_sts(spmcsr_addr, 16), // 7:
                spmcsr_addr,                 // 8:
                encode_ldi(30, 0x01),        // 9: ZL = 1
                encode_lpm_z(18),            // 10: r18 = Lockbits
                
                encode_ldi(16, 0x08),        // 11: BLBSET again
                encode_sts(spmcsr_addr, 16), // 12:
                spmcsr_addr,                 // 13:
                encode_ldi(30, 0x02),        // 14: ZL = 2
                encode_lpm_z(19),            // 15: r19 = Ext Fuse
                
                encode_ldi(16, 0x08),        // 16: BLBSET again
                encode_sts(spmcsr_addr, 16), // 17:
                spmcsr_addr,                 // 18:
                encode_ldi(30, 0x03),        // 19: ZL = 3
                encode_lpm_z(20),            // 20: r20 = High Fuse
                0x0000U
            },
            .entry_word = 0U
        });
        cpu.reset();

        cpu.step(); cpu.step(); cpu.step(); cpu.step(); cpu.step();
        CHECK(cpu.registers()[17] == 0x62U);

        cpu.step(); cpu.step(); cpu.step(); cpu.step();
        CHECK(cpu.registers()[18] == 0xFFU);

        cpu.step(); cpu.step(); cpu.step(); cpu.step();
        CHECK(cpu.registers()[19] == 0xFFU);

        cpu.step(); cpu.step(); cpu.step(); cpu.step();
        CHECK(cpu.registers()[20] == 0xD9U);
        
        cpu.step();
        CHECK((bus.read_data(spmcsr_addr) & 0x08U) == 0U);
    }
}
