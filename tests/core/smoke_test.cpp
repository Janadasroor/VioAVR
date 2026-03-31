#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega8.hpp"

TEST_CASE("AVR Core Smoke Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega8};
    AvrCpu cpu {bus};

    SUBCASE("Initial state") {
        CHECK(cpu.state() == CpuState::halted);
        cpu.run(4);
        CHECK(cpu.cycles() == 0);
    }

    SUBCASE("Basic Execution") {
        const auto image = HexImage {
            .flash_words = {0x0000U, 0x0000U, 0x0000U, 0x0000U},
            .entry_word = 1U
        };
        bus.load_image(image);
        cpu.reset();
        cpu.run(4);
        const auto snapshot = cpu.snapshot();
        
        CHECK(snapshot.cycles == 3);
        CHECK(snapshot.program_counter == 4);
        CHECK(snapshot.stack_pointer == atmega8.sram_range().end);
    }
}
