#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/uart0.hpp"
#include "vioavr/core/devices/atmega328.hpp"

TEST_CASE("UART0 Peripheral Functional Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    Uart0 uart0 {"USART0", atmega328};
    bus.attach_peripheral(uart0);
    bus.reset();

    const auto ucsra = atmega328.uart0.ucsra_address;
    const auto ucsrb = atmega328.uart0.ucsrb_address;
    const auto udr = atmega328.uart0.udr_address;

    SUBCASE("Default status and transmission") {
        // UDRE (bit 5) should be set on reset. TXC (bit 6) is cleared on reset.
        CHECK((bus.read_data(ucsra) & 0x60U) == 0x20U);

        bus.write_data(ucsrb, 0x18U); // TXEN, RXEN
        bus.write_data(udr, 0x5AU);

        // UDRE and TXC should be cleared while transmitting
        CHECK((bus.read_data(ucsra) & 0x60U) == 0x00U);

        // Tick to complete transmission 
        // With default UBRR=0, duration is 160 cycles (10 bits * 16 cycles/bit)
        bus.tick_peripherals(80U);
        CHECK((bus.read_data(ucsra) & 0x40U) == 0x00U); // TXC still clear

        bus.tick_peripherals(81U);
        CHECK((bus.read_data(ucsra) & 0x40U) == 0x40U); // TXC set

        u8 transmitted = 0U;
        CHECK(uart0.consume_transmitted_byte(transmitted));
        CHECK(transmitted == 0x5AU);
    }

    SUBCASE("Reception and Flag Clearing") {
        uart0.inject_received_byte(0xA5U);
        
        // RXC (bit 7) should be set
        CHECK((bus.read_data(ucsra) & 0x80U) != 0U);

        // Reading UDR should return data and clear RXC
        CHECK(bus.read_data(udr) == 0xA5U);
        CHECK((bus.read_data(ucsra) & 0x80U) == 0U);
    }
}
