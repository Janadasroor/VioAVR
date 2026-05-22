#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/opamp.hpp"

namespace {
using namespace vioavr::core;
}

OpampDescriptor make_desc() noexcept {
    OpampDescriptor desc {};
    desc.ctrla_address = 0x100;
    desc.ctrlb_address = 0x101;
    desc.resctrl_address = 0x102;
    desc.muxctrl_address = 0x103;
    desc.out_generator_id = 1;
    desc.pos_pin_address = 0x200;
    desc.pos_pin_bit = 0;
    desc.neg_pin_address = 0x200;
    desc.neg_pin_bit = 1;
    desc.out_pin_address = 0x200;
    desc.out_pin_bit = 2;
    return desc;
}

TEST_CASE("OPAMP: Register Read/Write") {
    auto desc = make_desc();
    Opamp opamp {desc};
    opamp.reset();

    CHECK(opamp.read(desc.ctrla_address) == 0x00);
    CHECK(opamp.read(desc.ctrlb_address) == 0x00);
    CHECK(opamp.read(desc.resctrl_address) == 0x00);
    CHECK(opamp.read(desc.muxctrl_address) == 0x00);

    opamp.write(desc.ctrla_address, 0xAB);
    opamp.write(desc.ctrlb_address, 0xCD);
    opamp.write(desc.resctrl_address, 0xEF);
    opamp.write(desc.muxctrl_address, 0x12);

    CHECK(opamp.read(desc.ctrla_address) == 0xAB);
    CHECK(opamp.read(desc.ctrlb_address) == 0xCD);
    CHECK(opamp.read(desc.resctrl_address) == 0xEF);
    CHECK(opamp.read(desc.muxctrl_address) == 0x12);

    CHECK(opamp.read(desc.ctrla_address + 4) == 0);
}

TEST_CASE("OPAMP: Reset clears registers") {
    auto desc = make_desc();
    Opamp opamp {desc};

    opamp.write(desc.ctrla_address, 0xFF);
    opamp.write(desc.ctrlb_address, 0xFF);
    opamp.write(desc.resctrl_address, 0xFF);
    opamp.write(desc.muxctrl_address, 0xFF);

    opamp.reset();

    CHECK(opamp.read(desc.ctrla_address) == 0x00);
    CHECK(opamp.read(desc.ctrlb_address) == 0x00);
    CHECK(opamp.read(desc.resctrl_address) == 0x00);
    CHECK(opamp.read(desc.muxctrl_address) == 0x00);
}

TEST_CASE("OPAMP: Tick is a no-op") {
    auto desc = make_desc();
    Opamp opamp {desc};

    opamp.write(desc.ctrla_address, 0x01);
    opamp.tick(1000);
    CHECK(opamp.read(desc.ctrla_address) == 0x01);
}

TEST_CASE("OPAMP: Mapped ranges") {
    auto desc = make_desc();
    Opamp opamp {desc};

    auto ranges = opamp.mapped_ranges();
    REQUIRE(ranges.size() == 1);
    CHECK(ranges[0].begin == desc.ctrla_address);
    CHECK(ranges[0].end == desc.ctrla_address + 3);
}
