#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/memory_bus.hpp"

using namespace vioavr::core;

static constexpr u16 CTRLA = 0x1000;
static constexpr u16 CTRLB = 0x1001;
static constexpr u16 STATUS = 0x1002;
static constexpr u16 INTCTRL = 0x1003;
static constexpr u16 INTFLAGS = 0x1004;
static constexpr u16 DATA = 0x1006;
static constexpr u16 ADDR = 0x1008;
static constexpr u16 FLASH_MAP = 0x4000;

TEST_CASE("AVR8X NVMCTRL - Page Erase") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    // Write 0xDEAD to flash word 0 via page buffer + page write
    bus.write_data(FLASH_MAP, 0xAD);
    bus.write_data(FLASH_MAP + 1, 0xDE);
    bus.write_data(ADDR, 0x00);
    bus.write_data(ADDR + 1, 0x00);
    bus.write_data(CTRLA, 0x01);
    bus.tick_peripherals(70000);
    CHECK(bus.flash_words()[0] == 0xDEAD);

    // Erase via PAGEERASE
    bus.write_data(CTRLA, 0x02);
    CHECK((bus.read_data(STATUS) & 0x01) != 0);
    bus.tick_peripherals(70000);
    CHECK((bus.read_data(STATUS) & 0x01) == 0);
    CHECK(bus.flash_words()[0] == 0xFFFF);
}

TEST_CASE("AVR8X NVMCTRL - Page Erase Write") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    bus.write_data(FLASH_MAP, 0x42);
    bus.write_data(FLASH_MAP + 1, 0x24);
    bus.write_data(ADDR, 0x00);
    bus.write_data(ADDR + 1, 0x00);

    bus.write_data(CTRLA, 0x03);
    CHECK((bus.read_data(STATUS) & 0x01) != 0);
    bus.tick_peripherals(70000);
    CHECK((bus.read_data(STATUS) & 0x01) == 0);
    CHECK(bus.flash_words()[0] == 0x2442);
}

TEST_CASE("AVR8X NVMCTRL - Chip Erase") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    // Write 0xDEAD to flash word 0
    bus.write_data(FLASH_MAP, 0xAD);
    bus.write_data(FLASH_MAP + 1, 0xDE);
    bus.write_data(ADDR, 0x00);
    bus.write_data(ADDR + 1, 0x00);
    bus.write_data(CTRLA, 0x01);
    bus.tick_peripherals(70000);
    CHECK(bus.flash_words()[0] == 0xDEAD);

    // Chip erase
    bus.write_data(CTRLA, 0x05);
    CHECK((bus.read_data(STATUS) & 0x01) != 0);
    bus.tick_peripherals(1700000);
    CHECK((bus.read_data(STATUS) & 0x01) == 0);
    CHECK(bus.flash_words()[0] == 0xFFFF);
    CHECK(bus.flash_words()[127] == 0xFFFF);
}

TEST_CASE("AVR8X NVMCTRL - Interrupt on Completion") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    // Enable NVM ready interrupt
    bus.write_data(INTCTRL, 0x01);

    bus.write_data(CTRLA, 0x02);
    CHECK((bus.read_data(INTFLAGS) & 0x01) == 0);

    bus.tick_peripherals(70000);

    CHECK((bus.read_data(INTFLAGS) & 0x01) != 0);

    // Clear by writing 1
    bus.write_data(INTFLAGS, 0x01);
    CHECK((bus.read_data(INTFLAGS) & 0x01) == 0);
}

TEST_CASE("AVR8X NVMCTRL - Data Register") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    bus.write_data(DATA, 0x34);
    CHECK(bus.read_data(DATA) == 0x34);
    CHECK(bus.read_data(DATA + 1) == 0x00);

    bus.write_data(DATA + 1, 0x12);
    CHECK(bus.read_data(DATA) == 0x34);
    CHECK(bus.read_data(DATA + 1) == 0x12);
}

TEST_CASE("AVR8X NVMCTRL - CTRLB Wait States") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    machine.reset();
    auto& bus = machine.bus();

    CHECK(bus.flash_wait_states() == 0);

    bus.write_data(CTRLB, 0x03);
    CHECK(bus.flash_wait_states() == 3);

    bus.write_data(CTRLB, 0x01);
    CHECK(bus.flash_wait_states() == 1);
}
