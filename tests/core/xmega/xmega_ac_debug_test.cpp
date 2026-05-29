#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/xmegaac.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include <cstdio>

using namespace vioavr::core;

TEST_CASE("Debug XMEGA AC evaluate") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAc ac(device->acs_xmega[0]);
    const auto& d = device->acs_xmega[0];

    printf("ac0ctrl=0x%04X ac1ctrl=0x%04X ac0mux=0x%04X ac1mux=0x%04X\n",
           d.ac0ctrl_address, d.ac1ctrl_address, d.ac0mux_address, d.ac1mux_address);
    printf("ctrla=0x%04X ctrlb=0x%04X winctrl=0x%04X status=0x%04X\n",
           d.ctrla_address, d.ctrlb_address, d.winctrl_address, d.status_address);

    AnalogSignalBank bank;
    ac.set_analog_signal_bank(&bank);

    ac.write(d.ctrla_address, 0x01U);
    ac.write(d.ac0ctrl_address, 0x01U);
    ac.write(d.ac0mux_address, 0x00U);

    bank.set_voltage(0, 2.0);
    bank.set_voltage(8, 0.0);
    ac.tick(1);
    u8 s = ac.read(d.status_address);
    printf("After tick Vp=2 Vn=0: status=0x%02X\n", s);

    bank.set_voltage(0, 0.0);
    bank.set_voltage(8, 2.0);
    ac.tick(1);
    s = ac.read(d.status_address);
    printf("After tick Vp=0 Vn=2: status=0x%02X\n", s);
}
