#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

namespace {

using namespace vioavr::core;
using namespace vioavr::core::devices;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate)
{
    return static_cast<u16>(
        0xE000U |
        ((static_cast<u16>(immediate & 0xF0U)) << 4U) |
        (static_cast<u16>(destination - 16U) << 4U) |
        (immediate & 0x0FU)
    );
}

constexpr u16 encode_in(const u8 destination, const u8 io_offset)
{
    return static_cast<u16>(
        0xB000U |
        ((static_cast<u16>(destination) & 0x1FU) << 4U) |
        ((static_cast<u16>(io_offset) & 0x30U) << 5U) |
        (io_offset & 0x0FU)
    );
}

constexpr u16 encode_out(const u8 io_offset, const u8 source)
{
    return static_cast<u16>(
        0xB800U |
        ((static_cast<u16>(source) & 0x1FU) << 4U) |
        ((static_cast<u16>(io_offset) & 0x30U) << 5U) |
        (io_offset & 0x0FU)
    );
}

}  // namespace

TEST_CASE("Timer0 External Clock Logic Firmware Test")
{
    MemoryBus bus {atmega328p};
    // Use descriptor to find correct port for T0 (it's PORTD bit 4 on 328P)
    const u16 port_addr = atmega328p.ports[2].port_address; // PORTD
    const u16 ddr_addr  = atmega328p.ports[2].ddr_address;
    const u16 pin_addr  = atmega328p.ports[2].pin_address;
    
    GpioPort port_d {"PORTD", pin_addr, ddr_addr, port_addr};
    Timer8 timer0 {"TIMER0", atmega328p.timers8[0]};
    
    bus.attach_peripheral(port_d);
    bus.attach_peripheral(timer0);
    AvrCpu cpu {bus};

    const u8 ocr0_io = static_cast<u8>(atmega328p.timers8[0].ocra_address - 0x20);
    const u8 tccrb_io = static_cast<u8>(atmega328p.timers8[0].tccrb_address - 0x20);
    const u8 tcnt0_io = static_cast<u8>(atmega328p.timers8[0].tcnt_address - 0x20);
    const u8 tifr_io = static_cast<u8>(atmega328p.timers8[0].tifr_address - 0x20);

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x02U),
            encode_out(ocr0_io, 16U),
            encode_ldi(17U, 0x07U),
            encode_out(tccrb_io, 17U),  // TCCR0B rising-edge external clock
            encode_in(18U, tcnt0_io),   // TCNT0 initial
            encode_in(19U, tcnt0_io),   // TCNT0 after first rising edge
            encode_in(20U, tifr_io),    // TIFR after second rising edge / compare
            encode_ldi(21U, 0x02U),
            encode_out(tifr_io, 21U),   // clear OCF0A (clear on write-1)
            encode_ldi(22U, 0x06U),
            encode_out(tccrb_io, 22U),  // TCCR0B falling-edge external clock
            encode_in(23U, tcnt0_io),   // TCNT0 after falling edge
            0x0000U
        },
        .entry_word = 0U
    });

    cpu.reset();
    port_d.set_input_levels(0x00U);

    SUBCASE("Setup and Running Check") {
        for (int i = 0; i < 4; ++i) cpu.step();
        const auto snapshot = cpu.snapshot();
        CHECK(timer0.running());
        CHECK(timer0.compare_a() == 0x02U);
        CHECK(timer0.control_b() == 0x07U);
        CHECK(snapshot.cycles == 4U);
    }

    SUBCASE("External Clock - Risingedge") {
        for (int i = 0; i < 4; ++i) cpu.step();
        cpu.step(); // IN R18, TCNT0
        CHECK(cpu.snapshot().gpr[18] == 0x00U);

        port_d.set_input_levels(0x00U);
        bus.tick_peripherals(1);
        // T0 pin = atmega328p.timers8[0].t_pin_bit
        port_d.set_input_levels(1U << atmega328p.timers8[0].t_pin_bit); // Rising edge
        bus.tick_peripherals(1);

        cpu.step(); // IN R19, TCNT0
        CHECK(cpu.snapshot().gpr[19] == 0x01U);
    }
}
