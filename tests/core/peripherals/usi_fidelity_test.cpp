#include "doctest.h"
#include "vioavr/core/usi.hpp"

namespace {
using namespace vioavr::core;
}

UsiDescriptor make_desc() noexcept {
    UsiDescriptor desc{};
    desc.usicr_address = 0x2D;
    desc.usisr_address = 0x2E;
    desc.usidr_address = 0x2F;
    desc.usibr_address = 0x30;
    desc.start_vector_index = 13;
    desc.overflow_vector_index = 14;
    desc.sda_pin_address = 0x38;
    desc.sda_pin_bit = 0;
    desc.scl_pin_address = 0x38;
    desc.scl_pin_bit = 2;
    desc.do_pin_address = 0x38;
    desc.do_pin_bit = 1;
    return desc;
}

TEST_CASE("USI: Register defaults after reset") {
    auto desc = make_desc();
    Usi usi{"USI", desc};
    usi.reset();

    CHECK(usi.read(desc.usicr_address) == 0x00);
    CHECK(usi.read(desc.usisr_address) == 0x00);
    CHECK(usi.read(desc.usidr_address) == 0x00);
    CHECK(usi.read(desc.usibr_address) == 0x00);
}

TEST_CASE("USI: Register write/read") {
    auto desc = make_desc();
    Usi usi{"USI", desc};

    usi.write(desc.usicr_address, 0xC0); // USISIE + USIOIE
    usi.write(desc.usidr_address, 0xAB);
    usi.write(desc.usisr_address, 0x05); // USICNT = 5

    CHECK(usi.read(desc.usicr_address) == 0xC0);
    CHECK(usi.read(desc.usidr_address) == 0xAB);
    CHECK(usi.read(desc.usisr_address) == 0x05);
}

TEST_CASE("USI: Software reset clears registers") {
    auto desc = make_desc();
    Usi usi{"USI", desc};

    usi.write(desc.usicr_address, 0xC0);
    usi.write(desc.usidr_address, 0xFF);
    usi.write(desc.usisr_address, 0x0F);

    usi.reset();

    CHECK(usi.read(desc.usicr_address) == 0x00);
    CHECK(usi.read(desc.usidr_address) == 0x00);
    CHECK(usi.read(desc.usisr_address) == 0x00);
}

TEST_CASE("USI: USIBR is read-only") {
    auto desc = make_desc();
    Usi usi{"USI", desc};

    usi.write(desc.usibr_address, 0xAB);
    CHECK(usi.read(desc.usibr_address) == 0x00);
}

TEST_CASE("USI: Counter overflow sets OVF flag") {
    auto desc = make_desc();
    Usi usi{"USI", desc};

    usi.write(desc.usicr_address, 0xC4); // USISIE|USIOIE, software clock (USICS=11)
    usi.write(desc.usisr_address, 0x0E); // USICNT = 14

    // Two clock strobes to overflow (14 -> 15 -> overflow)
    usi.write(desc.usicr_address, 0xC6); // USICLK = 1
    usi.write(desc.usicr_address, 0xC6); // second strobe

    u8 sr = usi.read(desc.usisr_address);
    CHECK((sr & 0x40) != 0); // USIOIF set
    CHECK((sr & 0x0F) == 0x00); // Counter wrapped to 0
}

TEST_CASE("USI: Interrupt on counter overflow") {
    auto desc = make_desc();
    Usi usi{"USI", desc};

    InterruptRequest req;
    CHECK_FALSE(usi.pending_interrupt_request(req));
    CHECK_FALSE(usi.consume_interrupt_request(req));

    usi.write(desc.usicr_address, 0x44); // USIOIE, USICS=11 (software)
    usi.write(desc.usisr_address, 0x0F); // USICNT = 15

    // Clock strobe to overflow
    usi.write(desc.usicr_address, 0x46); // USICLK = 1

    CHECK(usi.pending_interrupt_request(req));
    CHECK(req.vector_index == 14);

    CHECK(usi.consume_interrupt_request(req));
    CHECK(req.vector_index == 14);

    // Flag should be cleared
    u8 sr = usi.read(desc.usisr_address);
    CHECK((sr & 0x40) == 0);
}

TEST_CASE("USI: Write-1-to-clear interrupt flags") {
    auto desc = make_desc();
    Usi usi{"USI", desc};

    usi.write(desc.usicr_address, 0x44); // USIOIE
    usi.write(desc.usisr_address, 0x0F); // USICNT = 15
    usi.write(desc.usicr_address, 0x46); // Clock strobe -> overflow

    u8 sr = usi.read(desc.usisr_address);
    CHECK((sr & 0x40) != 0); // USIOIF set

    // Write 1 to clear USIOIF
    usi.write(desc.usisr_address, 0x40);
    sr = usi.read(desc.usisr_address);
    CHECK((sr & 0x40) == 0); // Cleared
    CHECK((sr & 0x0F) == 0x00); // Counter wrapped to 0 after overflow
}

TEST_CASE("USI: Data shift on clock strobe") {
    auto desc = make_desc();
    Usi usi{"USI", desc};

    usi.write(desc.usidr_address, 0x80); // MSB = 1
    usi.write(desc.usicr_address, 0x44); // USIOIE, USICS=11 (software)
    usi.write(desc.usisr_address, 0x00); // USICNT = 0

    // Clock strobe
    usi.write(desc.usicr_address, 0x46); // USICLK = 1

    // Data should have shifted left by 1 (0x80 -> 0x00, MSB shifted out)
    CHECK(usi.read(desc.usidr_address) == 0x00);
    CHECK((usi.read(desc.usisr_address) & 0x0F) == 0x01); // Counter incremented
}

TEST_CASE("USI: Mapped ranges") {
    auto desc = make_desc();
    Usi usi{"USI", desc};

    auto ranges = usi.mapped_ranges();
    REQUIRE(ranges.size() == 4);
    CHECK(ranges[0].begin == desc.usicr_address);
    CHECK(ranges[1].begin == desc.usisr_address);
    CHECK(ranges[2].begin == desc.usidr_address);
    CHECK(ranges[3].begin == desc.usibr_address);
}

TEST_CASE("USI: No ranges with zero addresses") {
    UsiDescriptor empty_desc{};
    Usi usi{"USI", empty_desc};
    auto ranges = usi.mapped_ranges();
    CHECK(ranges.size() == 0);
}

TEST_CASE("USI: Empty descriptor no-op") {
    UsiDescriptor empty_desc{};
    Usi usi{"USI", empty_desc};
    usi.reset();
    usi.tick(1000);
    // No crash
}
