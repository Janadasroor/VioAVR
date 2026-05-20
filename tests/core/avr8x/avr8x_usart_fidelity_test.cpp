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

    // Inject byte via Pin (0x5A = 01011010)
    const auto& desc = uart->descriptor();
    auto drive_bit = [&](bool level) {
        // Use a high-priority owner to drive the pin despite UART claiming it as input
        machine.pin_mux().claim_pin_by_address(desc.rxd_pin_address, desc.rxd_pin_bit, static_cast<PinOwner>(15));
        machine.pin_mux().update_pin_by_address(desc.rxd_pin_address, desc.rxd_pin_bit, static_cast<PinOwner>(15), true, level);
        bus.tick_peripherals(1667); // Use 1667 to ensure bit-duration (1666.75) is exceeded
    };

    // Idle
    drive_bit(true);

    // Start bit
    drive_bit(false);
    
    // 1. RXSIF should be set now (during reception)
    CHECK((bus.read_data(0x0804) & 0x10) != 0); 
    
    // Data bits (0x5A = LSB: 0, 1, 0, 1, 1, 0, 1, 0)
    drive_bit(false); // Bit 0: 0
    drive_bit(true);  // Bit 1: 1
    drive_bit(false); // Bit 2: 0
    drive_bit(true);  // Bit 3: 1
    drive_bit(true);  // Bit 4: 1
    drive_bit(false); // Bit 5: 0
    drive_bit(true);  // Bit 6: 1
    drive_bit(false); // Bit 7: 0
    
    // Stop bit
    drive_bit(true);

    // 4. RXCIF should be set bit now
    CHECK((bus.read_data(0x0804) & 0x80) != 0);
    CHECK(bus.read_data(0x0800) == 0x5A); // RXDATA
}

TEST_CASE("AVR8X USART - PORTMUX Routing Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // 1. Configure USART0 for TX @ 9600 baud, 16MHz
    const u16 baud_val = 6667;
    bus.write_data(0x0808, static_cast<u8>(baud_val & 0xFF)); // BAUDL
    bus.write_data(0x0809, static_cast<u8>(baud_val >> 8));   // BAUDH
    bus.write_data(0x0806, 0x40); // CTRLB.TXEN = 1

    // 2. Transmit on DEFAULT pins (PA0)
    bus.write_data(0x0802, 0x00); // TX 0x00 (all bits low after start bit)
    bus.tick_peripherals(2000); // Start bit should be active (~1667 cycles)
    
    // Check PA0 (0x404) - should be LOW (start bit)
    CHECK(machine.pin_mux().get_state_by_address(0x404, 0).drive_level == false);
    CHECK(machine.pin_mux().get_state_by_address(0x404, 0).owner == PinOwner::uart);

    // Check PA4 (0x404, bit 4) - should be HIGH (idle/GPIO)
    CHECK(machine.pin_mux().get_state_by_address(0x404, 4).owner == PinOwner::gpio);

    // 3. Wait for completion
    bus.tick_peripherals(20000);
    CHECK((bus.read_data(0x0804) & 0x40) != 0); // TXCIF

    // 4. Change Routing to ALT1 (PA4)
    // USARTROUTEA (0x05E2) = 0x01 (USART0 ALT1)
    bus.write_data(0x05E2, 0x01);

    // 5. Transmit again
    bus.write_data(0x0802, 0x00);
    bus.tick_peripherals(2000); 

    // Now PA4 should be LOW (start bit)
    CHECK(machine.pin_mux().get_state_by_address(0x404, 4).drive_level == false);
    CHECK(machine.pin_mux().get_state_by_address(0x404, 4).owner == PinOwner::uart);

    // And PA0 should be released back to GPIO
    CHECK(machine.pin_mux().get_state_by_address(0x404, 0).owner == PinOwner::gpio);
}
