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

constexpr u16 encode_push(const u8 source) {
    return static_cast<u16>(0x920FU | (static_cast<u16>(source) << 4U));
}

constexpr u16 encode_pop(const u8 destination) {
    return static_cast<u16>(0x900FU | (static_cast<u16>(destination) << 4U));
}

constexpr u16 encode_call_high() { return 0x940EU; }
constexpr u16 encode_ret() { return 0x9508U; }

constexpr u16 encode_rjmp(const int displacement) {
    return static_cast<u16>(0xC000U | (displacement & 0x0FFF));
}

void step_to(AvrCpu& cpu, u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) {
        cpu.step();
    }
}

} // namespace

TEST_CASE("CPU Stack Operations Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};
    const auto initial_sp = atmega328.sram_range().end;

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x42U),                     // 0
            encode_push(16U),                           // 1
            encode_ldi(16U, 0x00U),                     // 2
            encode_pop(17U),                            // 3
            encode_call_high(), 0x0008U,                // 4, 5: CALL PC=8
            encode_ldi(18U, 0xAAU),                     // 6: Return target
            encode_rjmp(2),                             // 7: End rjmp (to 10)
            encode_ldi(19U, 0x55U),                     // 8: Subroutine target
            encode_ret(),                               // 9
            0x0000U                                     // 10: HALT
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("PUSH and POP") {
        cpu.step(); // LDI r16, 0x42
        cpu.step(); // PUSH r16
        auto s = cpu.snapshot();
        CHECK(s.stack_pointer == initial_sp - 1U);
        CHECK(bus.read_data(initial_sp) == 0x42U);

        cpu.step(); // LDI r16, 0x00
        cpu.step(); // POP r17
        s = cpu.snapshot();
        CHECK(s.gpr[17] == 0x42U);
        CHECK(s.stack_pointer == initial_sp);
    }

    SUBCASE("CALL and RET") {
        step_to(cpu, 4U); // Run until CALL
        CHECK(cpu.program_counter() == 4U);
        
        cpu.step(); // CALL
        auto s = cpu.snapshot();
        CHECK(s.program_counter == 8U);
        CHECK(s.stack_pointer == initial_sp - 2U);
        CHECK(bus.read_data(initial_sp) == 0x06U); // Low byte of return address
        CHECK(bus.read_data(initial_sp - 1U) == 0x00U); // High byte

        cpu.step(); // LDI r19, 0x55
        cpu.step(); // RET
        s = cpu.snapshot();
        CHECK(s.program_counter == 6U);
        CHECK(s.stack_pointer == initial_sp);
    }
}
