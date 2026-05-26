#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/wdt8x.hpp"
#include "vioavr/core/rstctrl.hpp"
#include "vioavr/core/cpu_control.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

// ATmega4809 WDT: CTRLA=0x100, STATUS=0x101 (no WINCTRLA, no INTCTRL)
static constexpr u16 CTRLA   = 0x0100;
static constexpr u16 STATUS  = 0x0101;
static constexpr u16 CPU_CCP = 0x34;

TEST_CASE("WDT8X — Reset defaults") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    CHECK(bus.read_data(CTRLA) == 0x00);
    CHECK(bus.read_data(STATUS) == 0x00);
}

TEST_CASE("WDT8X — CTRLA round-trip via CCP") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    std::vector<u16> prog = {0xCFFF};
    bus.load_flash(prog);
    machine.cpu().reset();

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(CTRLA, 0x05);
    CHECK(bus.read_data(CTRLA) == 0x05);
}

TEST_CASE("WDT8X — CTRLA write without CCP fails") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0x01);
    CHECK(bus.read_data(CTRLA) == 0x00);
}

TEST_CASE("WDT8X — STATUS EWIF write-1-to-clear") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    auto wdts = machine.peripherals_of_type<Wdt8x>();
    Wdt8x* wdt = wdts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(CTRLA, 0x01);

    wdt->tick(64001);
    CHECK((bus.read_data(STATUS) & 0x01) != 0);

    bus.write_data(STATUS, 0x01);
    CHECK((bus.read_data(STATUS) & 0x01) == 0);
}

TEST_CASE("WDT8X — WDR when disabled starts WDT") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    auto wdts = machine.peripherals_of_type<Wdt8x>();
    Wdt8x* wdt = wdts[0];
    auto rsts = machine.peripherals_of_type<RstCtrl>();
    RstCtrl* rst = rsts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(CTRLA, 0x01);
    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(CTRLA, 0x00);

    wdt->reset_timer();
    wdt->tick(128001);
    CHECK((rst->reset_flags() & 0x08) != 0);
}

TEST_CASE("WDT8X — Period 2 (16ms) timing") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    auto wdts = machine.peripherals_of_type<Wdt8x>();
    Wdt8x* wdt = wdts[0];
    auto rsts = machine.peripherals_of_type<RstCtrl>();
    RstCtrl* rst = rsts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(CTRLA, 0x02);

    wdt->tick(255999);
    CHECK((rst->reset_flags() & 0x08) == 0);

    wdt->tick(1);
    CHECK((rst->reset_flags() & 0x08) != 0);
}

TEST_CASE("WDT8X — Multiple WDR resets prevent timeout") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    auto wdts = machine.peripherals_of_type<Wdt8x>();
    Wdt8x* wdt = wdts[0];
    auto rsts = machine.peripherals_of_type<RstCtrl>();
    RstCtrl* rst = rsts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(CTRLA, 0x01);

    for (int i = 0; i < 5; i++) {
        wdt->tick(100000);
        wdt->reset_timer();
    }
    CHECK((rst->reset_flags() & 0x08) == 0);
}

TEST_CASE("WDT8X — EWIF early warning at 50%") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    auto wdts = machine.peripherals_of_type<Wdt8x>();
    Wdt8x* wdt = wdts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(CTRLA, 0x01);

    wdt->tick(63999);
    CHECK((bus.read_data(STATUS) & 0x01) == 0);

    wdt->tick(1);
    CHECK((bus.read_data(STATUS) & 0x01) != 0);
}

TEST_CASE("WDT8X — Unmapped addresses return 0") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    CHECK(bus.read_data(0x0102) == 0);
    CHECK(bus.read_data(0x0103) == 0);
    CHECK(bus.read_data(0x0104) == 0);
}

TEST_CASE("WDT8X — Disable via period=0") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    auto wdts = machine.peripherals_of_type<Wdt8x>();
    Wdt8x* wdt = wdts[0];
    auto rsts = machine.peripherals_of_type<RstCtrl>();
    RstCtrl* rst = rsts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(CTRLA, 0x01);
    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(CTRLA, 0x00);

    wdt->tick(200000);
    CHECK((rst->reset_flags() & 0x08) == 0);
}

TEST_CASE("WDT8X — STATUS unchanged by writes to other bits") {
    Machine machine{atmega4809};
    auto& bus = machine.bus();
    machine.reset();

    auto wdts = machine.peripherals_of_type<Wdt8x>();
    Wdt8x* wdt = wdts[0];

    bus.write_data(CPU_CCP, 0xD8);
    bus.write_data(CTRLA, 0x01);

    wdt->tick(64001);
    CHECK((bus.read_data(STATUS) & 0x01) != 0);

    bus.write_data(STATUS, 0x02);
    CHECK((bus.read_data(STATUS) & 0x01) != 0);
}
