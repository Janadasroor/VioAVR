#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/uart8x.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X USART - Baud Rate Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Find first USART
    auto uart_list = machine.peripherals_of_type<Uart8x>();
    REQUIRE(!uart_list.empty());
    auto* uart = uart_list[0];

    // Configure 9600 baud @ 16MHz
    // BAUD = (64 * 16,000,000) / (16 * 9,600) = 6666.66 -> 6667
    const u16 baud_val = 6667;
    bus.write_data(0x0808, static_cast<u8>(baud_val & 0xFF)); // BAUDL
    bus.write_data(0x0809, static_cast<u8>(baud_val >> 8));   // BAUDH

    // Enable TX
    bus.write_data(0x0806, 0x40); // CTRLB.TXEN = 1

    // Initial Status: DREIF should be 1, TXCIF should be 0
    CHECK((bus.read_data(0x0804) & 0x20) != 0); // DREIF
    CHECK((bus.read_data(0x0804) & 0x40) == 0); // TXCIF

    // Write a byte to TXDATA
    bus.write_data(0x0802, 0xAA);

    // After 1 cycle, DREIF should be 1 (moved to shift reg)
    // and TXC should be 0.
    bus.tick_peripherals(1);
    CHECK((bus.read_data(0x0804) & 0x20) != 0); // DREIF
    CHECK((bus.read_data(0x0804) & 0x40) == 0); // TXC

    // Duration for 10 bits @ 9600 baud:
    // (10 * 16,000,000) / 9,600 = 16,666.66 cycles.
    // Let's tick 16,000 cycles. TXC should still be 0.
    bus.tick_peripherals(16000);
    CHECK((bus.read_data(0x0804) & 0x40) == 0);

    // Tick remaining duration
    bus.tick_peripherals(1000);
    CHECK((bus.read_data(0x0804) & 0x40) != 0); // TXCIF
}

TEST_CASE("AVR8X USART - Double Speed Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 baud_val = 13333;
    bus.write_data(0x0808, static_cast<u8>(baud_val & 0xFF));
    bus.write_data(0x0809, static_cast<u8>(baud_val >> 8));

    bus.write_data(0x0806, 0x40 | (0x01 << 1)); // TXEN + CLK2X

    bus.write_data(0x0802, 0x55);
    bus.tick_peripherals(1); // Move to shift reg

    bus.tick_peripherals(16400); // 16,666 is target
    CHECK((bus.read_data(0x0804) & 0x40) == 0);
    bus.tick_peripherals(500);
    CHECK((bus.read_data(0x0804) & 0x40) != 0);
}

TEST_CASE("AVR8X USART - Receiver Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* uart = machine.peripherals_of_type<Uart8x>()[0];

    // 9600 baud @ 16MHz
    const u16 baud_val = 6667;
    bus.write_data(0x0808, static_cast<u8>(baud_val & 0xFF));
    bus.write_data(0x0809, static_cast<u8>(baud_val >> 8));
    bus.write_data(0x0806, 0x80); // CTRLB.RXEN = 1

    // Inject byte
    uart->inject_received_byte(0x5A);

    // 1. RXSIF should be set immediately
    CHECK((bus.read_data(0x0804) & 0x10) != 0); // RXSIF
    // 2. RXCIF should NOT be set yet
    CHECK((bus.read_data(0x0804) & 0x80) == 0); // RXCIF

    // 3. Wait 16,000 cycles
    bus.tick_peripherals(16000);
    CHECK((bus.read_data(0x0804) & 0x80) == 0);

    // 4. Complete reception
    bus.tick_peripherals(1000);
    CHECK((bus.read_data(0x0804) & 0x80) != 0); // RXCIF
    CHECK(bus.read_data(0x0800) == 0x5A); // RXDATA
}
