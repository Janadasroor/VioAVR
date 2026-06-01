#include "doctest.h"
#include "vioavr/core/ptc.hpp"

namespace {
using namespace vioavr::core;

PtcDescriptor make_desc() noexcept {
    PtcDescriptor desc {};
    desc.base_address = 0x130;
    desc.eoc_vector_index = 37;
    desc.wcomp_vector_index = 38;
    return desc;
}

TEST_CASE("PTC: Register defaults after reset") {
    auto desc = make_desc();
    Ptc ptc {desc};
    ptc.reset();

    CHECK(ptc.read(desc.base_address + 0x00) == 0x00); // CTRLA
    CHECK(ptc.read(desc.base_address + 0x01) == 0x00); // CTRLB
    CHECK(ptc.read(desc.base_address + 0x02) == 0x00); // CTRLC
    CHECK(ptc.read(desc.base_address + 0x03) == 0x00); // CTRLD
    CHECK(ptc.read(desc.base_address + 0x04) == 0x00); // EVCTRL
    CHECK(ptc.read(desc.base_address + 0x05) == 0x00); // INTCTRL
    CHECK(ptc.read(desc.base_address + 0x06) == 0x00); // INTFLAGS
    CHECK(ptc.read(desc.base_address + 0x07) == 0x00); // DBGCTRL
    CHECK(ptc.read(desc.base_address + 0x08) == 0x00); // SYNCBUSY
    CHECK(ptc.read(desc.base_address + 0x09) == 0x00); // DATAL
    CHECK(ptc.read(desc.base_address + 0x0A) == 0x00); // DATAH
    CHECK(ptc.read(desc.base_address + 0x0D) == 0x00); // WCOMPCTRL
}

TEST_CASE("PTC: Register write/read") {
    auto desc = make_desc();
    Ptc ptc {desc};

    ptc.write(desc.base_address + 0x01, 0xAB); // CTRLB
    ptc.write(desc.base_address + 0x03, 0x03); // CTRLD

    CHECK(ptc.read(desc.base_address + 0x01) == 0xAB);
    CHECK(ptc.read(desc.base_address + 0x03) == 0x03);
}

TEST_CASE("PTC: Software reset clears registers") {
    auto desc = make_desc();
    Ptc ptc {desc};

    ptc.write(desc.base_address + 0x00, 0x01); // ENABLE
    ptc.write(desc.base_address + 0x01, 0xFF);
    ptc.write(desc.base_address + 0x02, 0x03);

    ptc.write(desc.base_address + 0x00, 0x80); // SWRST

    CHECK(ptc.read(desc.base_address + 0x00) == 0x00);
    CHECK(ptc.read(desc.base_address + 0x01) == 0x00);
    CHECK(ptc.read(desc.base_address + 0x02) == 0x00);
}

TEST_CASE("PTC: Interrupt on conversion complete") {
    auto desc = make_desc();
    Ptc ptc {desc};

    InterruptRequest req;

    // Not pending when disabled
    CHECK_FALSE(ptc.pending_interrupt_request(req));

    // Enable PTC and EOC interrupt
    ptc.write(desc.base_address + 0x05, 0x01); // INTCTRL.EOC = 1
    ptc.write(desc.base_address + 0x00, 0x01); // CTRLA.ENABLE = 1

    // Set a capacitance value and start conversion
    ptc.set_capacitance(0x1234);

    // Tick past conversion time
    ptc.tick(2000);

    // Should have EOC flag
    u8 flags = ptc.read(desc.base_address + 0x06);
    CHECK((flags & 0x01) != 0); // INTFLAGS.EOC

    // Should report pending interrupt
    CHECK(ptc.pending_interrupt_request(req));
    CHECK(req.vector_index == 37);

    // Consume it
    CHECK(ptc.consume_interrupt_request(req));
    CHECK(req.vector_index == 37);
    flags = ptc.read(desc.base_address + 0x06);
    CHECK((flags & 0x01) == 0); // Flag cleared
}

TEST_CASE("PTC: Clear interrupt flags via write-1-to-clear") {
    auto desc = make_desc();
    Ptc ptc {desc};

    ptc.write(desc.base_address + 0x00, 0x01);
    ptc.set_capacitance(0x100);
    ptc.tick(2000);

    u8 flags = ptc.read(desc.base_address + 0x06);
    CHECK((flags & 0x01) != 0); // EOC set

    ptc.write(desc.base_address + 0x06, 0x01); // Write 1 to clear
    flags = ptc.read(desc.base_address + 0x06);
    CHECK((flags & 0x01) == 0); // Cleared
}

TEST_CASE("PTC: Mapped ranges") {
    auto desc = make_desc();
    Ptc ptc {desc};

    auto ranges = ptc.mapped_ranges();
    REQUIRE(ranges.size() == 1);
    CHECK(ranges[0].begin == desc.base_address);
    CHECK(ranges[0].end == desc.base_address + 0x0D);
}

TEST_CASE("PTC: Disable aborts conversion") {
    auto desc = make_desc();
    Ptc ptc {desc};

    ptc.write(desc.base_address + 0x00, 0x01); // Enable
    ptc.set_capacitance(0x100);
    // Tick partway through conversion
    ptc.tick(100);

    // Disable mid-conversion
    ptc.write(desc.base_address + 0x00, 0x00);
    ptc.tick(2000);

    // Should not have EOC flag
    u8 flags = ptc.read(desc.base_address + 0x06);
    CHECK((flags & 0x01) == 0);
}
} // namespace
