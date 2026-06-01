#include "doctest.h"
#include "vioavr/core/ebi.hpp"

using namespace vioavr::core;

static EbiDescriptor make_test_desc() noexcept {
    EbiDescriptor d{};
    d.ctrl_address = 0x10;
    d.sdramctrla_address = 0x11;
    d.refresh_address = 0x12;
    d.initdly_address = 0x14;
    d.sdramctrlb_address = 0x16;
    d.sdramctrlc_address = 0x17;
    d.cs0_ctrla_address = 0x20;
    d.cs0_ctrlb_address = 0x21;
    d.cs0_baseaddr_address = 0x22;
    d.cs1_ctrla_address = 0x24;
    d.cs1_ctrlb_address = 0x25;
    d.cs1_baseaddr_address = 0x26;
    d.cs2_ctrla_address = 0x28;
    d.cs2_ctrlb_address = 0x29;
    d.cs2_baseaddr_address = 0x2A;
    d.cs3_ctrla_address = 0x2C;
    d.cs3_ctrlb_address = 0x2D;
    d.cs3_baseaddr_address = 0x2E;
    return d;
}

TEST_CASE("XMEGA EBI — Register access") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    CHECK(ebi.read(d.ctrl_address) == 0);
    ebi.write(d.ctrl_address, 0x03U);
    CHECK(ebi.read(d.ctrl_address) == 0x03U);

    ebi.write(d.sdramctrla_address, 0x0AU);
    CHECK(ebi.read(d.sdramctrla_address) == 0x0AU);

    ebi.write(d.sdramctrlb_address, 0x01U);
    CHECK(ebi.read(d.sdramctrlb_address) == 0x01U);

    ebi.write(d.sdramctrlc_address, 0x02U);
    CHECK(ebi.read(d.sdramctrlc_address) == 0x02U);
}

TEST_CASE("XMEGA EBI — 16-bit registers (REFRESH, INITDLY, CS0_BASEADDR)") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    ebi.write(d.refresh_address, 0x34U);
    CHECK(ebi.read(d.refresh_address) == 0x34U);
    CHECK(ebi.read(d.refresh_address + 1) == 0x00U);

    ebi.write(d.refresh_address + 1, 0x12U);
    u16 ref = static_cast<u16>(ebi.read(d.refresh_address)) |
              (static_cast<u16>(ebi.read(d.refresh_address + 1)) << 8);
    CHECK(ref == 0x1234U);

    ebi.write(d.initdly_address, 0xCDU);
    ebi.write(d.initdly_address + 1, 0xABU);
    u16 init = static_cast<u16>(ebi.read(d.initdly_address)) |
               (static_cast<u16>(ebi.read(d.initdly_address + 1)) << 8);
    CHECK(init == 0xABCDU);

    ebi.write(d.cs0_baseaddr_address, 0x00U);
    ebi.write(d.cs0_baseaddr_address + 1, 0x80U);
    u16 base = static_cast<u16>(ebi.read(d.cs0_baseaddr_address)) |
               (static_cast<u16>(ebi.read(d.cs0_baseaddr_address + 1)) << 8);
    CHECK(base == 0x8000U);
}

TEST_CASE("XMEGA EBI — CS0 chip select registers") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    ebi.write(d.cs0_ctrla_address, 0x05U);
    CHECK(ebi.read(d.cs0_ctrla_address) == 0x05U);

    ebi.write(d.cs0_ctrlb_address, 0x03U);
    CHECK(ebi.read(d.cs0_ctrlb_address) == 0x03U);

    ebi.write(d.cs0_baseaddr_address, 0x00U);
    ebi.write(d.cs0_baseaddr_address + 1, 0x80U);
    CHECK(ebi.read(d.cs0_baseaddr_address) == 0x00U);
    CHECK(ebi.read(d.cs0_baseaddr_address + 1) == 0x80U);
}

