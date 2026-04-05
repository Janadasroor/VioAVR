#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;

TEST_CASE("CPU Trace Utility Test")
{
    MemoryBus bus {devices::atmega328};
    AvrCpu cpu {bus};

    HexImage image {
        .flash_words = {
            0xE000U, // ldi r16, 0x00
            0xE110U, // ldi r17, 0x10
            0x0000U, // nop
        },
        .entry_word = 0U
    };
    bus.load_image(image);
    cpu.reset();

    SUBCASE("Step through instructions") {
        CHECK(cpu.program_counter() == 0U);
        cpu.step();
        CHECK(cpu.program_counter() == 1U);
        CHECK(cpu.snapshot().gpr[16] == 0x00U);
        cpu.step();
        CHECK(cpu.program_counter() == 2U);
        CHECK(cpu.snapshot().gpr[17] == 0x10U);
        cpu.step();
        CHECK(cpu.program_counter() == 3U);
    }
}
