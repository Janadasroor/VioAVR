#include "doctest.h"
#include "vioavr/core/mvio.hpp"

using namespace vioavr::core;

static MvioDescriptor make_mvio_desc(u16 intctrl_addr = 0x01A0) {
    MvioDescriptor d{};
    d.intctrl_address = intctrl_addr;
    d.intflags_address = intctrl_addr + 1;
    d.status_address = intctrl_addr + 2;
    d.vector_index = 12;
    return d;
}

TEST_CASE("MVIO: Register access") {
    Mvio mvio(make_mvio_desc());
    CHECK(mvio.mapped_ranges().size() == 1);
    CHECK(mvio.read(0x01A0) == 0);
    CHECK(mvio.read(0x01A1) == 0);
    CHECK(mvio.read(0x01A2) == 0);
    mvio.write(0x01A0, 0x03);
    CHECK(mvio.read(0x01A0) == 0x03);
}

TEST_CASE("MVIO: Reset clears registers") {
    Mvio mvio(make_mvio_desc());
    mvio.write(0x01A0, 0x03);
    mvio.set_vddio2(3.3);
    mvio.reset();
    CHECK(mvio.read(0x01A0) == 0);
    CHECK(mvio.read(0x01A1) == 0);
}

TEST_CASE("MVIO: VDDIO2 above threshold sets status") {
    Mvio mvio(make_mvio_desc());
    mvio.set_vddio2(3.3);
    mvio.tick(1);
    CHECK((mvio.read(0x01A2) & 0x01) == 0x01);
}

TEST_CASE("MVIO: VDDIO2 below threshold clears status") {
    Mvio mvio(make_mvio_desc());
    mvio.set_vddio2(3.3);
    mvio.tick(1);
    CHECK((mvio.read(0x01A2) & 0x01) == 0x01);
    mvio.set_vddio2(0.0);
    mvio.tick(1);
    CHECK((mvio.read(0x01A2) & 0x01) == 0x00);
}

TEST_CASE("MVIO: VDDIO2 rising triggers interrupt") {
    Mvio mvio(make_mvio_desc());
    mvio.write(0x01A0, 0x02);
    mvio.set_vddio2(3.3);
    mvio.tick(1);
    CHECK((mvio.read(0x01A1) & 0x02) == 0x02);
    InterruptRequest req;
    CHECK(mvio.pending_interrupt_request(req));
    CHECK(req.vector_index == 12);
}

TEST_CASE("MVIO: VDDIO2 falling triggers interrupt") {
    Mvio mvio(make_mvio_desc());
    mvio.set_vddio2(3.3);
    mvio.tick(1);
    mvio.write(0x01A0, 0x01);
    mvio.set_vddio2(0.0);
    mvio.tick(1);
    CHECK((mvio.read(0x01A1) & 0x01) == 0x01);
}

TEST_CASE("MVIO: Intflags write-one-to-clear") {
    Mvio mvio(make_mvio_desc());
    mvio.write(0x01A0, 0x02);
    mvio.set_vddio2(3.3);
    mvio.tick(1);
    CHECK((mvio.read(0x01A1) & 0x02) == 0x02);
    mvio.write(0x01A1, 0x02);
    CHECK((mvio.read(0x01A1) & 0x02) == 0x00);
}

TEST_CASE("MVIO: No duplicate interrupt on same state") {
    Mvio mvio(make_mvio_desc());
    mvio.write(0x01A0, 0x02);
    mvio.set_vddio2(3.3);
    mvio.tick(1);
    CHECK((mvio.read(0x01A1) & 0x02) == 0x02);
    mvio.write(0x01A1, 0x02);
    mvio.tick(1);
    CHECK((mvio.read(0x01A1) & 0x02) == 0x00);
}

TEST_CASE("MVIO: No interrupt without intctrl") {
    Mvio mvio(make_mvio_desc());
    mvio.set_vddio2(3.3);
    mvio.tick(1);
    CHECK(mvio.read(0x01A1) == 0);
    InterruptRequest req;
    CHECK(!mvio.pending_interrupt_request(req));
}