TEST_CASE("XMEGA EBI — SDRAM init delay countdown") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    ebi.write(d.initdly_address, 0x0AU);
    ebi.write(d.initdly_address + 1, 0x00U);

    ebi.write(d.ctrl_address, 0x02U);

    ebi.tick(5);
    ebi.tick(5);
}

TEST_CASE("XMEGA EBI — CS0 range updates on ENABLE + BASEADDR") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    CHECK(!ebi.is_cs0_active());

    // Enable EBI, set CS0 mode and base address
    ebi.write(d.ctrl_address, 0x01U);
    ebi.write(d.cs0_ctrla_address, 0x01U);  // SRAM mode
    ebi.write(d.cs0_baseaddr_address, 0x00U);
    ebi.write(d.cs0_baseaddr_address + 1, 0x80U);

    CHECK(ebi.is_cs0_active());
    CHECK(ebi.cs0_start() == 0x8000U);

    // Changing base address updates range
    ebi.write(d.cs0_baseaddr_address + 1, 0x00U);
    CHECK(ebi.cs0_start() == 0x0000U);
}

TEST_CASE("XMEGA EBI — External memory read/write through EBI") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    // Enable + mode + base = 0x8000
    ebi.write(d.ctrl_address, 0x01U);
    ebi.write(d.cs0_ctrla_address, 0x01U);
    ebi.write(d.cs0_baseaddr_address, 0x00U);
    ebi.write(d.cs0_baseaddr_address + 1, 0x80U);
    CHECK(ebi.is_cs0_active());
    CHECK(ebi.cs0_start() == 0x8000U);

    // Write and read external memory at offset 0x100
    ebi.external_write(0x8100, 0xAAU);
    CHECK(ebi.external_read(0x8100) == 0xAAU);

    // Write and read external memory at offset 0x0000
    ebi.external_write(0x8000, 0x55U);
    CHECK(ebi.external_read(0x8000) == 0x55U);

    // Data is independent (different offsets)
    CHECK(ebi.external_read(0x8100) == 0xAAU);
    CHECK(ebi.external_read(0x8000) == 0x55U);

    // Internal memory buffer preserved values
    CHECK(ebi.memory()[0x0100] == 0xAAU);
    CHECK(ebi.memory()[0x0000] == 0x55U);
}

TEST_CASE("XMEGA EBI — External memory returns 0xFF when disabled") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    // Not enabled
    CHECK(!ebi.is_cs0_active());

    ebi.external_write(0x8000, 0xAAU);
    CHECK(ebi.external_read(0x8000) == 0xFFU);

    // Enable + mode + base = 0x8000
    ebi.write(d.ctrl_address, 0x01U);
    ebi.write(d.cs0_ctrla_address, 0x01U);
    ebi.write(d.cs0_baseaddr_address, 0x00U);
    ebi.write(d.cs0_baseaddr_address + 1, 0x80U);

    ebi.external_write(0x8000, 0xBBU);
    CHECK(ebi.external_read(0x8000) == 0xBBU);

    ebi.write(d.ctrl_address, 0x00U);
    CHECK(ebi.external_read(0x8000) == 0xFFU);
}

TEST_CASE("XMEGA EBI — External memory out-of-range returns 0xFF") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    ebi.write(d.ctrl_address, 0x01U);
    ebi.write(d.cs0_ctrla_address, 0x01U);
    ebi.write(d.cs0_baseaddr_address, 0x00U);
    ebi.write(d.cs0_baseaddr_address + 1, 0x80U);

    // 0x0000 is before CS0 start
    CHECK(ebi.external_read(0x0000) == 0xFFU);

    // 0x7FFF is before CS0 start
    CHECK(ebi.external_read(0x7FFF) == 0xFFU);
}

