#include "doctest.h"
#include "vioavr/core/zcd.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

using namespace vioavr::core;

static ZcdDescriptor make_zcd_desc(u16 ctrla_addr = 0x0100) {
    ZcdDescriptor d{};
    d.ctrla_address = ctrla_addr;
    d.intctrl_address = ctrla_addr + 1;
    d.status_address = ctrla_addr + 2;
    d.vector_index = 42;
    return d;
}

TEST_CASE("ZCD: Register access") {
    Zcd zcd(make_zcd_desc(), 0);
    CHECK(zcd.mapped_ranges().size() == 1);
    CHECK(zcd.read(0x0100) == 0);
    CHECK(zcd.read(0x0101) == 0);
    CHECK(zcd.read(0x0102) == 0);
    zcd.write(0x0100, 0x01);
    CHECK(zcd.read(0x0100) == 0x01);
    zcd.write(0x0101, 0x02);
    CHECK(zcd.read(0x0101) == 0x02);
}

TEST_CASE("ZCD: Reset clears registers") {
    Zcd zcd(make_zcd_desc(), 0);
    zcd.write(0x0100, 0x01);
    zcd.write(0x0101, 0x03);
    zcd.reset();
    CHECK(zcd.read(0x0100) == 0);
    CHECK(zcd.read(0x0101) == 0);
}

TEST_CASE("ZCD: Analog crossing sets interrupt") {
    AnalogSignalBank bank;
    Zcd zcd(make_zcd_desc(), 0);
    zcd.set_analog_signal_bank(&bank);
    zcd.set_vdd(5.0);
    zcd.set_input_channel(0);
    zcd.write(0x0100, 0x01);
    zcd.write(0x0101, 0x03);
    bank.set_voltage(0, 5.0);
    zcd.tick(1);
    CHECK(zcd.read(0x0102) == 0);
    bank.set_voltage(0, 0.0);
    zcd.tick(1);
    CHECK(zcd.read(0x0102) == 0x01);
    InterruptRequest req;
    CHECK(zcd.pending_interrupt_request(req));
    CHECK(req.vector_index == 42);
    CHECK(zcd.consume_interrupt_request(req));
    CHECK(!zcd.pending_interrupt_request(req));
}

TEST_CASE("ZCD: Both edge triggers on any change") {
    AnalogSignalBank bank;
    Zcd zcd(make_zcd_desc(), 0);
    zcd.set_analog_signal_bank(&bank);
    zcd.set_vdd(5.0);
    zcd.set_input_channel(0);
    zcd.write(0x0100, 0x01);
    zcd.write(0x0101, 0x03);
    bank.set_voltage(0, 5.0);
    zcd.tick(1);
    CHECK(zcd.read(0x0102) == 0);
    bank.set_voltage(0, 0.0);
    zcd.tick(1);
    CHECK(zcd.read(0x0102) == 0x01);
    zcd.write(0x0102, 0x01);
    bank.set_voltage(0, 5.0);
    zcd.tick(1);
    CHECK(zcd.read(0x0102) == 0x01);
}

TEST_CASE("ZCD: Status write-one-to-clear") {
    AnalogSignalBank bank;
    Zcd zcd(make_zcd_desc(), 0);
    zcd.set_analog_signal_bank(&bank);
    zcd.set_vdd(5.0);
    zcd.set_input_channel(0);
    zcd.write(0x0100, 0x01);
    zcd.write(0x0101, 0x03);
    bank.set_voltage(0, 5.0);
    zcd.tick(1);
    CHECK(zcd.read(0x0102) == 0);
    bank.set_voltage(0, 0.0);
    zcd.tick(1);
    CHECK(zcd.read(0x0102) == 0x01);
    zcd.write(0x0102, 0x01);
    CHECK(zcd.read(0x0102) == 0);
}


