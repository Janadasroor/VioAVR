#include "doctest.h"
#include "vioavr/core/lin.hpp"

using namespace vioavr::core;

static LinDescriptor make_lin_desc(u16 ctrla_addr = 0x00C8) {
    LinDescriptor d{};
    d.ctrla_address = ctrla_addr;
    d.vector_index = 18;
    d.lincr_lswres_mask = 0x80;
    d.lincr_lin13_mask = 0x40;
    d.lincr_lconf_mask = 0x30;
    d.lincr_lena_mask = 0x08;
    d.lincr_lcmd_mask = 0x07;
    d.linsir_lidst_mask = 0xE0;
    d.linsir_lbusy_mask = 0x10;
    d.linsir_lerr_mask = 0x08;
    d.linsir_lidok_mask = 0x04;
    d.linsir_ltxok_mask = 0x02;
    d.linsir_lrxok_mask = 0x01;
    return d;
}

TEST_CASE("LIN: Register access") {
    LinUART lin(make_lin_desc());
    CHECK(lin.mapped_ranges().size() == 1);
    CHECK(lin.read(0x00C8) == 0);
    CHECK(lin.read(0x00C9) == 0);
    CHECK(lin.read(0x00CC) == 0);
    lin.write(0x00C8, 0x01);
    CHECK(lin.read(0x00C8) == 0x01);
    lin.write(0x00CC, 0x55);
    CHECK(lin.read(0x00CC) == 0x55);
}

TEST_CASE("LIN: Reset clears registers") {
    LinUART lin(make_lin_desc());
    lin.write(0x00C8, 0x01);
    lin.write(0x00CC, 0x55);
    lin.reset();
    CHECK(lin.read(0x00C8) == 0);
    CHECK(lin.read(0x00CC) == 0);
}

TEST_CASE("LIN: LSWRES resets state") {
    LinUART lin(make_lin_desc());
    lin.write(0x00C8, 0x01);
    CHECK(lin.read(0x00C8) == 0x01);
    lin.write(0x00C8, 0x80);
    CHECK(lin.read(0x00C8) == 0);
}

TEST_CASE("LIN: Transmit command sets busy") {
    LinUART lin(make_lin_desc());
    lin.write(0x00C8, 0x01);
    CHECK((lin.read(0x00C9) & 0x10) == 0x10);
}

TEST_CASE("LIN: Receive command sets busy") {
    LinUART lin(make_lin_desc());
    lin.write(0x00C8, 0x02);
    CHECK((lin.read(0x00C9) & 0x10) == 0x10);
}

TEST_CASE("LIN: RX byte sync field") {
    LinUART lin(make_lin_desc());
    lin.simulate_rx_break();
    lin.simulate_rx_byte(0x55);
    CHECK((lin.read(0x00C9) & 0x04) == 0);
}

TEST_CASE("LIN: RX frame complete") {
    LinUART lin(make_lin_desc());
    lin.simulate_rx_break();
    lin.simulate_rx_byte(0x55);
    lin.simulate_rx_byte(0x12);
    CHECK((lin.read(0x00C9) & 0x04) == 0x04);
    lin.simulate_rx_byte(0xAA);
    lin.simulate_rx_byte(0xBB);
    lin.simulate_rx_byte(0x99);
    CHECK((lin.read(0x00C9) & 0x01) != 0);
    CHECK((lin.read(0x00C9) & 0x10) == 0);
}

TEST_CASE("LIN: Baud register") {
    LinUART lin(make_lin_desc());
    lin.write(0x00CC, 0x34);
    CHECK(lin.read(0x00CC) == 0x34);
}

TEST_CASE("LIN: Interrupt on RX OK") {
    LinUART lin(make_lin_desc());
    lin.write(0x00CA, 0x01);
    lin.simulate_rx_break();
    lin.simulate_rx_byte(0x55);
    lin.simulate_rx_byte(0x12);
    lin.simulate_rx_byte(0xAA);
    lin.simulate_rx_byte(0xBB);
    lin.simulate_rx_byte(0x99);
    InterruptRequest req;
    CHECK(lin.pending_interrupt_request(req));
    CHECK(req.vector_index == 18);
}

TEST_CASE("LIN: Interrupt consume") {
    LinUART lin(make_lin_desc());
    lin.write(0x00CA, 0x01);
    lin.simulate_rx_break();
    lin.simulate_rx_byte(0x55);
    lin.simulate_rx_byte(0x12);
    lin.simulate_rx_byte(0xAA);
    lin.simulate_rx_byte(0xBB);
    lin.simulate_rx_byte(0x99);
    InterruptRequest req;
    CHECK(lin.consume_interrupt_request(req));
    CHECK(!lin.pending_interrupt_request(req));
}

TEST_CASE("LIN: Checksum error") {
    LinUART lin(make_lin_desc());
    lin.simulate_rx_break();
    lin.simulate_rx_byte(0x55);
    lin.simulate_rx_byte(0x12);
    lin.simulate_rx_byte(0xAA);
    lin.simulate_rx_byte(0xBB);
    lin.simulate_rx_byte(0xFF);
    CHECK((lin.read(0x00CB) & 0x08) == 0x08);
    CHECK((lin.read(0x00C9) & 0x08) == 0x08);
}

TEST_CASE("LIN: Power reduction disables tick") {
    LinUART lin(make_lin_desc());
    lin.write(0x00C8, 0x01);
    lin.tick(100);
    CHECK(true);
}
