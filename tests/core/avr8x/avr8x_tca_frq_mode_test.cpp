#include "doctest.h"
#include "vioavr/core/tca.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X TCA - FRQ Mode (WGMODE=1) Toggle Output") {
    Tca tca("TCA0", devices::atmega4809.timers_tca[0]);
    tca.reset();

    // Enable timer, FRQ mode (WGMODE=1)
    tca.write(devices::atmega4809.timers_tca[0].ctrla_address, 0x01);
    tca.write(devices::atmega4809.timers_tca[0].ctrlb_address, 0x01);

    // CMP0 = 5, PER = 10
    tca.write(devices::atmega4809.timers_tca[0].cmp0_address, 5);
    tca.write(devices::atmega4809.timers_tca[0].cmp0_address + 1, 0);
    tca.write(devices::atmega4809.timers_tca[0].period_address, 10);
    tca.write(devices::atmega4809.timers_tca[0].period_address + 1, 0);

    // get_wo_level(0) should be wo_states_[0] which starts false
    CHECK(tca.get_wo_level(0) == false);

    // Tick 5 times: CNT goes 0→1→2→3→4→5
    // At CNT=5, CMP match: wo toggles to true, CNT resets to 0
    tca.tick(6);

    // wo should now be true
    CHECK(tca.get_wo_level(0) == true);

    // Tick to PER match (CNT 0→1→...→10), wo toggles at CMP=5
    // Tick 5: CNT=5, wo toggles false, CNT resets
    // 6 more ticks to reach PER
    tca.tick(11);

    // wo should have toggled again at CMP match → false
    CHECK(tca.get_wo_level(0) == false);

    // CMP0 match interrupt should have fired
    CHECK((tca.read(devices::atmega4809.timers_tca[0].intflags_address) & 0x10) != 0);
}

TEST_CASE("AVR8X TCA - Dual-Slope Counting Pattern") {
    Tca tca("TCA0", devices::atmega4809.timers_tca[0]);
    tca.reset();

    tca.write(devices::atmega4809.timers_tca[0].ctrla_address, 0x01);
    tca.write(devices::atmega4809.timers_tca[0].ctrlb_address, 0x05); // WGMODE=5

    tca.write(devices::atmega4809.timers_tca[0].period_address, 10);
    tca.write(devices::atmega4809.timers_tca[0].period_address + 1, 0);
    tca.write(devices::atmega4809.timers_tca[0].cmp0_address, 5);
    tca.write(devices::atmega4809.timers_tca[0].cmp0_address + 1, 0);

    // Dual-slope: count up to PER, then down to 0
    // CNT: 0→1→2→...→10→9→...→0
    // Period = (2 * PER) cycles = 20 cycles

    // After 11 ticks: CNT goes 0,1,2,3,4,5,6,7,8,9,10 (at PER)
    tca.tick(11);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 9); // counting down now

    // After another 10 ticks: 9,8,7,6,5,4,3,2,1,0
    tca.tick(10);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 1); // counting up again (wrapped at 0)

    // OVF should have fired at bottom (BOTTOM=0)
    CHECK((tca.read(devices::atmega4809.timers_tca[0].intflags_address) & 0x01) != 0);
}

TEST_CASE("AVR8X TCA - Software CMD (UPDATE via CTRLESET)") {
    Tca tca("TCA0", devices::atmega4809.timers_tca[0]);
    tca.reset();

    // Enable timer, NORMAL mode
    tca.write(devices::atmega4809.timers_tca[0].ctrla_address, 0x01);

    // PER = 100
    tca.write(devices::atmega4809.timers_tca[0].period_address, 100);
    tca.write(devices::atmega4809.timers_tca[0].period_address + 1, 0);

    // PERBUF = 200
    tca.write(devices::atmega4809.timers_tca[0].perbuf_address, 200);
    tca.write(devices::atmega4809.timers_tca[0].perbuf_address + 1, 0);

    CHECK(tca.read(devices::atmega4809.timers_tca[0].period_address) == 100);

    // Software UPDATE via CTRLESET: CMD=0x01
    tca.write(devices::atmega4809.timers_tca[0].ctrleclr_address + 1, 0x01);

    CHECK(tca.read(devices::atmega4809.timers_tca[0].period_address) == 200);
}

TEST_CASE("AVR8X TCA - Prescaler DIV16") {
    Tca tca("TCA0", devices::atmega4809.timers_tca[0]);
    tca.reset();

    // Enable timer with prescaler DIV16 (CLKSEL=5 at bits 3:1)
    // CLKSEL values: 0=DIV1, 1=DIV2, 2=DIV4, 3=DIV8, 4=DIV16, 5=DIV64, 6=DIV256, 7=DIV1024
    // CTRLA = 0x01 (ENABLE) | (4 << 1) = 0x09
    tca.write(devices::atmega4809.timers_tca[0].ctrla_address, 0x09);

    // PER = 10
    tca.write(devices::atmega4809.timers_tca[0].period_address, 10);
    tca.write(devices::atmega4809.timers_tca[0].period_address + 1, 0);

    // With DIV16, 16 ticks advance CNT by 1
    tca.tick(16);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 1);

    // 16 more ticks
    tca.tick(16);
    CHECK(tca.read(devices::atmega4809.timers_tca[0].tcnt_address) == 2);
}
