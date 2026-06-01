#include "doctest.h"
#include "vioavr/core/tcd.hpp"

namespace {
using namespace vioavr::core;

TcdDescriptor make_desc() noexcept {
    TcdDescriptor desc{};
    desc.base_address = 0x0A80;
    desc.ovf_vector_index = 14;
    desc.trig_vector_index = 15;
    desc.woa_pin_address = 0x04; // PORTA_OUT
    desc.woa_pin_bit = 4;
    desc.wob_pin_address = 0x04;
    desc.wob_pin_bit = 5;
    desc.woc_pin_address = 0x06; // PORTC_OUT
    desc.woc_pin_bit = 0;
    desc.wod_pin_address = 0x06;
    desc.wod_pin_bit = 1;
    return desc;
}

TEST_CASE("TCD: Register defaults after reset") {
    auto desc = make_desc();
    Tcd tcd{desc};
    tcd.reset();

    CHECK(tcd.read(desc.base_address + 0x00) == 0x00); // CTRLA
    CHECK(tcd.read(desc.base_address + 0x01) == 0x00); // CTRLB
    CHECK(tcd.read(desc.base_address + 0x02) == 0x00); // CTRLC
    CHECK(tcd.read(desc.base_address + 0x03) == 0x00); // CTRLD
    CHECK(tcd.read(desc.base_address + 0x04) == 0x00); // CTRLE
    CHECK(tcd.read(desc.base_address + 0x08) == 0x00); // EVCTRLA
    CHECK(tcd.read(desc.base_address + 0x09) == 0x00); // EVCTRLB
    CHECK(tcd.read(desc.base_address + 0x0C) == 0x00); // INTCTRL
    CHECK(tcd.read(desc.base_address + 0x0D) == 0x00); // INTFLAGS
    CHECK(tcd.read(desc.base_address + 0x0E) == 0x00); // STATUS
    CHECK(tcd.read(desc.base_address + 0x10) == 0x00); // INPUTCTRLA
    CHECK(tcd.read(desc.base_address + 0x11) == 0x00); // INPUTCTRLB
    CHECK(tcd.read(desc.base_address + 0x12) == 0x00); // FAULTCTRL
    CHECK(tcd.read(desc.base_address + 0x14) == 0x00); // DLYCTRL
    CHECK(tcd.read(desc.base_address + 0x15) == 0x00); // DLYVAL
    CHECK(tcd.read(desc.base_address + 0x18) == 0x00); // DITCTRL
    CHECK(tcd.read(desc.base_address + 0x19) == 0x00); // DITVAL
    CHECK(tcd.read(desc.base_address + 0x1E) == 0x00); // DBGCTRL
    CHECK(tcd.read(desc.base_address + 0x22) == 0x00); // CAPTUREA (low)
    CHECK(tcd.read(desc.base_address + 0x23) == 0x00); // CAPTUREA (high)
    CHECK(tcd.read(desc.base_address + 0x24) == 0x00); // CAPTUREB (low)
    CHECK(tcd.read(desc.base_address + 0x25) == 0x00); // CAPTUREB (high)
    CHECK(tcd.read(desc.base_address + 0x28) == 0x00); // CMPASET (low)
    CHECK(tcd.read(desc.base_address + 0x29) == 0x00); // CMPASET (high)
    CHECK(tcd.read(desc.base_address + 0x2A) == 0x00); // CMPACLR (low)
    CHECK(tcd.read(desc.base_address + 0x2B) == 0x00); // CMPACLR (high)
    CHECK(tcd.read(desc.base_address + 0x2C) == 0x00); // CMPBSET (low)
    CHECK(tcd.read(desc.base_address + 0x2D) == 0x00); // CMPBSET (high)
    CHECK(tcd.read(desc.base_address + 0x2E) == 0x00); // CMPBCLR (low)
    CHECK(tcd.read(desc.base_address + 0x2F) == 0x00); // CMPBCLR (high)
}

TEST_CASE("TCD: Register write/read") {
    auto desc = make_desc();
    Tcd tcd{desc};

    tcd.write(desc.base_address + 0x00, 0x01); // CTRLA.ENABLE
    tcd.write(desc.base_address + 0x01, 0x02); // CTRLB
    tcd.write(desc.base_address + 0x02, 0x80); // CTRLC
    tcd.write(desc.base_address + 0x03, 0xFF); // CTRLD
    tcd.write(desc.base_address + 0x0C, 0x05); // INTCTRL (OVF + TRIGA)
    tcd.write(desc.base_address + 0x28, 0x34); // CMPASET low
    tcd.write(desc.base_address + 0x29, 0x01); // CMPASET high

    CHECK(tcd.read(desc.base_address + 0x00) == 0x01);
    CHECK(tcd.read(desc.base_address + 0x01) == 0x02);
    CHECK(tcd.read(desc.base_address + 0x02) == 0x80);
    CHECK(tcd.read(desc.base_address + 0x03) == 0xFF);
    CHECK(tcd.read(desc.base_address + 0x0C) == 0x05);
    CHECK(tcd.read(desc.base_address + 0x28) == 0x34);
    CHECK(tcd.read(desc.base_address + 0x29) == 0x01);
}

TEST_CASE("TCD: Software reset clears registers") {
    auto desc = make_desc();
    Tcd tcd{desc};

    tcd.write(desc.base_address + 0x00, 0x01);
    tcd.write(desc.base_address + 0x01, 0x03);
    tcd.write(desc.base_address + 0x0C, 0x0D);
    tcd.write(desc.base_address + 0x28, 0xFF);
    tcd.write(desc.base_address + 0x29, 0x0F);

    tcd.reset();

    CHECK(tcd.read(desc.base_address + 0x00) == 0x00);
    CHECK(tcd.read(desc.base_address + 0x01) == 0x00);
    CHECK(tcd.read(desc.base_address + 0x0C) == 0x00);
    CHECK(tcd.read(desc.base_address + 0x28) == 0x00);
    CHECK(tcd.read(desc.base_address + 0x29) == 0x00);
}

TEST_CASE("TCD: Mapped ranges") {
    auto desc = make_desc();
    Tcd tcd{desc};

    auto ranges = tcd.mapped_ranges();
    REQUIRE(ranges.size() == 1);
    CHECK(ranges[0].begin == desc.base_address);
    CHECK(ranges[0].end == desc.base_address + 0x2E);
}

TEST_CASE("TCD: No ranges with zero base") {
    TcdDescriptor empty_desc{};
    Tcd tcd{empty_desc};
    auto ranges = tcd.mapped_ranges();
    CHECK(ranges.size() == 0);
}

TEST_CASE("TCD: Empty descriptor no-op") {
    TcdDescriptor empty_desc{};
    Tcd tcd{empty_desc};
    tcd.reset();
    tcd.tick(1000);
    // No crash
}

TEST_CASE("TCD: OVF interrupt on period match in ONERAMP mode") {
    auto desc = make_desc();
    Tcd tcd{desc};

    // Enable OVF interrupt
    tcd.write(desc.base_address + 0x0C, 0x01); // INTCTRL.OVF
    tcd.write(desc.base_address + 0x00, 0x01); // CTRLA.ENABLE

    // Set period via CMPBCLR
    tcd.write(desc.base_address + 0x2E, 0x10); // CMPBCLR low
    tcd.write(desc.base_address + 0x2F, 0x00); // CMPBCLR high (period = 16)

    // Tick enough cycles for ~17 counter steps
    tcd.tick(400);

    // Should have OVF flag at some point
    u8 flags = tcd.read(desc.base_address + 0x0D);
    CHECK((flags & 0x01) != 0);

    InterruptRequest req;
    CHECK(tcd.pending_interrupt_request(req));
    CHECK(req.vector_index == 14);

    CHECK(tcd.consume_interrupt_request(req));
    CHECK(req.vector_index == 14);
    flags = tcd.read(desc.base_address + 0x0D);
    CHECK((flags & 0x01) == 0);
}

TEST_CASE("TCD: TRIGA interrupt on CMPASET match") {
    auto desc = make_desc();
    Tcd tcd{desc};

    tcd.write(desc.base_address + 0x0C, 0x04); // INTCTRL.TRIGA
    tcd.write(desc.base_address + 0x00, 0x01); // CTRLA.ENABLE

    // Set compare set values
    tcd.write(desc.base_address + 0x28, 0x08); // CMPASET low (set at 8)
    tcd.write(desc.base_address + 0x29, 0x00); // CMPASET high
    // Period via CMPBCLR
    tcd.write(desc.base_address + 0x2E, 0x20); // CMPBCLR low
    tcd.write(desc.base_address + 0x2F, 0x00); // CMPBCLR high (period = 32)

    // Tick past CMPASET
    tcd.tick(400);

    // Should have TRIGA flag
    u8 flags = tcd.read(desc.base_address + 0x0D);
    CHECK((flags & 0x04) != 0);

    InterruptRequest req;
    CHECK(tcd.pending_interrupt_request(req));
    CHECK(req.vector_index == 15);

    CHECK(tcd.consume_interrupt_request(req));
    CHECK(req.vector_index == 15);
    flags = tcd.read(desc.base_address + 0x0D);
    CHECK((flags & 0x04) == 0);
}

TEST_CASE("TCD: Clear interrupt flags via write-1-to-clear") {
    auto desc = make_desc();
    Tcd tcd{desc};

    tcd.write(desc.base_address + 0x0C, 0x01); // INTCTRL.OVF
    tcd.write(desc.base_address + 0x00, 0x01); // CTRLA.ENABLE
    tcd.write(desc.base_address + 0x2E, 0x04); // CMPBCLR low (period = 4)
    tcd.tick(200);

    u8 flags = tcd.read(desc.base_address + 0x0D);
    CHECK((flags & 0x01) != 0); // OVF set

    tcd.write(desc.base_address + 0x0D, 0x01); // Write 1 to clear
    flags = tcd.read(desc.base_address + 0x0D);
    CHECK((flags & 0x01) == 0);
}

TEST_CASE("TCD: CTRLA reserved bits masked") {
    auto desc = make_desc();
    Tcd tcd{desc};

    tcd.write(desc.base_address + 0x00, 0xFF);
    CHECK(tcd.read(desc.base_address + 0x00) == 0x67);
}

TEST_CASE("TCD: CMP registers 12-bit") {
    auto desc = make_desc();
    Tcd tcd{desc};

    // Write 12-bit values
    tcd.write(desc.base_address + 0x28, 0xFF); // CMPASET low
    tcd.write(desc.base_address + 0x29, 0x0A); // CMPASET high (10 << 8 | FF = 0xAFF)
    CHECK(tcd.read(desc.base_address + 0x28) == 0xFF);
    CHECK(tcd.read(desc.base_address + 0x29) == 0x0A);

    tcd.write(desc.base_address + 0x29, 0x0F); // max high nibble
    CHECK(tcd.read(desc.base_address + 0x29) == 0x0F);

    tcd.write(desc.base_address + 0x29, 0x0F); // Should still be 12-bit
    CHECK(tcd.read(desc.base_address + 0x29) == 0x0F);
}

TEST_CASE("TCD: CAPTUREA/B read-only") {
    auto desc = make_desc();
    Tcd tcd{desc};

    // Write to capture registers should be ignored
    tcd.write(desc.base_address + 0x22, 0xAB);
    tcd.write(desc.base_address + 0x23, 0xCD);
    CHECK(tcd.read(desc.base_address + 0x22) == 0x00);
    CHECK(tcd.read(desc.base_address + 0x23) == 0x00);
}

TEST_CASE("TCD: RESTART strobe resets counter") {
    auto desc = make_desc();
    Tcd tcd{desc};

    tcd.write(desc.base_address + 0x00, 0x01); // ENABLE
    tcd.write(desc.base_address + 0x2E, 0x10); // CMPBCLR = 16
    tcd.tick(100); // Advance counter

    // Write RESTART to CTRLE
    tcd.write(desc.base_address + 0x04, 0x04); // CTRLE.RESTART
}

TEST_CASE("TCD: DISEOC strobe disables timer") {
    auto desc = make_desc();
    Tcd tcd{desc};

    tcd.write(desc.base_address + 0x00, 0x01); // ENABLE
    tcd.write(desc.base_address + 0x04, 0x80); // CTRLE.DISEOC

    CHECK(tcd.read(desc.base_address + 0x00) == 0x00); // ENABLE cleared
}

TEST_CASE("TCD: Not pending when disabled") {
    auto desc = make_desc();
    Tcd tcd{desc};

    InterruptRequest req;
    CHECK_FALSE(tcd.pending_interrupt_request(req));
    CHECK_FALSE(tcd.consume_interrupt_request(req));
}
} // namespace
