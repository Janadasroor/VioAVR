#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/wdt8x.hpp"
#include "vioavr/core/rstctrl.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR8X WDT - Timeout via machine.run") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    auto& cpu = machine.cpu();

    const u16 CPU_CCP = 0x34;
    const u16 WDT_CTRLA = 0x0100;

    std::vector<u16> program = {0xCFFF}; // RJMP -1 infinite loop
    bus.load_flash(program);
    cpu.reset();

    auto rsts = machine.peripherals_of_type<RstCtrl>();
    REQUIRE(!rsts.empty());
    RstCtrl* rst = rsts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(WDT_CTRLA, 0x01);

    machine.run(200000);

    u8 rstfr = rst->reset_flags();
    CHECK_MESSAGE((rstfr & 0x08) != 0, "WDRF should be set after WDT timeout");
}

TEST_CASE("AVR8X WDT - Early Warning Flag") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    auto& cpu = machine.cpu();

    const u16 CPU_CCP = 0x34;
    const u16 WDT_CTRLA = 0x0100;
    const u16 WDT_STATUS = 0x0101;

    std::vector<u16> program = {0xCFFF};
    bus.load_flash(program);
    cpu.reset();

    auto wdts = machine.peripherals_of_type<Wdt8x>();
    REQUIRE(!wdts.empty());
    Wdt8x* wdt = wdts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(WDT_CTRLA, 0x01);

    // Tick just past 50% of timeout to set EWIF
    wdt->tick(64001);

    u8 status = wdt->read(WDT_STATUS);
    CHECK_MESSAGE((status & 0x01) != 0, "EWIF should be set at 50% timeout");
}

TEST_CASE("AVR8X WDT - WDR Resets Timer") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    auto& cpu = machine.cpu();

    const u16 CPU_CCP = 0x34;
    const u16 WDT_CTRLA = 0x0100;

    std::vector<u16> program = {0xCFFF};
    bus.load_flash(program);
    cpu.reset();

    auto wdts = machine.peripherals_of_type<Wdt8x>();
    REQUIRE(!wdts.empty());
    Wdt8x* wdt = wdts[0];

    auto rsts = machine.peripherals_of_type<RstCtrl>();
    REQUIRE(!rsts.empty());
    RstCtrl* rst = rsts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(WDT_CTRLA, 0x01);

    // Tick nearly to timeout
    wdt->tick(127999);

    // WDR resets the timer
    wdt->reset_timer();

    // Tick less than timeout — WDR should have prevented the timeout
    wdt->tick(127999);

    u8 rstfr = rst->reset_flags();
    CHECK_MESSAGE((rstfr & 0x08) == 0, "WDR should prevent WDT timeout");
}

TEST_CASE("AVR8X WDT - Timeout After Full Period Post-WDR") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    auto& cpu = machine.cpu();

    const u16 CPU_CCP = 0x34;
    const u16 WDT_CTRLA = 0x0100;

    std::vector<u16> program = {0xCFFF};
    bus.load_flash(program);
    cpu.reset();

    auto wdts = machine.peripherals_of_type<Wdt8x>();
    REQUIRE(!wdts.empty());
    Wdt8x* wdt = wdts[0];

    auto rsts = machine.peripherals_of_type<RstCtrl>();
    REQUIRE(!rsts.empty());
    RstCtrl* rst = rsts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(WDT_CTRLA, 0x01);

    // WDR immediately
    wdt->reset_timer();

    // Tick the full timeout period
    wdt->tick(128001);

    u8 rstfr = rst->reset_flags();
    CHECK_MESSAGE((rstfr & 0x08) != 0, "WDT should fire after WDR + full timeout period");
}
