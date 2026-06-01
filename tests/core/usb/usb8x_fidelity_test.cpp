#include "doctest.h"
#include "vioavr/core/usb8x.hpp"

using namespace vioavr::core;

static Usb8xDescriptor make_test_desc() noexcept {
    Usb8xDescriptor d{};
    d.ctrla_address = 0x10;
    d.ctrlb_address = 0x11;
    d.busstate_address = 0x12;
    d.addr_address = 0x13;
    d.fifowp_address = 0x14;
    d.fiforp_address = 0x15;
    d.epptr_address = 0x16;
    d.intctrla_address = 0x17;
    d.intctrlb_address = 0x18;
    d.intflagsa_address = 0x19;
    d.intflagsb_address = 0x1A;
    d.cal0_address = 0x1B;
    d.cal1_address = 0x1C;
    d.pllcsr_address = 0x1D;
    d.gen_vector_index = 42;
    d.usb_pll_frequency_hz = 48000000ULL;
    return d;
}

TEST_CASE("USB8x — Register access") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    CHECK(usb.read(d.ctrla_address) == 0);
    usb.write(d.ctrla_address, 0x03U);
    CHECK(usb.read(d.ctrla_address) == 0x03U);

    usb.write(d.ctrlb_address, 0x0AU);
    CHECK(usb.read(d.ctrlb_address) == 0x0AU);

    usb.write(d.busstate_address, 0x01U);
    CHECK(usb.read(d.busstate_address) == 0x01U);

    usb.write(d.addr_address, 0x2AU);
    CHECK(usb.read(d.addr_address) == 0x2AU);

    usb.write(d.epptr_address, 0x40U);
    CHECK(usb.read(d.epptr_address) == 0x40U);
}

TEST_CASE("USB8x — INTCTRL and INTFLAGS") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    usb.write(d.intctrla_address, 0x0FU);
    CHECK(usb.read(d.intctrla_address) == 0x0FU);

    usb.write(d.intctrlb_address, 0x07U);
    CHECK(usb.read(d.intctrlb_address) == 0x07U);

    // INTFLAGS: write-1-to-clear
    usb.write(d.intflagsa_address, 0x0FU);
    CHECK(usb.read(d.intflagsa_address) == 0x00U);

    usb.write(d.intflagsb_address, 0x07U);
    CHECK(usb.read(d.intflagsb_address) == 0x00U);
}

TEST_CASE("USB8x — FIFO write/read") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    CHECK(usb.fifo_count() == 0);

    usb.write(d.fifowp_address, 0xAAU);
    CHECK(usb.fifo_count() == 1);
    usb.write(d.fifowp_address, 0xBBU);
    CHECK(usb.fifo_count() == 2);
    usb.write(d.fifowp_address, 0xCCU);
    CHECK(usb.fifo_count() == 3);

    CHECK(usb.read(d.fiforp_address) == 0xAAU);
    CHECK(usb.fifo_count() == 2);
    CHECK(usb.read(d.fiforp_address) == 0xBBU);
    CHECK(usb.fifo_count() == 1);
}

TEST_CASE("USB8x — FIFO overflow (max 64 bytes)") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    for (int i = 0; i < 64; ++i) {
        usb.write(d.fifowp_address, static_cast<u8>(i));
    }
    CHECK(usb.fifo_count() == 64);

    // 65th write is dropped (FIFO full)
    usb.write(d.fifowp_address, 0xFFU);
    CHECK(usb.fifo_count() == 64);
}

TEST_CASE("USB8x — Interrupt dispatch") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    usb.write(d.intctrlb_address, 0x02U);
    usb.write(d.fifowp_address, 0xAAU);

    InterruptRequest req;
    CHECK(usb.pending_interrupt_request(req));
    CHECK(req.vector_index == d.gen_vector_index);

    CHECK(usb.consume_interrupt_request(req));
    CHECK(req.vector_index == d.gen_vector_index);

    CHECK(!usb.pending_interrupt_request(req));
}

TEST_CASE("USB8x — SOF generation via tick") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    // Enable USB and PLL
    usb.write(d.ctrla_address, 0x01U);
    usb.write(d.pllcsr_address, 0x01U);

    // At 48MHz PLL, SOF period = 48MHz/1000 = 48000 cycles
    // Tick just under one frame
    usb.tick(47999);
    CHECK(usb.read(d.intflagsa_address) == 0x00U);

    // Tick across the boundary
    usb.tick(2);
    CHECK((usb.read(d.intflagsa_address) & 0x02) != 0);
}

TEST_CASE("USB8x — SOF interrupt fires when INTCTRL enabled") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    usb.write(d.ctrla_address, 0x01U);
    usb.write(d.pllcsr_address, 0x01U);
    usb.write(d.intctrla_address, 0x02U); // enable SOF interrupt

    // Tick past one frame
    usb.tick(48000);

    InterruptRequest req;
    CHECK(usb.pending_interrupt_request(req));
    CHECK(req.vector_index == d.gen_vector_index);
}

TEST_CASE("USB8x — No SOF when USB disabled") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    // PLL enabled but USB disabled
    usb.write(d.pllcsr_address, 0x01U);
    usb.tick(48000);
    CHECK(usb.read(d.intflagsa_address) == 0x00U);
}

TEST_CASE("USB8x — No SOF when PLL disabled") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    // USB enabled but PLL disabled
    usb.write(d.ctrla_address, 0x01U);
    usb.tick(48000);
    CHECK(usb.read(d.intflagsa_address) == 0x00U);
}

