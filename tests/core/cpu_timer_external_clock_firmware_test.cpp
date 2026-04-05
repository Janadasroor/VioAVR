#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328.hpp"

namespace {

constexpr vioavr::core::u16 encode_ldi(const vioavr::core::u8 destination, const vioavr::core::u8 immediate)
{
    return static_cast<vioavr::core::u16>(
        0xE000U |
        ((static_cast<vioavr::core::u16>(immediate & 0xF0U)) << 4U) |
        (static_cast<vioavr::core::u16>(destination - 16U) << 4U) |
        (immediate & 0x0FU)
    );
}

constexpr vioavr::core::u16 encode_in(const vioavr::core::u8 destination, const vioavr::core::u8 io_offset)
{
    return static_cast<vioavr::core::u16>(
        0xB000U |
        ((static_cast<vioavr::core::u16>(destination) & 0x1FU) << 4U) |
        ((static_cast<vioavr::core::u16>(io_offset) & 0x30U) << 5U) |
        (io_offset & 0x0FU)
    );
}

constexpr vioavr::core::u16 encode_out(const vioavr::core::u8 io_offset, const vioavr::core::u8 source)
{
    return static_cast<vioavr::core::u16>(
        0xB800U |
        ((static_cast<vioavr::core::u16>(source) & 0x1FU) << 4U) |
        ((static_cast<vioavr::core::u16>(io_offset) & 0x30U) << 5U) |
        (io_offset & 0x0FU)
    );
}

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
}  // namespace

TEST_CASE("Timer0 External Clock Firmware Test")
{
    using vioavr::core::AvrCpu;
    using vioavr::core::GpioPort;
    using vioavr::core::HexImage;
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328;

    constexpr auto pinb = static_cast<vioavr::core::u16>(0x23U);
    constexpr auto ddrb = static_cast<vioavr::core::u16>(0x24U);
    constexpr auto portb = static_cast<vioavr::core::u16>(0x25U);
    
    MemoryBus bus {atmega328};
    GpioPort port_d {"PORTD", atmega328.timer0.t0_pin_address, 0x11U, 0x12U};
    Timer8 timer0 {"TIMER0", atmega328};
    
    bus.attach_peripheral(port_d);
    bus.attach_peripheral(timer0);
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x02U),
            encode_out(0x27U, 16U),  // OCR0
            encode_ldi(17U, 0x07U),
            encode_out(0x25U, 17U),  // TCCR0B rising-edge external clock (0x45 -> 0x25)
            encode_in(18U, 0x26U),   // TCNT0 initial (0x46 -> 0x26)
            encode_in(19U, 0x26U),   // TCNT0 after first rising edge
            encode_in(20U, 0x15U),   // TIFR after second rising edge / compare (0x35 -> 0x15)
            encode_ldi(21U, 0x02U),
            encode_out(0x15U, 21U),  // clear OCF0A (clear on write-1)
            encode_ldi(22U, 0x06U),
            encode_out(0x25U, 22U),  // TCCR0B falling-edge external clock
            encode_in(23U, 0x26U),   // TCNT0 after falling edge
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
        port_d.set_input_levels(1U << atmega328.timer0.t0_pin_bit); // Rising edge
        bus.tick_peripherals(1);

        cpu.step(); // IN R19, TCNT0
        CHECK(cpu.snapshot().gpr[19] == 0x01U);
    }
}
