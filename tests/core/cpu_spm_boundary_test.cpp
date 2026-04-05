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

    SUBCASE("SPM executes without halt") {
        const auto spmcsr_addr = bus.device().spmcsr_address;
        
        // Setup SPMCSR to enable SPM (SPMEN = 1)
        bus.write_data(spmcsr_addr, 0x01U);

        // Run until right before SPM (4 instructions already ran)
        cpu.run(4);
        CHECK(cpu.program_counter() == 4U);
        CHECK(cpu.state() == CpuState::running);

        // Execute SPM (Should NOT halt anymore)
        cpu.step();
        
        CHECK(cpu.state() == CpuState::running);
        CHECK(cpu.program_counter() == 5U);
        
        // SPMCSR SPMEN bit should be cleared by hardware
        CHECK((bus.read_data(spmcsr_addr) & 0x01U) == 0U);

        // Further runs should complete the next LDI
        cpu.run(1);
        CHECK(cpu.registers()[18] == 0x55U);
        CHECK(cpu.program_counter() == 6U);
    }
}
