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

constexpr u16 encode_sts(const u8 source) {
    return static_cast<u16>(0x9200U | (static_cast<u16>(source) << 4U));
}

constexpr u16 kSpm = 0x95E8U;

} // namespace

TEST_CASE("CPU SPM Boundary and Halt Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(30U, 0x00U),                 // 0: Z_low
            encode_ldi(31U, 0x00U),                 // 1: Z_high
            encode_ldi(16U, 0x01U),                 // 2: Val=1
            encode_sts(16U),                        // 3: STS
            atmega328p.spmcsr_address,               // 4: SPMCSR address
            kSpm,                                   // 5: SPM instruction
            encode_ldi(18U, 0x55U),                 // 6: Next instr
            0x0000U                                 // 7: NOP
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("SPM executes without halt") {
        const auto spmcsr_addr = bus.device().spmcsr_address;
        
        // Execute until right before SPM.
        // Instrs: LDI(0), LDI(1), LDI(2), STS(3,4). Total 4 instructions.
        // STS takes 2 cycles. LDI takes 1 cycle. Total = 1+1+1+2 = 5 cycles.
        for (int i = 0; i < 4; ++i) cpu.step();
        
        CHECK(cpu.program_counter() == 5U);
        CHECK(cpu.state() == CpuState::running);

        // Execute SPM (Should NOT halt anymore)
        cpu.step();
        
        CHECK(cpu.state() == CpuState::running);
        CHECK(cpu.program_counter() == 6U);
        
        // SPMCSR SPMEN bit should be cleared by hardware
        CHECK((bus.read_data(spmcsr_addr) & 0x01U) == 0U);

        // Further runs should complete the next LDI
        cpu.step();
        CHECK(cpu.registers()[18] == 0x55U);
        CHECK(cpu.program_counter() == 7U);
    }
}
