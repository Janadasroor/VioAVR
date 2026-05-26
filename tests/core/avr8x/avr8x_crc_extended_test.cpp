#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"

using namespace vioavr::core;

static constexpr u16 CTRLA = 0x0120;
static constexpr u16 CTRLB = 0x0121;
static constexpr u16 STATUS = 0x0122;
static constexpr u16 DATA = 0x0123;
static constexpr u16 CHECKSUML = 0x0124;
static constexpr u16 CHECKSUMH = 0x0125;

TEST_CASE("AVR8X CRC - Register Readback") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    CHECK(bus.read_data(CTRLA) == 0x00);
    CHECK(bus.read_data(CTRLB) == 0x00);
    CHECK(bus.read_data(STATUS) == 0x00);
    CHECK(bus.read_data(DATA) == 0x00);
    CHECK(bus.read_data(CHECKSUML) == 0x00);
    CHECK(bus.read_data(CHECKSUMH) == 0x00);
}

TEST_CASE("AVR8X CRC - CTRLA Bit Masking") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    // Only bits 0 (ENABLE), 1 (NMIEN), 7 (RESET) should be writable
    bus.write_data(CTRLA, 0xFF);
    CHECK((bus.read_data(CTRLA) & 0x83) == 0x83); // bits 0,1,7 set
    CHECK((bus.read_data(CTRLA) & 0x7C) == 0x00); // bits 2-6 masked

    // Clear and write individual bits
    bus.write_data(CTRLA, 0x00);
    CHECK(bus.read_data(CTRLA) == 0x00);

    bus.write_data(CTRLA, 0x01); // ENABLE only
    CHECK(bus.read_data(CTRLA) == 0x01);

    bus.write_data(CTRLA, 0x02); // NMIEN only
    CHECK(bus.read_data(CTRLA) == 0x02);
}

TEST_CASE("AVR8X CRC - CTRLB SRC and MODE Bits") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    // CTRLB should only store SRC (bits 0-1) and MODE (bits 4-5)
    bus.write_data(CTRLB, 0x33); // bits 0-1 and 4-5 set
    CHECK((bus.read_data(CTRLB) & 0x33) == 0x33);
    CHECK((bus.read_data(CTRLB) & 0xCC) == 0x00); // bits 2-3, 6-7 masked

    // Write individual SRC values
    bus.write_data(CTRLB, 0x00);
    CHECK((bus.read_data(CTRLB) & 0x03) == 0x00);
    bus.write_data(CTRLB, 0x01); // SRC = 1
    CHECK((bus.read_data(CTRLB) & 0x03) == 0x01);
    bus.write_data(CTRLB, 0x02); // SRC = 2
    CHECK((bus.read_data(CTRLB) & 0x03) == 0x02);
    bus.write_data(CTRLB, 0x03); // SRC = 3
    CHECK((bus.read_data(CTRLB) & 0x03) == 0x03);
}

TEST_CASE("AVR8X CRC - DATA Register Write/Readback") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    bus.write_data(DATA, 0x42);
    CHECK(bus.read_data(DATA) == 0x42);

    bus.write_data(DATA, 0xA5);
    CHECK(bus.read_data(DATA) == 0xA5);
}

TEST_CASE("AVR8X CRC - Checksum After Scan") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    REQUIRE(device->flash_words > 0);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    // Load known flash content
    std::vector<u16> flash(8, 0x0000);
    flash[0] = 0x1234;
    flash[1] = 0x5678;
    bus.load_flash(flash);

    bus.write_data(CTRLA, 0x01);
    CHECK((bus.read_data(STATUS) & 0x01) != 0);
    CHECK((bus.read_data(STATUS) & 0x02) == 0);

    // CRC scan uses flash_words size for timing
    bus.tick_peripherals(device->flash_words + 1);

    CHECK((bus.read_data(STATUS) & 0x01) == 0);
    CHECK((bus.read_data(STATUS) & 0x02) != 0);

    u16 cs = bus.read_data(CHECKSUML) | (bus.read_data(CHECKSUMH) << 8);
    CHECK(cs != 0);
}

TEST_CASE("AVR8X CRC - CRC16 Algorithm Verification") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    REQUIRE(device->flash_words > 0);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    std::vector<u16> flash(1, 0x0000);
    bus.load_flash(flash);
    bus.write_data(CTRLA, 0x01);
    bus.tick_peripherals(device->flash_words + 1);
    u16 cs1 = bus.read_data(CHECKSUML) | (bus.read_data(CHECKSUMH) << 8);

    flash[0] = 0x0001;
    bus.load_flash(flash);
    bus.write_data(CTRLA, 0x01);
    bus.tick_peripherals(device->flash_words + 1);
    u16 cs2 = bus.read_data(CHECKSUML) | (bus.read_data(CHECKSUMH) << 8);

    CHECK(cs1 != cs2);
}

TEST_CASE("AVR8X CRC - Consecutive Scans") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    REQUIRE(device->flash_words > 0);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    std::vector<u16> flash(8, 0xABCD);
    bus.load_flash(flash);

    bus.write_data(CTRLA, 0x01);
    bus.tick_peripherals(device->flash_words + 1);
    CHECK((bus.read_data(STATUS) & 0x02) != 0);
    u16 cs1 = bus.read_data(CHECKSUML) | (bus.read_data(CHECKSUMH) << 8);

    flash[0] = 0xDEAD;
    bus.load_flash(flash);
    bus.write_data(CTRLA, 0x01);
    bus.tick_peripherals(device->flash_words + 1);
    CHECK((bus.read_data(STATUS) & 0x02) != 0);
    u16 cs2 = bus.read_data(CHECKSUML) | (bus.read_data(CHECKSUMH) << 8);

    CHECK(cs1 != cs2);
}

TEST_CASE("AVR8X CRC - Status OK Cleared On New Scan") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    REQUIRE(device->flash_words > 0);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    std::vector<u16> flash(4, 0x0000);
    bus.load_flash(flash);

    bus.write_data(CTRLA, 0x01);
    bus.tick_peripherals(device->flash_words + 1);
    CHECK((bus.read_data(STATUS) & 0x02) != 0);

    bus.write_data(CTRLA, 0x01);
    CHECK((bus.read_data(STATUS) & 0x02) == 0);
    CHECK((bus.read_data(STATUS) & 0x01) != 0);
}

TEST_CASE("AVR8X CRC - Flash Checksum Determinism") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    REQUIRE(device->flash_words > 0);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    std::vector<u16> flash(16, 0x0000);
    for (u16 i = 0; i < 16; i++) flash[i] = i;
    bus.load_flash(flash);

    bus.write_data(CTRLA, 0x01);
    bus.tick_peripherals(device->flash_words + 1);
    u16 cs1 = bus.read_data(CHECKSUML) | (bus.read_data(CHECKSUMH) << 8);

    bus.write_data(CTRLA, 0x01);
    bus.tick_peripherals(device->flash_words + 1);
    u16 cs2 = bus.read_data(CHECKSUML) | (bus.read_data(CHECKSUMH) << 8);

    CHECK(cs1 == cs2);
}
