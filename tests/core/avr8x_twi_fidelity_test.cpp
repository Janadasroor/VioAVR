#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/twi8x.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR8X TWI8X - Bit Timing Fidelity") {
    // 4809 TWI0 is at 0x480
    Twi8xDescriptor desc = {
        .mctrla_address = 0x480,
        .mctrlb_address = 0x481,
        .mstatus_address = 0x482,
        .mbaud_address = 0x483,
        .maddr_address = 0x484,
        .mdata_address = 0x485
    };
    
    // We need a dummy CPU for the TWI constructor if it takes one?
    // Actually Twi8x doesn't take CPU in constructor
    Twi8x twi{desc};
    twi.reset();

    // 1. Enable Master
    twi.write(0x480, 0x01U); // ENABLE=1
    
    // 2. Set BAUD (e.g. 10 -> approx 100kbps @ 16MHz)
    twi.write(0x483, 10);
    
    // 3. Set ADDR to start transfer
    twi.write(0x484, 0xA0U); // Write to address 0x50
    
    // TWI state should move to BUSY
    // 9 bits * (baud_cycles)
    // Approx (10*2 + 10) cycles per bit? Depends on implementation.
    // In our high-fidelity Twi8x, it's roughly (BAUD + 1) per half-phase.
    
    // Let's tick 50 cycles. Flag should NOT be set.
    twi.tick(50);
    CHECK((twi.read(0x482) & 0x40U) == 0U); // WIF (bit 6) should be 0
    
    // Tick 1000 cycles. Flag should be set.
    twi.tick(1000);
    CHECK((twi.read(0x482) & 0x40U) != 0U); // WIF should be 1
}
