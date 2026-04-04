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

constexpr u16 encode_bset(const SregFlag flag_bit) {
    return static_cast<u16>(0x9408U | (static_cast<u16>(flag_bit) << 4U));
}

constexpr u16 encode_bclr(const SregFlag flag_bit) {
    return static_cast<u16>(0x9488U | (static_cast<u16>(flag_bit) << 4U));
}

constexpr u16 encode_branch_set(const SregFlag flag_bit, const int displacement) {
    return static_cast<u16>(0xF000U | ((displacement & 0x7FU) << 3U) | static_cast<u16>(flag_bit));
}

constexpr u16 encode_branch_clear(const SregFlag flag_bit, const int displacement) {
    return static_cast<u16>(0xF400U | ((displacement & 0x7FU) << 3U) | static_cast<u16>(flag_bit));
}

void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
} // namespace

TEST_CASE("CPU Branch Alias and Explicit SREG Bit Instructions Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_bset(SregFlag::overflow),              // 0: SEV
            encode_branch_set(SregFlag::overflow, 1),     // 1: BRVS +1 -> skip PC=2
            encode_ldi(16U, 0x11U),                       // 2 (Skipped)
            encode_bclr(SregFlag::overflow),              // 3: CLV
            encode_branch_clear(SregFlag::overflow, 1),   // 4: BRVC +1 -> skip PC=5
            encode_ldi(17U, 0x22U),                       // 5 (Skipped)
            encode_bset(SregFlag::sign),                  // 6: SES
            encode_branch_set(SregFlag::sign, 1),         // 7: BRLT +1 -> skip PC=8 (Note: BRLT is BRBS 4)
            encode_ldi(18U, 0x33U),                       // 8 (Skipped)
            encode_bclr(SregFlag::sign),                  // 9: CLS
            encode_branch_clear(SregFlag::sign, 1),       // 10: BRGE +1 -> skip PC=11
            encode_ldi(19U, 0x44U),                       // 11 (Skipped)
            encode_bset(SregFlag::halfCarry),             // 12: SEH
            encode_branch_set(SregFlag::halfCarry, 1),    // 13: BRHS +1 -> skip PC=14
            encode_ldi(20U, 0x55U),                       // 14 (Skipped)
            encode_bclr(SregFlag::halfCarry),             // 15: CLH
            encode_branch_clear(SregFlag::halfCarry, 1),  // 16: BRHC +1 -> skip PC=17
            encode_ldi(21U, 0x66U),                       // 17 (Skipped)
            encode_bset(SregFlag::transfer),              // 18: SET
            encode_branch_set(SregFlag::transfer, 1),     // 19: BRTS +1 -> skip PC=20
            encode_ldi(22U, 0x77U),                       // 20 (Skipped)
            encode_bclr(SregFlag::transfer),              // 21: CLT
            encode_branch_clear(SregFlag::transfer, 1),   // 22: BRTC +1 -> skip PC=23
            encode_ldi(23U, 0x88U),                       // 23 (Skipped)
            0x9478U,                                      // 24: SEI
            encode_branch_set(SregFlag::interrupt, 1),    // 25: BRIE +1 -> skip PC=26
            encode_ldi(24U, 0x99U),                       // 26 (Skipped)
            0x94F8U,                                      // 27: CLI
            encode_branch_clear(SregFlag::interrupt, 1),  // 28: BRID +1 -> skip PC=29
            encode_ldi(25U, 0xAAU),                       // 29 (Skipped)
            0x0000U                                       // 30: HALT marker
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("All branch aliases and SREG bit ops") {
        // Run until end
        cpu.run(100);
        
        auto s = cpu.snapshot();
        CHECK(s.program_counter == 31U);
        CHECK(cpu.state() == CpuState::halted);

        // Verify that no LDI was executed (all skipped by branches)
        for (int reg = 16; reg <= 25; ++reg) {
            CHECK(s.gpr[static_cast<std::size_t>(reg)] == 0U);
        }

        // Final SREG should be 0 because CLI was the last bit op
        CHECK(s.sreg == 0U);
    }
}