TEST_CASE("XMEGA EBI — Wait states from CS0_CTRLB") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    ebi.write(d.ctrl_address, 0x01U);
    ebi.write(d.cs0_ctrla_address, 0x01U);
    ebi.write(d.cs0_baseaddr_address, 0x00U);
    ebi.write(d.cs0_baseaddr_address + 1, 0x80U);

    // Default: 0 wait states
    CHECK(ebi.get_wait_states() == 0);

    // Set 3 wait states
    ebi.write(d.cs0_ctrlb_address, 0x03U);
    CHECK(ebi.get_wait_states() == 3);

    // Set 15 wait states
    ebi.write(d.cs0_ctrlb_address, 0x0FU);
    CHECK(ebi.get_wait_states() == 15);

    // Disable EBI → 0 wait states
    ebi.write(d.ctrl_address, 0x00U);
    CHECK(ebi.get_wait_states() == 0);
}

TEST_CASE("XMEGA EBI — CS1-CS3 chip select register access") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    // CS1
    ebi.write(d.cs1_ctrla_address, 0x03U);
    CHECK(ebi.read(d.cs1_ctrla_address) == 0x03U);
    ebi.write(d.cs1_ctrlb_address, 0x05U);
    CHECK(ebi.read(d.cs1_ctrlb_address) == 0x05U);
    ebi.write(d.cs1_baseaddr_address, 0x00U);
    ebi.write(d.cs1_baseaddr_address + 1, 0x40U);
    CHECK(ebi.read(d.cs1_baseaddr_address) == 0x00U);
    CHECK(ebi.read(d.cs1_baseaddr_address + 1) == 0x40U);

    // CS2
    ebi.write(d.cs2_ctrla_address, 0x07U);
    CHECK(ebi.read(d.cs2_ctrla_address) == 0x07U);
    ebi.write(d.cs2_ctrlb_address, 0x09U);
    CHECK(ebi.read(d.cs2_ctrlb_address) == 0x09U);
    ebi.write(d.cs2_baseaddr_address, 0x00U);
    ebi.write(d.cs2_baseaddr_address + 1, 0xC0U);
    CHECK(ebi.read(d.cs2_baseaddr_address) == 0x00U);
    CHECK(ebi.read(d.cs2_baseaddr_address + 1) == 0xC0U);

    // CS3
    ebi.write(d.cs3_ctrla_address, 0x01U);
    CHECK(ebi.read(d.cs3_ctrla_address) == 0x01U);
    ebi.write(d.cs3_ctrlb_address, 0x0FU);
    CHECK(ebi.read(d.cs3_ctrlb_address) == 0x0FU);
    ebi.write(d.cs3_baseaddr_address, 0x00U);
    ebi.write(d.cs3_baseaddr_address + 1, 0xE0U);
    CHECK(ebi.read(d.cs3_baseaddr_address) == 0x00U);
    CHECK(ebi.read(d.cs3_baseaddr_address + 1) == 0xE0U);
}

TEST_CASE("XMEGA EBI — CS1-CS3 range updates on ENABLE + BASEADDR") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    // All CS start disabled
    CHECK(!ebi.is_cs_active(0));
    CHECK(!ebi.is_cs_active(1));
    CHECK(!ebi.is_cs_active(2));
    CHECK(!ebi.is_cs_active(3));

    // Enable EBI and set mode for all CS
    ebi.write(d.ctrl_address, 0x01U);
    ebi.write(d.cs0_ctrla_address, 0x01U);
    ebi.write(d.cs0_baseaddr_address, 0x00U);
    ebi.write(d.cs0_baseaddr_address + 1, 0x80U);
    CHECK(ebi.is_cs_active(0));

    ebi.write(d.cs1_ctrla_address, 0x01U);
    ebi.write(d.cs1_baseaddr_address, 0x00U);
    ebi.write(d.cs1_baseaddr_address + 1, 0x40U);
    CHECK(ebi.is_cs_active(1));

    ebi.write(d.cs2_ctrla_address, 0x01U);
    ebi.write(d.cs2_baseaddr_address, 0x00U);
    ebi.write(d.cs2_baseaddr_address + 1, 0xC0U);
    CHECK(ebi.is_cs_active(2));

    ebi.write(d.cs3_ctrla_address, 0x01U);
    ebi.write(d.cs3_baseaddr_address, 0x00U);
    ebi.write(d.cs3_baseaddr_address + 1, 0xE0U);
    CHECK(ebi.is_cs_active(3));

    // Changing CS0 base address updates its range
    ebi.write(d.cs0_baseaddr_address + 1, 0x00U);
    CHECK(ebi.cs_start(0) == 0x0000U);

    // Other CS ranges unchanged
    CHECK(ebi.cs_start(1) == 0x4000U);
    CHECK(ebi.cs_start(2) == 0xC000U);
    CHECK(ebi.cs_start(3) == 0xE000U);
}

