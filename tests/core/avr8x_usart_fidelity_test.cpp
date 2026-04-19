#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/uart8x.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR8X USART - Double Buffered TX Fidelity") {
    Uart8x uart{atmega4809.uarts8x[0]};
    uart.reset();

    const u16 STATUS = atmega4809.uarts8x[0].status_address;
    const u16 CTRLB  = atmega4809.uarts8x[0].ctrlb_address;
    const u16 TXDATA = atmega4809.uarts8x[0].txdata_address;
    const u16 BAUD   = atmega4809.uarts8x[0].baud_address;

    // 1. Setup: 9600 baud @ 16MHz -> BAUD = (16e6 * 64) / (16 * 9600) = 6666
    uart.write(CTRLB, 0x40U); // TXEN
    uart.write(BAUD, 0x0AU);   // Low BAUD for fast testing (duration ~ 25 cycles)
    uart.write(BAUD + 1, 0x00U);

    // Initial state: DREIF should be set
    CHECK((uart.read(STATUS) & 0x20U) != 0);

    // 2. Write Byte 1
    uart.write(TXDATA, 'A');
    CHECK((uart.read(STATUS) & 0x20U) == 0); // DREIF cleared immediately

    // 3. Tick to move from Data to Shift
    uart.tick(1);
    CHECK((uart.read(STATUS) & 0x20U) != 0); // DREIF set again (can take byte 2)
    CHECK((uart.read(STATUS) & 0x40U) == 0); // TXCIF cleared

    // 4. Write Byte 2 (Queued)
    uart.write(TXDATA, 'B');
    CHECK((uart.read(STATUS) & 0x20U) == 0); // DREIF cleared

    // 5. Tick for duration of Byte 1 (approx 25 cycles)
    // Formula: (BAUD * 16 * 10 / 64) = (10 * 160 / 64) = 25 cycles
    uart.tick(25);
    
    // Byte 1 finished, Byte 2 moved to Shift
    CHECK((uart.read(STATUS) & 0x20U) != 0); // DREIF set again
    CHECK((uart.read(STATUS) & 0x40U) != 0); // TXCIF set by completion of Byte 1
    
    // Wait... if Byte 2 moved to Shift, TXCIF might be cleared if we wrote that logic.
    // Hardware: TXCIF is set after ALL transmission is done.
}