TEST_CASE("USB8x — SOF resets on device reset") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    usb.write(d.ctrla_address, 0x01U);
    usb.write(d.pllcsr_address, 0x01U);
    usb.tick(48000);
    CHECK((usb.read(d.intflagsa_address) & 0x02) != 0);

    usb.reset();
    CHECK(usb.read(d.intflagsa_address) == 0x00U);
}

TEST_CASE("USB8x — Multiple SOF events accumulate") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    usb.write(d.ctrla_address, 0x01U);
    usb.write(d.pllcsr_address, 0x01U);

    // Tick past 3 frames
    usb.tick(144000);
    CHECK((usb.read(d.intflagsa_address) & 0x02) != 0);
}

TEST_CASE("USB8x — Setup packet detection via FIFO") {
    auto d = make_test_desc();
    d.bus_vector_index = 43;
    Usb8x usb(d);
    usb.reset();

    // Set EPPTR to EP0 SETUP (type=4, epnum=0)
    usb.write(d.epptr_address, 0x04U);

    // Write 8-byte setup packet to FIFO
    // GET_DESCRIPTOR request: 0x80 0x06 0x00 0x01 0x00 0x00 0x12 0x00
    u8 setup[] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00};
    for (auto byte : setup) {
        usb.write(d.fifowp_address, byte);
    }

    CHECK(usb.setup_packet()[0] == 0x80);
    CHECK(usb.setup_packet()[1] == 0x06);
    CHECK(usb.ep0_data_dir() == 1);
    CHECK(usb.ep0_wLength() == 0x12);
    CHECK(!usb.ep0_stalled());

    // SETUP interrupt flag should be set
    CHECK((usb.read(d.intflagsb_address) & 0x04) != 0);
}

TEST_CASE("USB8x — Setup packet on non-SETUP endpoint ignored") {
    auto d = make_test_desc();
    Usb8x usb(d);
    usb.reset();

    // EPPTR = BULK OUT (type=2, epnum=1)
    usb.write(d.epptr_address, 0x12U);

    u8 setup[] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x12, 0x00};
    for (auto byte : setup) {
        usb.write(d.fifowp_address, byte);
    }

    // No SETUP flag for non-SETUP endpoint
    CHECK((usb.read(d.intflagsb_address) & 0x04) == 0);
}

TEST_CASE("USB8x — BUSSTATE transitions fire interrupts") {
    auto d = make_test_desc();
    d.bus_vector_index = 43;
    Usb8x usb(d);
    usb.reset();

    usb.write(d.busstate_address, 0x02U); // DISCON → CONN
    usb.write(d.busstate_address, 0x03U); // CONN → SUSPEND (fires SUSPEND)
    CHECK((usb.read(d.intflagsa_address) & 0x01) != 0);

    usb.write(d.intflagsa_address, 0x01U); // write-1-to-clear

    usb.write(d.busstate_address, 0x01U); // SUSPEND → DISCON (fires WAKEUP)
    CHECK((usb.read(d.intflagsa_address) & 0x08) != 0);
}

TEST_CASE("USB8x — CTRLB ATTACH transitions to CONN state") {
    auto d = make_test_desc();
    d.bus_vector_index = 43;
    Usb8x usb(d);
    usb.reset();

    CHECK(usb.bus_state() == 0);

    usb.write(d.ctrlb_address, 0x02U); // ATTACH
    CHECK(usb.bus_state() == 2);
}

TEST_CASE("USB8x — Bus interrupt uses bus_vector_index") {
    auto d = make_test_desc();
    d.bus_vector_index = 43;
    d.gen_vector_index = 42;
    Usb8x usb(d);
    usb.reset();

    // Trigger a bus event (SUSPEND CONN→SUSPEND)
    usb.write(d.intctrla_address, 0x01U); // enable SUSPEND
    usb.write(d.busstate_address, 0x02U);
    usb.write(d.busstate_address, 0x03U);

    InterruptRequest req;
    CHECK(usb.pending_interrupt_request(req));
    CHECK(req.vector_index == 43); // bus_vector_index
}

TEST_CASE("USB8x — Data interrupt uses gen_vector_index") {
    auto d = make_test_desc();
    d.bus_vector_index = 43;
    d.gen_vector_index = 42;
    Usb8x usb(d);
    usb.reset();

    // Trigger a data event (TRANS via FIFO write)
    usb.write(d.intctrlb_address, 0x02U); // enable TRANS
    usb.write(d.fifowp_address, 0xAAU);

    InterruptRequest req;
    CHECK(usb.pending_interrupt_request(req));
    CHECK(req.vector_index == 42); // gen_vector_index
}

TEST_CASE("USB8x — Address range covers all registers") {
    auto d = make_test_desc();
    Usb8x usb(d);

    auto ranges = usb.mapped_ranges();
    REQUIRE(!ranges.empty());

    u16 r_begin = ranges[0].begin;
    u16 r_end = ranges[0].end;

    CHECK(d.ctrla_address >= r_begin);
    CHECK(d.ctrla_address <= r_end);
    CHECK(d.ctrlb_address >= r_begin);
    CHECK(d.ctrlb_address <= r_end);
    CHECK(d.fifowp_address >= r_begin);
    CHECK(d.fifowp_address <= r_end);
    CHECK(d.fiforp_address >= r_begin);
    CHECK(d.fiforp_address <= r_end);
    CHECK(d.intctrla_address >= r_begin);
    CHECK(d.intctrla_address <= r_end);
    CHECK(d.intflagsa_address >= r_begin);
    CHECK(d.intflagsa_address <= r_end);
}
