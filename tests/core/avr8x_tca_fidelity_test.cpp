#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/tca.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/memory_bus.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X TCA Fidelity: Split Mode Aliasing") {
    Tca tca(devices::atmega4809.timers_tca[0]);
    tca.reset();

    // Enable Split Mode
    tca.write(devices::atmega4809.timers_tca[0].ctrld_address, 0x01);
    
    // Write to LCNT (via base address)
    tca.write(devices::atmega4809.timers_tca[0].tcnt_address, 0xAA);
    // Write to HCNT (via base address + 1)
    tca.write(devices::atmega4809.timers_tca[0].tcnt_address + 1, 0xBB);

    // Read back in 16-bit fashion (should be mirrored)
    // TCNT is mapped to same addresses
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 0xAA);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address + 1) == 0xBB);

    // Switch back to normal mode
    tca.write(devices::atmega4809.timers_tca[0].ctrld_address, 0x00);
    // Should still be 0xBBAA if we were reading it as a 16-bit word
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 0xAA);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address + 1) == 0xBB);
}

TEST_CASE("AVR8X TCA Fidelity: Normal Mode Double Buffering") {
    Tca tca(devices::atmega4809.timers_tca[0]);
    tca.reset();

    // Enable Timer, Single Slope PWM
    tca.write(devices::atmega4809.timers_tca[0].ctrla_address, 0x01); 
    
    // Set Period to 10 (Counts 0..10, total 11 cycles)
    tca.write(devices::atmega4809.timers_tca[0].period_address, 10);
    tca.write(devices::atmega4809.timers_tca[0].period_address + 1, 0x00);
    
    // Tick to 5
    tca.tick(5);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 5);
    
    // Write PERBUF = 20
    tca.write(devices::atmega4809.timers_tca[0].perbuf_address, 20);
    tca.write(devices::atmega4809.timers_tca[0].perbuf_address + 1, 0x00);
    
    // Active PERIOD should still be 10
    CHECK(tca.read(devices::atmega4809.timers_tca[0].period_address) == 10);
    
    // Tick to reach TCNT=10
    tca.tick(5);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 10);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].period_address) == 10); // Still 10

    // Next tick should trigger UPDATE and reset TCNT to 0
    tca.tick(1);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 0);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].period_address) == 20); // Now 20
}

TEST_CASE("AVR8X TCA Fidelity: Split Mode Independent Counting") {
    Tca tca(devices::atmega4809.timers_tca[0]);
    tca.reset();

    // Enable Timer and Split Mode
    tca.write(devices::atmega4809.timers_tca[0].ctrla_address, 0x01);
    tca.write(devices::atmega4809.timers_tca[0].ctrld_address, 0x01);
    
    // LPER = 10 (11 cycles), HPER = 5 (6 cycles)
    tca.write(devices::atmega4809.timers_tca[0].period_address, 10);
    tca.write(devices::atmega4809.timers_tca[0].period_address + 1, 5);
    
    // Tick 6 times
    tca.tick(6);
    
    // LCNT should be 6
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 6);
    // HCNT should have reached 5, then reset to 0
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address + 1) == 0);
    
    // Tick another 5 times (total 11)
    tca.tick(5);
    // LCNT should have reached 10, then reset to 0
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 0);
}
