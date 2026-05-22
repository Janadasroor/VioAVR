#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega808.hpp"
#include "vioavr/core/devices/atmega328p.hpp"
#include "vioavr/core/memory_bus.hpp"

namespace {
using namespace vioavr::core;
}

TEST_CASE("ATmega808: Read fuses via mapped memory") {
    auto machine = Machine::create_for_device("ATmega808");
    REQUIRE(machine != nullptr);

    auto& bus = machine->bus();

    // Fuses are mapped at 0x1280 on ATmega808
    // OSCCFG is at offset 0x02, default 0x7E
    u8 osccfg = bus.read_data(0x1282);
    CHECK(osccfg == 0x7E);

    // SYSCFG0 at offset 0x05, default 0xF6
    u8 syscfg0 = bus.read_data(0x1285);
    CHECK(syscfg0 == 0xF6);

    // Read outside fuse range — data space at 0x3C00+ is SRAM
    u8 out_of_range = bus.read_data(0x3EF0);
    CHECK(out_of_range == 0x00); // SRAM default
}

TEST_CASE("ATmega808: Write fuses via mapped memory") {
    auto machine = Machine::create_for_device("ATmega808");
    REQUIRE(machine != nullptr);

    auto& bus = machine->bus();

    // Read default
    CHECK(bus.read_data(0x1280) == 0x00); // WDTCFG

    // Write to fuse address
    bus.write_data(0x1280, 0x5A);

    // Verify readback
    CHECK(bus.read_data(0x1280) == 0x5A);
}

TEST_CASE("ATmega808: Fuse values persist through mapped access") {
    auto machine = Machine::create_for_device("ATmega808");
    REQUIRE(machine != nullptr);

    auto& bus = machine->bus();

    bus.write_data(0x1282, 0x7F); // OSCCFG
    CHECK(bus.read_data(0x1282) == 0x7F);
    CHECK(bus.read_data(0x1280) == 0x00); // WDTCFG unchanged
}

TEST_CASE("ATmega808: Reset restores default fuse values") {
    auto machine = Machine::create_for_device("ATmega808");
    REQUIRE(machine != nullptr);

    auto& bus = machine->bus();

    bus.write_data(0x1285, 0x00); // SYSCFG0
    CHECK(bus.read_data(0x1285) == 0x00);

    machine->reset();

    CHECK(bus.read_data(0x1285) == 0xF6); // Restored to default
}

TEST_CASE("ATmega328P: Read fuses via LPM with BLBSET") {
    auto machine = Machine::create_for_device("ATmega328P");
    REQUIRE(machine != nullptr);

    auto& bus = machine->bus();

    // Set BLBSET in SPMCSR to enable fuse/lockbit read via LPM
    bus.write_data(bus.device().spmcsr_address, 0x08); // BLBSET = bit 3

    // Read fuses via program byte read
    u8 low_fuse = bus.read_program_byte(0x0000);
    CHECK(low_fuse == 0x62);

    u8 lock_bits = bus.read_program_byte(0x0001);
    CHECK(lock_bits == 0xFF);

    u8 ext_fuse = bus.read_program_byte(0x0002);
    CHECK(ext_fuse == 0xFF);

    u8 high_fuse = bus.read_program_byte(0x0003);
    CHECK(high_fuse == 0xD9);
}

TEST_CASE("ATmega328P: Read signature via LPM with SIGRD") {
    auto machine = Machine::create_for_device("ATmega328P");
    REQUIRE(machine != nullptr);

    auto& bus = machine->bus();

    // Set SIGRD in SPMCSR to enable signature read via LPM
    bus.write_data(bus.device().spmcsr_address, 0x20); // SIGRD = bit 5

    // Read signature bytes at word-aligned addresses per real hardware
    u8 sig0 = bus.read_program_byte(0x0000);
    u8 sig1 = bus.read_program_byte(0x0002);
    u8 sig2 = bus.read_program_byte(0x0004);

    CHECK(sig0 == 0x1E);
    CHECK(sig1 == 0x95);
    CHECK(sig2 == 0x0F);
}
