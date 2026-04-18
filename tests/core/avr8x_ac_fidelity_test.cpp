#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include <iostream>

using namespace vioavr::core;

TEST_CASE("AVR8X Analog Comparator Fidelity Test") {
    // 1. Setup ATmega4809
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    
    Machine machine(*device);
    auto& bus = machine.bus();

    const u16 AC_BASE = 0x0680;
    const u16 AC_CTRLA = AC_BASE + 0x00;
    const u16 AC_INTCTRL = AC_BASE + 0x06;
    const u16 AC_STATUS = AC_BASE + 0x07;

    // 2. Enable AC
    machine.analog_signal_bank().set_voltage(0, 0.6); // P-mux 0
    machine.analog_signal_bank().set_voltage(4, 0.4); // N-mux 0 (maps to bank 4 in my dummy logic)
    bus.write_data(AC_CTRLA, 0x01); // ENABLE=1
    CHECK((bus.read_data(AC_CTRLA) & 0x01) == 0x01);

    // 3. Check State
    bus.tick_peripherals(1);
    u8 s1 = bus.read_data(AC_STATUS);
    CHECK((s1 & 0x10) != 0); // STATE should be high (0.6 > 0.4)
    
    // Change voltages to trigger event
    machine.analog_signal_bank().set_voltage(0, 0.3); // Now 0.3 < 0.4
    bus.tick_peripherals(1);
    u8 s2 = bus.read_data(AC_STATUS);
    CHECK((s2 & 0x10) == 0); // STATE should be low

    // 4. Test Interrupt Flag
    // By default CTRLA has INTMODE=0 (BOTHEDGE)
    // If state changed from s1 to s2, CMP flag should be set
    CHECK((bus.read_data(AC_STATUS) & 0x01) == 0x01);

    // 5. Clear flag
    bus.write_data(AC_STATUS, 0x01);
    CHECK((bus.read_data(AC_STATUS) & 0x01) == 0);

    // 6. Test NEGEDGE Mode
    bus.write_data(AC_CTRLA, 0x21); // INTMODE=2 (NEGEDGE), ENABLE=1
    machine.analog_signal_bank().set_voltage(0, 0.6); // Start high
    bus.tick_peripherals(1);

    // Clear flag again just in case
    bus.write_data(AC_STATUS, 0x01);
    
    // Trigger Neg edge
    machine.analog_signal_bank().set_voltage(0, 0.3); // High to Low
    bus.tick_peripherals(1);
    CHECK((bus.read_data(AC_STATUS) & 0x01) == 0x01); // Flag set on Neg edge
    
    // Clear flag
    bus.write_data(AC_STATUS, 0x01);

    // Trigger Pos edge (should NOT set flag in NEGEDGE mode)
    machine.analog_signal_bank().set_voltage(0, 0.6); // Low to High
    bus.tick_peripherals(1);
    CHECK((bus.read_data(AC_STATUS) & 0x01) == 0); // Flag NOT set on Pos edge

}
