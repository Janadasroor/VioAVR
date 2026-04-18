#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"

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
    // STATUS should have DREIF (0x20) and TXCIF (0x40) set after reset in our model
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x60);

    // 2. Transmit a byte
    bus.write_data(USART0_CTRLB, 0x40); // TXEN=1
    bus.write_data(USART0_TXDATAL, 'A');

    // DREIF and TXCIF should clear immediately if TX starts
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x00);

    // 3. Tick machine
    // Default duration is 160 cycles
    bus.tick_peripherals(100);
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x00); // Still in progress

    bus.tick_peripherals(100);
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x60); // Done!

    // 4. Test Baud Rate Timing
    // BAUD = 1389 -> Duration approx (64*16 + 1389)*10 / 64 = 2414 / 64 * 10 approx 377 cycles
    bus.write_data(USART0_BAUD, 0x6D); // 1389 lower
    bus.write_data(USART0_BAUD + 1, 0x05); // 1389 upper
    
    bus.write_data(USART0_TXDATAL, 'B');
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x00);

    bus.tick_peripherals(300);
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x00);

    bus.tick_peripherals(100); // Crosses 377
    CHECK((bus.read_data(USART0_STATUS) & 0x60) == 0x60);

    // 5. Interrupt Test
    bus.write_data(USART0_CTRLA, 0x80); // RXCIE=1
    // Inject byte manually for test (need access to peripheral pointer)
    // For now we check if flags set
}
