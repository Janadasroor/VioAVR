#include "doctest.h"
#include "vioavr/core/dac8x.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

using namespace vioavr::core;

static Dac8xDescriptor make_desc() noexcept {
    Dac8xDescriptor d{};
    d.ctrla_address = 0x20;
    d.data_address = 0x21;
    return d;
}

TEST_CASE("DAC8x: Register access") {
    auto d = make_desc();
    Dac8x dac(d, 0);
    dac.reset();

    CHECK(dac.read(d.ctrla_address) == 0);
    dac.write(d.ctrla_address, 0x03U);
    CHECK(dac.read(d.ctrla_address) == 0x03U);

    dac.write(d.data_address, 0xAAU);
    CHECK(dac.read(d.data_address) == 0xAAU);
}

TEST_CASE("DAC8x: Reset clears registers") {
    auto d = make_desc();
    Dac8x dac(d, 0);
    dac.write(d.ctrla_address, 0x01U);
    dac.write(d.data_address, 0xFFU);
    dac.reset();
    CHECK(dac.read(d.ctrla_address) == 0);
    CHECK(dac.read(d.data_address) == 0);
}

TEST_CASE("DAC8x: Conversion timing") {
    auto d = make_desc();
    Dac8x dac(d, 0);
    dac.reset();
    dac.write(d.ctrla_address, 0x01U);

    dac.write(d.data_address, 0x80U);
    CHECK(dac.read(d.data_address) == 0x80U);

    dac.tick(0);
    dac.tick(1);
}

TEST_CASE("DAC8x: Output voltage calculation") {
    auto d = make_desc();
    Dac8x dac(d, 0);
    dac.reset();

    dac.set_vref(5.0);
    dac.write(d.ctrla_address, 0x01U);
    dac.write(d.data_address, 0x80U);
    dac.tick(1);
    double expected = 128.0 / 255.0 * 5.0;
    CHECK(dac.output_voltage() == doctest::Approx(expected));

    dac.write(d.data_address, 0x00U);
    dac.tick(1);
    CHECK(dac.output_voltage() == doctest::Approx(0.0));

    dac.write(d.data_address, 0xFFU);
    dac.tick(1);
    CHECK(dac.output_voltage() == doctest::Approx(5.0));
}

TEST_CASE("DAC8x: Disabled when EN=0") {
    auto d = make_desc();
    AnalogSignalBank bank;
    Dac8x dac(d, 0);
    dac.set_analog_signal_bank(&bank);
    dac.set_vref(5.0);
    dac.reset();

    dac.write(d.data_address, 0x80U);
    dac.tick(10);
    CHECK(bank.voltage(64) == doctest::Approx(0.0));

    dac.write(d.ctrla_address, 0x01U);
    dac.write(d.data_address, 0x80U);
    dac.tick(1);
    CHECK(bank.voltage(64) > 0.0);
}

TEST_CASE("DAC8x: Analog signal bank output") {
    auto d = make_desc();
    AnalogSignalBank bank;
    Dac8x dac(d, 0);
    dac.set_analog_signal_bank(&bank);
    dac.set_vref(3.3);
    dac.reset();

    dac.write(d.ctrla_address, 0x01U);
    dac.write(d.data_address, 0x80U);
    dac.tick(1);

    double expected = 128.0 / 255.0 * 3.3;
    CHECK(bank.voltage(64) == doctest::Approx(expected));
}

TEST_CASE("DAC8x: Mapped ranges") {
    auto d = make_desc();
    Dac8x dac(d, 0);
    auto r = dac.mapped_ranges();
    REQUIRE(r.size() == 1);
    CHECK(r[0].begin == d.ctrla_address);
    CHECK(r[0].end == d.data_address);
}
