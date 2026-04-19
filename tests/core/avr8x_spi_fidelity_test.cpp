#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/spi8x.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR8X SPI - Back-to-back Transfer Fidelity") {
    Spi8x spi{atmega4809.spis8x[0]};
    spi.reset();

    const u16 CTRLA = atmega4809.spis8x[0].ctrla_address;
    const u16 INTFLAGS = atmega4809.spis8x[0].intflags_address;
    const u16 DATA = atmega4809.spis8x[0].data_address;

    // 1. Setup: Enable, Master, /2 Prescaler (divisor 2)
    // CTRLA: ENABLE=1, MASTER=1, PRESC=010 (case 4 in switch is /2) 
    // Wait, let's check switch case: switch((ctrla >> 1) & 0x07) { case 4: divisor = 2; }
    // CTRLA = (4 << 1) | 0x05 (ENABLE=1, MASTER=1) ? 
    // Actually CTRLA: [0: ENABLE, 1,2,3: PRESC, 4: MASTER, 5: CLK2X, 6: DORD, 7: BUFEN?]
    // Wait, let's check the code: u8 presc = (ctrla_ >> 1) & 0x07U;
    spi.write(CTRLA, (4 << 1) | 0x01U); // PRESC=4 (/2), ENABLE=1

    // 2. Write Byte 1
    spi.write(DATA, 0xAA);
    // Byte 1 duration should be 2 * 8 = 16 cycles.
    
    // 3. Write Byte 2 immediately (Double buffering)
    spi.write(DATA, 0xBB);
    
    // 4. Tick exactly 16 cycles
    spi.tick(16);
    
    // Byte 1 should be done, Byte 2 should be in progress
    // INTFLAGS_IF is 0x80? No, let's check SPI header.
    // In spi8x.cpp: intflags_ |= INTFLAGS_IF;
    // Assuming INTFLAGS_IF = 0x80.
    CHECK((spi.read(INTFLAGS) & 0x80U) != 0);
    
    // 5. Clear IF flag and check Byte 2 completion
    spi.read(DATA); // Reading DATA clears IF in this implementation
    CHECK((spi.read(INTFLAGS) & 0x80U) == 0);
    
    // 6. Tick another 16 cycles
    spi.tick(16);
    CHECK((spi.read(INTFLAGS) & 0x80U) != 0); // Byte 2 done
}
