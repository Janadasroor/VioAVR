#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate)
{
    return static_cast<u16>(
        0xE000U |
        ((static_cast<u16>(immediate & 0xF0U)) << 4U) |
        (static_cast<u16>(destination - 16U) << 4U) |
        (immediate & 0x0FU)
    );
}

}  // namespace

TEST_CASE("Timer16 basic functionality")
{
    using namespace vioavr::core::devices;
    
    MemoryBus bus {atmega328p};
    Timer16 timer1 {"TIMER1", atmega328p.timers16[0]};
    timer1.set_bus(bus);
    bus.attach_peripheral(timer1);
    AvrCpu cpu {bus};

    SUBCASE("16-bit register access protocol") {
        std::vector<u16> code = {
            encode_ldi(16, 0x12),
            0x9300U, static_cast<u16>(atmega328p.timers16[0].tcnt_address + 1), // STS TCNT1H, R16
            encode_ldi(16, 0x34),
            0x9300U, static_cast<u16>(atmega328p.timers16[0].tcnt_address),     // STS TCNT1L, R16
            0x0000 // NOP
        };
        bus.load_flash(code);
        cpu.reset();

        cpu.step(); // LDI
        cpu.step(); // STS TCNT1H (2 words)
        CHECK(timer1.counter() == 0x0000); 
        
        cpu.step(); // LDI
        cpu.step(); // STS TCNT1L (2 words)
        CHECK(timer1.counter() == 0x1234); 
    }

    SUBCASE("Normal mode counting") {
        std::vector<u16> code = {
            encode_ldi(16, 0x01),
            0x9300, atmega328p.timers16[0].tccrb_address,
            0x0000, 0x0000, 0x0000
        };
        bus.load_flash(code);
        cpu.reset();

        cpu.step(); // LDI
        cpu.step(); // STS TCCR1B (Timer enabled here, ticks 2 cycles)
        CHECK(timer1.counter() == 2);
        
        cpu.step(); // NOP
        CHECK(timer1.counter() == 3);
        cpu.step(); // NOP
        CHECK(timer1.counter() == 4);
    }

    SUBCASE("CTC mode (Clear Timer on Compare Match)") {
        std::vector<u16> code = {
            encode_ldi(16, 0x00),
            0x9300U, static_cast<u16>(atmega328p.timers16[0].ocra_address + 1),
            encode_ldi(16, 0x0A),
            0x9300U, static_cast<u16>(atmega328p.timers16[0].ocra_address),
            encode_ldi(16, 0x09),
            0x9300U, static_cast<u16>(atmega328p.timers16[0].tccrb_address),
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
        };
        bus.load_flash(code);
        cpu.reset();

        cpu.step(); // LDI
        cpu.step(); // STS OCRA1H
        cpu.step(); // LDI
        cpu.step(); // STS OCRA1L
        cpu.step(); // LDI
        cpu.step(); // STS TCCR1B (Timer enabled, ticks 2 cycles)
        
        CHECK(timer1.counter() == 2);
        
        for (int i = 2; i < 10; ++i) {
            cpu.step(); // TCNT increments
            CHECK(timer1.counter() == i + 1);
        }
        cpu.step(); // TCNT reaches TOP (10) and should reset to 0
        CHECK(timer1.counter() == 0);
    }

    SUBCASE("Input Capture") {
        GpioPort port {"PORTB", 0x23, 0x24, 0x25};
        bus.attach_peripheral(port);
        timer1.connect_input_capture(port, 0);
        
        std::vector<u16> code = {
            encode_ldi(16, 0x41),
            0x9300, atmega328p.timers16[0].tccrb_address,
            0x0000, 0x0000, 0x0000
        };
        bus.load_flash(code);
        cpu.reset();

        cpu.step(); // LDI
        cpu.step(); // STS (ticks 2)
        
        CHECK(timer1.counter() == 2);
        
        port.set_input_levels(0x01); // PINB0 = 1
        cpu.step(); // NOP (ticks 1) -> TCNT increments to 3
        
        CHECK(timer1.input_capture() == 3);
    }
}
