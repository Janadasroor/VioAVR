#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/uart0.hpp"
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

constexpr vioavr::core::u16 encode_lds(const vioavr::core::u8 destination)
{
    return static_cast<vioavr::core::u16>(0x9000U | (static_cast<vioavr::core::u16>(destination) << 4U));
}

constexpr vioavr::core::u16 encode_sts(const vioavr::core::u8 source)
{
    return static_cast<vioavr::core::u16>(0x9200U | (static_cast<vioavr::core::u16>(source) << 4U));
}

}  // namespace

TEST_CASE("UART0 Firmware Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    Uart0 uart0 {"USART0", atmega328};
    bus.attach_peripheral(uart0);
    AvrCpu cpu {bus};

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x18U),  // RXEN0 | TXEN0
            encode_sts(16U), atmega328.uart0.ucsrb_address,
            encode_ldi(17U, 0x41U),
            encode_sts(17U), atmega328.uart0.udr_address,
            encode_lds(18U), atmega328.uart0.ucsra_address,
            encode_lds(19U), atmega328.uart0.ucsra_address,
            encode_lds(20U), atmega328.uart0.udr_address,
            0x0000U
        },
        .entry_word = 0U
    });

    cpu.reset();

    SUBCASE("Setup and Initial Transmit") {
        cpu.step(); // LDI RXEN|TXEN
        cpu.step(); // STS UCSRB
        CHECK(bus.read_data(atmega328.uart0.ucsrb_address) == 0x18U);
        CHECK(cpu.cycles() == 3U);
        
        cpu.step(); // LDI 'A'
        cpu.step(); // STS UDR
        CHECK(cpu.cycles() == 6U);
        
        vioavr::core::u8 transmitted = 0U;
        CHECK(uart0.consume_transmitted_byte(transmitted));
        CHECK(transmitted == 0x41U);
    }

    SUBCASE("Receive then Read") {
        for (int i = 0; i < 4; ++i) cpu.step();
        
        uart0.inject_received_byte(0xA5U);
        
        cpu.step(); // LDS R18, UCSRA
        CHECK(cpu.snapshot().gpr[18] == 0x80U); // RXC set
        
        cpu.step(); // LDS R19, UCSRA
        // After reading UCSRA, RXC usually stays until UDR read.
        // Wait, the test expects 0xE0? 
        // Bit 7(RXC), 6(TXC), 5(UDRE). 0xE0 means all three set.
        CHECK(cpu.snapshot().gpr[19] == 0xE0U);
        
        cpu.step(); // LDS R20, UDR
        CHECK(cpu.snapshot().gpr[20] == 0xA5U);
        
        // After UDR read, RXC should clear
        CHECK((bus.read_data(atmega328.uart0.ucsra_address) & 0x80U) == 0U);
    }
}
