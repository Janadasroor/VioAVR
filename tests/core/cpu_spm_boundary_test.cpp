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

constexpr u16 kSpm = 0x95E8U;

} // namespace

TEST_CASE("CPU SPM Boundary and Halt Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(30U, 0x00U),                 // 0
            encode_ldi(31U, 0x00U),                 // 1
            encode_ldi(16U, 0x34U),                 // 2
            encode_ldi(17U, 0x12U),                 // 3
            kSpm,                                   // 4: SPM instruction
            encode_ldi(18U, 0x55U),                 // 5
            0x0000U                                 // 6
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("SPM causes halt (current implementation)") {
        const auto original_word0 = bus.read_program_word(0U);

        // Run until right before SPM
        cpu.run(4);
        CHECK(cpu.program_counter() == 4U);
        CHECK(cpu.state() == CpuState::running);

        // Execute SPM
        cpu.step();
        
        // SPM is currently a halt instruction in this simulator version
        CHECK(cpu.state() == CpuState::halted);
        CHECK(cpu.program_counter() == 4U);
        
        // Ensure flash was NOT modified (SPM usually writes flash, but here it shouldn't have)
        CHECK(bus.read_program_word(0U) == original_word0);
        
        // Further runs should do nothing
        cpu.run(10);
        CHECK(cpu.state() == CpuState::halted);
        CHECK(cpu.program_counter() == 4U);
        CHECK(cpu.registers()[18] == 0x00U); // Instruction 5 never ran
    }
}
