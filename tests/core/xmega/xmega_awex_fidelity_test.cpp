#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/awex.hpp"

using namespace vioavr::core;

static AwexDescriptor make_test_awex() noexcept {
    AwexDescriptor d{};
    d.ctrl_address = 0x0880;
    d.fdemask_address = 0x0882;
    d.fdctrl_address = 0x0883;
    d.status_address = 0x0884;
    d.dtboth_address = 0x0886;
    d.dtbothbuf_address = 0x0887;
    d.dtls_address = 0x0888;
    d.dths_address = 0x0889;
    d.dtlsbuf_address = 0x088A;
    d.dthsbuf_address = 0x088B;
    d.outoven_address = 0x088C;
    return d;
}

TEST_CASE("XMEGA AWEX Fidelity: Register Access") {
    auto d = make_test_awex();
    Awex awex("AWEXC", d);
    awex.reset();

    awex.write(d.ctrl_address, 0x0F);
    CHECK(awex.read(d.ctrl_address) == 0x0F);

    awex.write(d.dtboth_address, 0x10);
    CHECK(awex.read(d.dtboth_address) == 0x10);

    awex.write(d.outoven_address, 0x33);
    CHECK(awex.read(d.outoven_address) == 0x33);
}

TEST_CASE("XMEGA AWEX Fidelity: Dead Time Insertion") {
    auto d = make_test_awex();
    Awex awex("AWEXC", d);
    awex.reset();

    awex.write(d.ctrl_address, 0x01); // Enable DT on channel 0
    awex.write(d.dtboth_address, 5);   // 5 cycles dead time

    awex.set_wo_level(0, true);
    CHECK(awex.get_dh_level(0) == true);
    CHECK(awex.get_dl_level(0) == false);

    awex.set_wo_level(0, false);
    CHECK(awex.get_dh_level(0) == false);

    // During dead-time, DL should still be low (dead-time not expired)
    // After tick, dead-time expires
    awex.tick(5);
    CHECK(awex.get_dh_level(0) == false);
    CHECK(awex.get_dl_level(0) == true);
}

TEST_CASE("XMEGA AWEX Fidelity: OUTOVEN Override") {
    auto d = make_test_awex();
    Awex awex("AWEXC", d);
    awex.reset();

    // OUTOVEN: ch0 override enabled, forced high (bit 4 = 1)
    awex.write(d.outoven_address, 0x11);
    awex.set_wo_level(0, false);
    CHECK(awex.get_dh_level(0) == true);
}
