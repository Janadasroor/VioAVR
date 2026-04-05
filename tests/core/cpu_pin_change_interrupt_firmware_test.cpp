#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
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

constexpr vioavr::core::u16 encode_sts(const vioavr::core::u8 source)
{
    return static_cast<vioavr::core::u16>(0x9200U | (static_cast<vioavr::core::u16>(source) << 4U));
}

constexpr vioavr::core::u16 kSei = 0x9478U;
constexpr vioavr::core::u16 kReti = 0x9518U;
constexpr vioavr::core::u16 kNop = 0x0000U;

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
}  // namespace

TEST_CASE("Pin Change Interrupt Firmware Test")
{
    using vioavr::core::AvrCpu;
    using vioavr::core::GpioPort;
    using vioavr::core::HexImage;
    using vioavr::core::MemoryBus;
    using vioavr::core::PinChangeInterrupt;
    using vioavr::core::SregFlag;
    using vioavr::core::devices::atmega328;

    constexpr auto pinb = static_cast<vioavr::core::u16>(0x23U);
    constexpr auto ddrb = static_cast<vioavr::core::u16>(0x24U);
    constexpr auto portb = static_cast<vioavr::core::u16>(0x25U);
    
    MemoryBus bus {atmega328};
    GpioPort port_b {"PORTB", pinb, ddrb, portb};
    PinChangeInterruptDescriptor pci0_desc { .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6BU, .pcicr_enable_mask = 0x01U, .pcifr_flag_mask = 0x01U, .vector_index = 3U };
    PinChangeInterrupt pci0 {"PCINT0", pci0_desc, port_b};
    
    bus.attach_peripheral(port_b);
    bus.attach_peripheral(pci0);
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            kNop,                     // 0
            kNop,                     // 1
            kNop,                     // 2
            encode_ldi(19U, 0xA5U),   // 3 ISR body
            kReti,                    // 4
            encode_ldi(16U, 0x04U),   // 5 mask PB2
            encode_sts(16U), 0x6BU,  // 6,7
            encode_ldi(17U, 0x01U),   // 8 enable group
            encode_sts(17U), 0x68U,   // 9,10
            kSei,                     // 11
            kNop,                     // 12 interrupted point
            encode_ldi(18U, 0x55U),   // 13 mainline after ISR
            kNop                      // 14
        },
        .entry_word = 5U
    });

    cpu.reset();

    SUBCASE("Setup and Mainline execution") {
        for (int i = 0; i < 5; ++i) cpu.step();
        const auto snapshot = cpu.snapshot();
        CHECK(snapshot.program_counter == 12U);
        CHECK((snapshot.sreg & (1U << static_cast<vioavr::core::u8>(SregFlag::interrupt))) != 0U);
    }

    SUBCASE("Pin Change and Wake/ISR") {
        for (int i = 0; i < 5; ++i) cpu.step();
        
        // Trigger pin change on PB2
        port_b.set_input_levels(0x00U);
        bus.tick_peripherals(1U);
        port_b.set_input_levels(0x04U);
        bus.tick_peripherals(1U);

        cpu.step(); // Service interrupt
        auto snapshot = cpu.snapshot();
        const auto ramend = atmega328.sram_range().end;
        
        // CHECK(snapshot.program_counter == 3U);
        CHECK(snapshot.stack_pointer == static_cast<vioavr::core::u16>(ramend - 2U));
        CHECK(snapshot.cycles == 11U);
        CHECK(snapshot.in_interrupt_handler);

        cpu.step(); // LDI R19, 0xA5
        snapshot = cpu.snapshot();
        CHECK(snapshot.gpr[19] == 0xA5U);
        CHECK(snapshot.program_counter == 4U);

        cpu.step(); // RETI
        snapshot = cpu.snapshot();
        CHECK(snapshot.program_counter == 12U);
        CHECK(snapshot.stack_pointer == ramend);
        CHECK_FALSE(snapshot.in_interrupt_handler);

        cpu.step(); // LDI R18, 0x55
        snapshot = cpu.snapshot();
        CHECK(snapshot.gpr[18] == 0x55U);
        CHECK(snapshot.program_counter == 14U);
    }
}
