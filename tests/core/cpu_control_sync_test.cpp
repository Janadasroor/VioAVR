#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_out(const u8 io_offset, const u8 source) {
    return static_cast<u16>(0xB800U | ((static_cast<u16>(source) & 0x1FU) << 4U) | ((static_cast<u16>(io_offset) & 0x30U) << 5U) | (io_offset & 0x0FU));
}

} // namespace

TEST_CASE("CPU Control Synchronization")
{
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    // Note: NOT attaching CpuControl yet

    SUBCASE("SREG synchronization via OUT") {
        // LDI R16, 0xFF
        // OUT SREG, R16
        std::vector<u16> code = {
            encode_ldi(16, 0xFF),
            encode_out(0x3F, 16), // SREG IO offset 0x3F
            0x0000
        };
        bus.load_flash(code);
        cpu.reset();

        cpu.step(); // LDI
        cpu.step(); // OUT SREG
        
        // This is expected to FAIL currently if CpuControl is not attached
        CHECK(cpu.sreg() == 0xFF); 
    }

    SUBCASE("SP synchronization via OUT") {
        // LDI R16, 0x12
        // OUT SPL, R16
        // LDI R16, 0x34
        // OUT SPH, R16
        std::vector<u16> code = {
            encode_ldi(16, 0x12),
            encode_out(0x3D, 16), // SPL
            encode_ldi(16, 0x34),
            encode_out(0x3E, 16), // SPH
            0x0000
        };
        bus.load_flash(code);
        cpu.reset();

        cpu.step(); // LDI
        cpu.step(); // OUT SPL
        cpu.step(); // LDI
        cpu.step(); // OUT SPH
        
        CHECK(cpu.stack_pointer() == 0x3412);
    }
}
