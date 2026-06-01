#include "doctest.h"
#include "vioavr/core/cfd.hpp"

using namespace vioavr::core;

static CfdDescriptor make_cfd_desc(u16 xfdcsr_addr = 0x01C0) {
    CfdDescriptor d{};
    d.xfdcsr_address = xfdcsr_addr;
    d.vector_index = 30;
    return d;
}

TEST_CASE("CFD: Register access") {
    Cfd cfd(make_cfd_desc());
    CHECK(cfd.mapped_ranges().size() == 1);
    CHECK(cfd.read(0x01C0) == 0);
    CHECK(cfd.read(0x01C1) == 0);
    CHECK(cfd.read(0x01C2) == 0);
    CHECK(cfd.read(0x01C3) == 0);
    cfd.write(0x01C0, 0x01);
    CHECK(cfd.read(0x01C0) == 0x01);
    cfd.write(0x01C3, 0x03);
    CHECK(cfd.read(0x01C3) == 0x03);
}

TEST_CASE("CFD: Reset clears registers") {
    Cfd cfd(make_cfd_desc());
    cfd.write(0x01C0, 0x01);
    cfd.write(0x01C3, 0x03);
    cfd.reset();
    CHECK(cfd.read(0x01C0) == 0);
    CHECK(cfd.read(0x01C3) == 0);
}

TEST_CASE("CFD: Edge prevents failure") {
    Cfd cfd(make_cfd_desc());
    cfd.write(0x01C0, 0x01);
    for (u64 i = 0; i < 1024; i++) cfd.tick(1);
    CHECK((cfd.read(0x01C2) & 0x02) == 0x02);
    cfd.write(0x01C2, 0x02);
    cfd.reset();
    cfd.write(0x01C0, 0x01);
    for (u64 i = 0; i < 500; i++) cfd.tick(1);
    cfd.on_edge();
    for (u64 i = 0; i < 500; i++) cfd.tick(1);
    cfd.on_edge();
    for (u64 i = 0; i < 500; i++) cfd.tick(1);
    cfd.on_edge();
    cfd.write(0x01C2, 0x02);
    for (u64 i = 0; i < 1100; i++) cfd.tick(1);
    CHECK((cfd.read(0x01C2) & 0x02) == 0x02);
}

TEST_CASE("CFD: Interrupt on failure") {
    Cfd cfd(make_cfd_desc());
    cfd.write(0x01C0, 0x01);
    cfd.write(0x01C3, 0x02);
    for (u64 i = 0; i < 1100; i++) cfd.tick(1);
    InterruptRequest req;
    CHECK(cfd.pending_interrupt_request(req));
    CHECK(req.vector_index == 30);
    CHECK(cfd.consume_interrupt_request(req));
    CHECK(!cfd.pending_interrupt_request(req));
}

TEST_CASE("CFD: No interrupt without intctrl") {
    Cfd cfd(make_cfd_desc());
    cfd.write(0x01C0, 0x01);
    for (u64 i = 0; i < 1100; i++) cfd.tick(1);
    InterruptRequest req;
    CHECK(!cfd.pending_interrupt_request(req));
}

TEST_CASE("CFD: Intflags write-one-to-clear") {
    Cfd cfd(make_cfd_desc());
    cfd.write(0x01C0, 0x01);
    for (u64 i = 0; i < 1100; i++) cfd.tick(1);
    CHECK((cfd.read(0x01C2) & 0x02) == 0x02);
    cfd.write(0x01C2, 0x02);
    CHECK((cfd.read(0x01C2) & 0x02) == 0x00);
}

TEST_CASE("CFD: Disabled when XFDCSR bit 0 clear") {
    Cfd cfd(make_cfd_desc());
    cfd.write(0x01C3, 0x02);
    for (u64 i = 0; i < 2000; i++) cfd.tick(1);
    InterruptRequest req;
    CHECK(!cfd.pending_interrupt_request(req));
}