TEST_CASE("XMEGA EBI — Wait states from multiple CS ranges") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);
    ebi.reset();

    ebi.write(d.ctrl_address, 0x01U);
    ebi.write(d.cs0_ctrla_address, 0x01U);
    ebi.write(d.cs0_baseaddr_address, 0x00U);
    ebi.write(d.cs0_baseaddr_address + 1, 0x80U);

    // No wait states initially
    CHECK(ebi.get_wait_states() == 0);

    // Set CS0 wait states to 3
    ebi.write(d.cs0_ctrlb_address, 0x03U);
    CHECK(ebi.get_wait_states() == 3);

    // Set CS1 wait states to 7 — enable CS1 first
    ebi.write(d.cs1_ctrla_address, 0x01U);
    ebi.write(d.cs1_baseaddr_address, 0x00U);
    ebi.write(d.cs1_baseaddr_address + 1, 0x40U);
    ebi.write(d.cs1_ctrlb_address, 0x07U);
    CHECK(ebi.get_wait_states() == 7);

    // Set CS2 wait states to 15 — enable CS2 first
    ebi.write(d.cs2_ctrla_address, 0x01U);
    ebi.write(d.cs2_baseaddr_address, 0x00U);
    ebi.write(d.cs2_baseaddr_address + 1, 0xC0U);
    ebi.write(d.cs2_ctrlb_address, 0x0FU);
    CHECK(ebi.get_wait_states() == 15);

    // CS0 not valid when disabled
    ebi.write(d.ctrl_address, 0x00U);
    CHECK(ebi.get_wait_states() == 0);
}

TEST_CASE("XMEGA EBI — Address range covers all registers") {
    auto d = make_test_desc();
    Ebi ebi("EBI", d);

    auto ranges = ebi.mapped_ranges();
    REQUIRE(!ranges.empty());

    u16 r_begin = ranges[0].begin;
    u16 r_end = ranges[0].end;

    CHECK(d.ctrl_address >= r_begin);
    CHECK(d.ctrl_address <= r_end);
    CHECK(d.sdramctrla_address >= r_begin);
    CHECK(d.sdramctrla_address <= r_end);
    CHECK(d.refresh_address >= r_begin);
    CHECK(d.refresh_address <= r_end);
    CHECK(d.initdly_address >= r_begin);
    CHECK(d.initdly_address <= r_end);
    CHECK(d.sdramctrlb_address >= r_begin);
    CHECK(d.sdramctrlb_address <= r_end);
    CHECK(d.sdramctrlc_address >= r_begin);
    CHECK(d.sdramctrlc_address <= r_end);

    REQUIRE(ranges.size() >= 2);
    u16 cs_begin = ranges[1].begin;
    u16 cs_end = ranges[1].end;
    CHECK(d.cs0_ctrla_address >= cs_begin);
    CHECK(d.cs0_ctrla_address <= cs_end);
    CHECK(d.cs0_ctrlb_address >= cs_begin);
    CHECK(d.cs0_ctrlb_address <= cs_end);
    CHECK(d.cs0_baseaddr_address >= cs_begin);
    CHECK(d.cs0_baseaddr_address <= cs_end);
}
