#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_in(const u8 destination, const u8 io_offset) {
    return static_cast<u16>(0xB000U | ((static_cast<u16>(destination) & 0x1FU) << 4U) | ((static_cast<u16>(io_offset) & 0x30U) << 5U) | (io_offset & 0x0FU));
}

constexpr u16 encode_out(const u8 io_offset, const u8 source) {
    return static_cast<u16>(0xB800U | ((static_cast<u16>(source) & 0x1FU) << 4U) | ((static_cast<u16>(io_offset) & 0x30U) << 5U) | (io_offset & 0x0FU));
}

void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
} // namespace

TEST_CASE("CPU I/O Instruction and PIN Toggling Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    constexpr auto pinb_addr = static_cast<u16>(0x23U);
    constexpr auto ddrb_addr = static_cast<u16>(0x24U);
    constexpr auto portb_addr = static_cast<u16>(0x25U);

    MemoryBus bus {atmega328p};
    vioavr::core::PinMux pm_port_b { 10 }; GpioPort port_b { "PORTB", pinb_addr, ddrb_addr, portb_addr, pm_port_b };
    bus.attach_peripheral(port_b);
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x0FU),                     // 0
            encode_out(0x04U, 16U),                      // 1: DDRB = 0x0F
            encode_ldi(16U, 0x05U),                     // 2
            encode_out(0x05U, 16U),                      // 3: PORTB = 0x05
            encode_in(17U, 0x03U),                       // 4: r17 = PINB
            encode_in(18U, 0x04U),                       // 5: r18 = DDRB
            encode_in(19U, 0x05U),                       // 6: r19 = PORTB
            encode_ldi(20U, 0x03U),                     // 7
            encode_out(0x03U, 20U),                      // 8: OUT PINB, 0x03 (Toggles PORTB[0:1])
            encode_in(21U, 0x05U),                       // 9: r21 = PORTB (Expected 0x05 ^ 0x03 = 0x06)
            0x0000U                                     // 10
        },
        .entry_word = 0U
    });

    cpu.reset();
    port_b.set_input_levels(0xA0U); // Inputs are 0xA0

    SUBCASE("I/O operations and PIN toggle logic") {
        step_to(cpu, 10U);
        auto s = cpu.snapshot();
        
        CHECK(port_b.ddr_register() == 0x0FU);
        CHECK(port_b.port_register() == 0x06U);
        
        // PINB reflects (Inputs & ~DDR) | (PORT & DDR)
        // PINB = (0xA0 & 0xF0) | (0x05 & 0x0F) = 0xA0 | 0x05 = 0xA5
        CHECK(s.gpr[17] == 0xA5U); 
        CHECK(s.gpr[18] == 0x0FU);
        CHECK(s.gpr[19] == 0x05U);
        CHECK(s.gpr[21] == 0x06U); // After toggle
        
        CHECK(port_b.output_levels() == 0x06U);
        CHECK(cpu.state() == CpuState::running);
    }
}
