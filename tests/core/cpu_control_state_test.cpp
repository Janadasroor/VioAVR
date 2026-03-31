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

constexpr u16 encode_sleep() { return 0x9588U; }
constexpr u16 encode_break() { return 0x9598U; }
constexpr u16 encode_wdr() { return 0x95A8U; }

} // namespace

TEST_CASE("CPU Control and State Transition Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    AvrCpu cpu {bus};

    SUBCASE("Sleep and Watchdog Reset") {
        bus.load_image(HexImage {
            .flash_words = {
                encode_ldi(16U, 0x12U),                 // 0
                encode_wdr(),                           // 1
                encode_sleep(),                         // 2
                encode_ldi(17U, 0x34U),                 // 3
                0x0000U                                 // 4
            },
            .entry_word = 0U
        });
        cpu.reset();

        cpu.step(); // LDI r16, 0x12
        CHECK(cpu.registers()[16] == 0x12U);
        CHECK(cpu.program_counter() == 1U);

        cpu.step(); // WDR
        CHECK(cpu.program_counter() == 2U);
        // Note: Original test expected 'halted' here, which is highly suspect for WDR.
        // We expect 'running' since WDR is just a NOP if no watchdog is attached.
        // If this fails, we'll investigate why the simulator halts on WDR.
        CHECK(cpu.state() == CpuState::running);

        cpu.step(); // SLEEP
        CHECK(cpu.program_counter() == 3U);
        CHECK(cpu.state() == CpuState::sleeping);

        // While sleeping, step should not execute the next instruction but advance cycles
        cpu.step();
        CHECK(cpu.program_counter() == 3U);
        CHECK(cpu.state() == CpuState::sleeping);
        CHECK(cpu.registers()[17] == 0x00U);
        
        cpu.run(10);
        CHECK(cpu.state() == CpuState::sleeping);
        CHECK(cpu.program_counter() == 3U);
    }

    SUBCASE("Break Instruction") {
        bus.load_image(HexImage {
            .flash_words = {
                encode_break(),                         // 0
                encode_ldi(18U, 0x56U),                 // 1
                0x0000U                                 // 2
            },
            .entry_word = 0U
        });
        cpu.reset();

        cpu.step(); // BREAK
        CHECK(cpu.program_counter() == 1U);
        CHECK(cpu.state() == CpuState::paused);

        // Step while paused should do nothing
        cpu.step();
        CHECK(cpu.program_counter() == 1U);
        CHECK(cpu.state() == CpuState::paused);
        CHECK(cpu.registers()[18] == 0x00U);
    }
}
