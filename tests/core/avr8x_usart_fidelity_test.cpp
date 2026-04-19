#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/uart8x.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X USART Fidelity Test") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    REQUIRE(device->uart8x_count >= 1);

    Machine machine(*device);
    auto& bus = machine.bus();

    const u16 USART0_BASE = 0x0800;
    const u16 USART0_RXDATAL = USART0_BASE + 0x00;
    const u16 USART0_TXDATAL = USART0_BASE + 0x02;
    const u16 USART0_STATUS  = USART0_BASE + 0x04;
    const u16 USART0_CTRLA   = USART0_BASE + 0x05;
    const u16 USART0_CTRLB   = USART0_BASE + 0x06;
    const u16 USART0_BAUD    = USART0_BASE + 0x08;

    // 1. Check initially empty/ready
    // STATUS should have DREIF (0x20) set, but TXCIF (0x40) is NOT set until first TX done.
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x20);

    // 2. Transmit a byte
    bus.write_data(USART0_CTRLB, 0x40); // TXEN=1
    bus.write_data(USART0_TXDATAL, 'A');

    // DREIF and TXCIF should both be 0 if TX starts
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x00);

    // 3. Tick machine
    // Default duration is 160 cycles
    bus.tick_peripherals(100);
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x00); // Still in progress

    bus.tick_peripherals(100);
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x60); // Done! Both DREIF and TXCIF set.

    // 4. Test Baud Rate Timing
    // BAUD = 1389 -> Duration = (1389 * 16 * 10) / 64 = 1389 * 2.5 = 3472.5 approx 3472 cycles
    bus.write_data(USART0_BAUD, 0x6D); // 1389 lower
    bus.write_data(USART0_BAUD + 1, 0x05); // 1389 upper
    
    bus.write_data(USART0_TXDATAL, 'B');
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x00);

    bus.tick_peripherals(3000);
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x00);

    bus.tick_peripherals(500); // Total 3500 > 3472
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x60);

    // 6. FIFO Test
    auto* uart = dynamic_cast<Uart8x*>(bus.get_peripheral_by_name("USART8X"));
    REQUIRE(uart != nullptr);

    uart->inject_received_byte('X');
    CHECK((bus.read_data(USART0_STATUS) & 0x80) != 0); // RXCIF set
    
    uart->inject_received_byte('Y');
    CHECK((bus.read_data(USART0_STATUS) & 0x80) != 0); // RXCIF still set
    
    // Read first byte
    CHECK(bus.read_data(USART0_RXDATAL) == 'X');
    CHECK((bus.read_data(USART0_STATUS) & 0x80) != 0); // RXCIF still set (one byte remains)
    
    // Read second byte
    CHECK(bus.read_data(USART0_RXDATAL) == 'Y');
    CHECK((bus.read_data(USART0_STATUS) & 0x80) == 0x00); // RXCIF now clear
    
    // 7. Overflow Test
    uart->inject_received_byte('1');
    uart->inject_received_byte('2');
    uart->inject_received_byte('3'); // Should overflow
    
    // Check first byte
    CHECK(bus.read_data(USART0_RXDATAL) == '1');
    // Check second byte + error flag in RXDATAH (offset +1)
    CHECK((bus.read_data(USART0_RXDATAL + 1) & 0x40) != 0); // BUFOVF bit
    CHECK(bus.read_data(USART0_RXDATAL) == '2');
}
