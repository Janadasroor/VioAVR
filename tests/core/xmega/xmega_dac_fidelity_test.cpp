#include "doctest.h"
#include "vioavr/core/xmega_dac.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

using namespace vioavr::core;

static XmegaDacDescriptor make_test_desc() noexcept {
    XmegaDacDescriptor d{};
    d.ctrla_address = 0x10;
    d.ctrlb_address = 0x11;
    d.ctrlc_address = 0x12;
    d.evctrl_address = 0x13;
    d.status_address = 0x14;
    d.ch0gaincal_address = 0x15;
    d.ch0offsetcal_address = 0x16;
    d.ch1gaincal_address = 0x17;
    d.ch1offsetcal_address = 0x18;
    d.ch0data_address = 0x20;
    d.ch1data_address = 0x30;
    d.vector_index = 10;
    return d;
}

TEST_CASE("XMEGA DAC — Register access") {
    auto d = make_test_desc();
    XmegaDac dac("XDAC0", d);
    dac.reset();

    CHECK(dac.read(d.ctrla_address) == 0);
    dac.write(d.ctrla_address, 0x03U);
    CHECK(dac.read(d.ctrla_address) == 0x03U);

    dac.write(d.ctrlb_address, 0x04U);
    CHECK(dac.read(d.ctrlb_address) == 0x04U);

    dac.write(d.ctrlc_address, 0x01U);
    CHECK(dac.read(d.ctrlc_address) == 0x01U);

    dac.write(d.evctrl_address, 0x03U);
    CHECK(dac.read(d.evctrl_address) == 0x03U);

    CHECK(dac.read(d.ch0gaincal_address) == 0);
    dac.write(d.ch0gaincal_address, 0xAAU);
    CHECK(dac.read(d.ch0gaincal_address) == 0xAAU);
}

TEST_CASE("XMEGA DAC — 16-bit data register") {
    auto d = make_test_desc();
    XmegaDac dac("XDAC0", d);
    dac.reset();

    dac.write(d.ch0data_address, 0x34U);
    CHECK(dac.read(d.ch0data_address) == 0x34U);
    CHECK(dac.read(d.ch0data_address + 1) == 0x00U);

    dac.write(d.ch0data_address + 1, 0x12U);
    CHECK(dac.read(d.ch0data_address) == 0x34U);
    CHECK(dac.read(d.ch0data_address + 1) == 0x12U);

    u16 raw = static_cast<u16>(dac.read(d.ch0data_address)) |
              (static_cast<u16>(dac.read(d.ch0data_address + 1)) << 8);
    CHECK(raw == 0x1234U);

    // Channel 1
    dac.write(d.ch1data_address, 0xCDU);
    dac.write(d.ch1data_address + 1, 0xABU);
    u16 raw1 = static_cast<u16>(dac.read(d.ch1data_address)) |
               (static_cast<u16>(dac.read(d.ch1data_address + 1)) << 8);
    CHECK(raw1 == 0xABCDU);
}

TEST_CASE("XMEGA DAC — Conversion triggers analog output") {
    auto d = make_test_desc();
    AnalogSignalBank bank;
    XmegaDac dac("XDAC0", d);
    dac.set_analog_signal_bank(&bank);
    dac.reset();

    dac.write(d.ctrla_address, 0x01U);
    dac.write(d.ch0data_address, 0x00U);
    dac.write(d.ch0data_address + 1, 0x08U);

    CHECK(bank.voltage(0) == 0.0);
    dac.tick(1);
    CHECK(bank.voltage(0) == 0.0);
    dac.tick(1);
    CHECK(bank.voltage(0) == doctest::Approx(2.5));
}

TEST_CASE("XMEGA DAC — Conversion requires channel enable") {
    auto d = make_test_desc();
    AnalogSignalBank bank;
    XmegaDac dac("XDAC0", d);
    dac.set_analog_signal_bank(&bank);
    dac.reset();

    dac.write(d.ch0data_address, 0x00U);
    dac.write(d.ch0data_address + 1, 0x08U);
    dac.tick(2);
    CHECK(bank.voltage(0) == 0.0);

    dac.write(d.ctrla_address, 0x01U);
    dac.write(d.ch0data_address, 0x00U);
    dac.write(d.ch0data_address + 1, 0x08U);
    dac.tick(2);
    CHECK(bank.voltage(0) == doctest::Approx(2.5));
}

TEST_CASE("XMEGA DAC — Voltage calculation different VREF") {
    auto d = make_test_desc();
    AnalogSignalBank bank;
    XmegaDac dac("XDAC0", d);
    dac.set_analog_signal_bank(&bank);
    dac.set_vref(3.3);
    dac.reset();

    dac.write(d.ctrla_address, 0x01U);
    dac.write(d.ch0data_address, 0xFFU);
    dac.write(d.ch0data_address + 1, 0x0FU);
    dac.tick(2);
    CHECK(bank.voltage(0) == doctest::Approx(3.3).epsilon(0.001));

    dac.set_vref(5.0);
    dac.write(d.ctrla_address, 0x01U);
    dac.write(d.ch0data_address, 0xFFU);
    dac.write(d.ch0data_address + 1, 0x0FU);
    dac.tick(2);
    CHECK(bank.voltage(0) == doctest::Approx(5.0).epsilon(0.001));
}

TEST_CASE("XMEGA DAC — STATUS interrupt flags") {
    auto d = make_test_desc();
    XmegaDac dac("XDAC0", d);
    dac.reset();

    dac.write(d.ctrla_address, 0x01U);
    dac.write(d.ch0data_address, 0x00U);
    dac.write(d.ch0data_address + 1, 0x08U);
    dac.tick(2);
    CHECK((dac.read(d.status_address) & 0x01) != 0);

    dac.write(d.status_address, 0x01);
    CHECK((dac.read(d.status_address) & 0x01) == 0);

    dac.write(d.ctrla_address, 0x02U);
    dac.write(d.ch1data_address, 0x00U);
    dac.write(d.ch1data_address + 1, 0x08U);
    dac.tick(2);
    CHECK((dac.read(d.status_address) & 0x02) != 0);

    dac.write(d.status_address, 0x02);
    CHECK((dac.read(d.status_address) & 0x02) == 0);
}

TEST_CASE("XMEGA DAC — LEFTADJ=1 mode") {
    auto d = make_test_desc();
    AnalogSignalBank bank;
    XmegaDac dac("XDAC0", d);
    dac.set_analog_signal_bank(&bank);
    dac.reset();

    dac.write(d.ctrlc_address, 0x01U);
    dac.write(d.ctrla_address, 0x01U);
    dac.write(d.ch0data_address, 0x00U);
    dac.write(d.ch0data_address + 1, 0x80U);
    dac.tick(2);
    CHECK(bank.voltage(0) == doctest::Approx(2.5));
}
