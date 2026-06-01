#include "doctest.h"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/xmegaac.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

using namespace vioavr::core;

TEST_CASE("XMEGA AC — Register access") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    REQUIRE(device->ac_xmega_count >= 1);

    XmegaAc ac(device->acs_xmega[0]);
    const auto& d = device->acs_xmega[0];

    CHECK(ac.read(d.ac0ctrl_address) == 0);
    CHECK(ac.read(d.ac1ctrl_address) == 0);
    CHECK(ac.read(d.ac0mux_address) == 0);
    CHECK(ac.read(d.ac1mux_address) == 0);
    CHECK(ac.read(d.ctrla_address) == 0);
    CHECK(ac.read(d.winctrl_address) == 0);
    CHECK(ac.read(d.status_address) == 0);

    ac.write(d.ac0ctrl_address, 0xAFU);
    CHECK(ac.read(d.ac0ctrl_address) == 0xAFU);

    ac.write(d.ac1ctrl_address, 0x5AU);
    CHECK(ac.read(d.ac1ctrl_address) == 0x5AU);

    ac.write(d.ac0mux_address, 0x29U);
    CHECK(ac.read(d.ac0mux_address) == 0x29U);

    ac.write(d.ac1mux_address, 0x31U);
    CHECK(ac.read(d.ac1mux_address) == 0x31U);

    ac.write(d.ctrla_address, 0x03U);
    CHECK(ac.read(d.ctrla_address) == 0x03U);

    ac.write(d.winctrl_address, 0x33U);
    CHECK(ac.read(d.winctrl_address) == 0x33U);
}

TEST_CASE("XMEGA AC — AC0 evaluates input voltages") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAc ac(device->acs_xmega[0]);
    const auto& d = device->acs_xmega[0];

    AnalogSignalBank bank;
    ac.set_analog_signal_bank(&bank);

    // Enable AC0
    ac.write(d.ctrla_address, 0x01U);
    ac.write(d.ac0ctrl_address, 0x01U); // ENABLE

    // MUXPOS=0, MUXNEG=0 (both map to analog channels 0 and 8+0=8)
    ac.write(d.ac0mux_address, 0x00U);

    // Vp = 2.0V, Vn = 0V → output HIGH
    bank.set_voltage(0, 2.0);
    bank.set_voltage(8, 0.0);
    ac.tick(1);
    CHECK((ac.read(d.status_address) & 0x01U) != 0);

    // Vp = 0V, Vn = 2.0V → output LOW
    bank.set_voltage(0, 0.0);
    bank.set_voltage(8, 2.0);
    ac.tick(1);
    CHECK((ac.read(d.status_address) & 0x01U) == 0);
}

TEST_CASE("XMEGA AC — AC1 evaluates independently") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAc ac(device->acs_xmega[0]);
    const auto& d = device->acs_xmega[0];

    AnalogSignalBank bank;
    ac.set_analog_signal_bank(&bank);

    ac.write(d.ctrla_address, 0x01U);
    ac.write(d.ac1ctrl_address, 0x01U); // AC1 ENABLE

    // MUXPOS=1, MUXNEG=2
    ac.write(d.ac1mux_address, (1U << 3) | 2U);

    bank.set_voltage(1, 1.5);
    bank.set_voltage(10, 0.5);
    ac.tick(1);
    CHECK((ac.read(d.status_address) & 0x02U) != 0);

    bank.set_voltage(1, 0.5);
    bank.set_voltage(10, 1.5);
    ac.tick(1);
    CHECK((ac.read(d.status_address) & 0x02U) == 0);
}

TEST_CASE("XMEGA AC — Interrupt on rising edge") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAc ac(device->acs_xmega[0]);
    const auto& d = device->acs_xmega[0];

    AnalogSignalBank bank;
    ac.set_analog_signal_bank(&bank);

    ac.write(d.ctrla_address, 0x01U);
    ac.write(d.ac0ctrl_address, 0x01U | (0x01U << 3)); // ENABLE + INTSEL=1
    // INTMODE=0 → toggle

    // Initially Vp < Vn → output LOW
    bank.set_voltage(0, 0.0);
    bank.set_voltage(8, 2.0);
    ac.tick(1);

    InterruptRequest req;
    CHECK_FALSE(ac.pending_interrupt_request(req));

    // Vp rises above Vn → rising edge
    bank.set_voltage(0, 3.0);
    ac.tick(1);

    CHECK(ac.pending_interrupt_request(req));
}

TEST_CASE("XMEGA AC — Interrupt consume and clear") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAc ac(device->acs_xmega[0]);
    const auto& d = device->acs_xmega[0];

    AnalogSignalBank bank;
    ac.set_analog_signal_bank(&bank);

    ac.write(d.ctrla_address, 0x01U);
    ac.write(d.ac0ctrl_address, 0x01U | (0x01U << 3));

    bank.set_voltage(0, 0.0);
    bank.set_voltage(8, 2.0);
    ac.tick(1);

    bank.set_voltage(0, 3.0);
    ac.tick(1);

    InterruptRequest req;
    CHECK(ac.pending_interrupt_request(req));
    CHECK(req.vector_index == d.vector_index);

    CHECK(ac.consume_interrupt_request(req));
    CHECK_FALSE(ac.pending_interrupt_request(req));
}

TEST_CASE("XMEGA AC — Window mode") {
    auto device = DeviceCatalog::find("ATxmega128A1");
    REQUIRE(device != nullptr);
    XmegaAc ac(device->acs_xmega[0]);
    const auto& d = device->acs_xmega[0];

    AnalogSignalBank bank;
    ac.set_analog_signal_bank(&bank);

    ac.write(d.ctrla_address, 0x01U);
    ac.write(d.ac0ctrl_address, 0x01U);
    ac.write(d.ac1ctrl_address, 0x01U);

    // AC0: positive=ch0, negative=ch8 → measures against low threshold
    // AC1: positive=ch1, negative=ch9 → measures against high threshold
    ac.write(d.ac0mux_address, 0x00U);
    ac.write(d.ac1mux_address, (1U << 3) | 1U); // MUXPOS=1, MUXNEG=1

    // Set thresholds: low=3V, high=5V
    bank.set_voltage(8, 3.0);
    bank.set_voltage(9, 5.0);

    // Signal = 2V → below window (both comparators: 0)
    bank.set_voltage(0, 2.0);
    bank.set_voltage(1, 2.0);
    ac.write(d.winctrl_address, 0x01U); // WINEN
    ac.tick(1);
    // WSTATE=00, WSTATE1(below)=1, WSTATE0(above)=0
    CHECK((ac.read(d.status_address) & 0x30U) == 0x20U);

    // Signal = 4V → inside window (AC0=1, AC1=0)
    bank.set_voltage(0, 4.0);
    bank.set_voltage(1, 4.0);
    ac.tick(1);
    CHECK((ac.read(d.status_address) & 0x30U) == 0x00U);

    // Signal = 6V → above window (both: 1)
    bank.set_voltage(0, 6.0);
    bank.set_voltage(1, 6.0);
    ac.tick(1);
    CHECK((ac.read(d.status_address) & 0x30U) == 0x10U);
}
