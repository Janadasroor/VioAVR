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

constexpr u16 encode_ijmp() { return 0x9409U; }
constexpr u16 encode_icall() { return 0x9509U; }
constexpr u16 encode_ret() { return 0x9508U; }

constexpr u16 encode_rjmp(const int displacement) {
    return static_cast<u16>(0xC000U | (displacement & 0x0FFF));
}

} // namespace

TEST_CASE("CPU Indirect Control Instructions Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};
    const auto initial_sp = atmega328.sram_range().end;

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(30U, 0x06U),                     // 0: ZL = 6
            encode_ldi(31U, 0x00U),                     // 1: ZH = 0
            encode_ijmp(),                              // 2: IJMP -> PC=6
            encode_ldi(16U, 0x11U),                     // 3 (Not reached)
            0x0000U,                                    // 4
            0x0000U,                                    // 5
            encode_ldi(17U, 0x22U),                     // 6: Reached via IJMP
            encode_ldi(30U, 0x0CU),                     // 7: ZL = 12
            encode_ldi(31U, 0x00U),                     // 8: ZH = 0
            encode_icall(),                             // 9: ICALL -> PC=12
            encode_ldi(18U, 0x33U),                     // 10: Target for RET
            encode_rjmp(2),                             // 11: End rjmp
            encode_ldi(19U, 0x44U),                     // 12: Subroutine reached via ICALL
            encode_ret(),                               // 13: RET -> PC=10
            0x0000U                                     // 14
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("IJMP Execution") {
        cpu.step(); // LDI ZL
        cpu.step(); // LDI ZH
        cpu.step(); // IJMP
        auto s = cpu.snapshot();
        CHECK(s.program_counter == 6U);
        CHECK(s.cycles == 4U); // LDI(1)+LDI(1)+IJMP(2)
        
        cpu.step(); // LDI r17, 0x22
        CHECK(cpu.snapshot().gpr[17] == 0x22U);
    }

    SUBCASE("ICALL and RET Execution") {
        cpu.run(7); // Run until ICALL
        CHECK(cpu.program_counter() == 9U);
        
        cpu.step(); // ICALL (3 cycles on most AVRs, check implementation)
        auto s = cpu.snapshot();
        CHECK(s.program_counter == 12U);
        CHECK(s.stack_pointer == initial_sp - 2U);
        
        // Verify return address (0x000A) on stack
        // AVR usually pushes High byte then Low byte.
        // [0x08FF] = High (0x00), [0x08FE] = Low (0x0A)? 
        // Or vice versa. Let's check original test assumption.
        CHECK(bus.read_data(initial_sp) == 0x0AU);
        CHECK(bus.read_data(initial_sp - 1U) == 0x00U);

        cpu.step(); // Step through subroutine (LDI r19, 0x44)
        CHECK(cpu.snapshot().gpr[19] == 0x44U);

        cpu.step(); // RET (4 or 5 cycles)
        s = cpu.snapshot();
        CHECK(s.program_counter == 10U);
        CHECK(s.stack_pointer == initial_sp);

        cpu.step(); // LDI r18, 0x33
        CHECK(cpu.snapshot().gpr[18] == 0x33U);
        
        cpu.step(); // RJMP 2 -> PC=14
        CHECK(cpu.program_counter() == 14U);
    }
}
