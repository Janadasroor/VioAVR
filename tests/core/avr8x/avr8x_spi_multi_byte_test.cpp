#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/spi8x.hpp"

using namespace vioavr::core;

static constexpr u16 CTRLA = 0x08C0;
static constexpr u16 INTCTRL = 0x08C2;
static constexpr u16 INTFLAGS = 0x08C3;
static constexpr u16 DATA = 0x08C4;

TEST_CASE("AVR8X SPI - Multi-Byte Auto-Transfer") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Master, PRESC=16, Enable
    bus.write_data(CTRLA, 0x23);
    constexpr u32 transfer_cycles = 128; // 8 bits * 16 prescaler

    // Write first byte - starts transfer
    bus.write_data(DATA, 0xAA);
    bus.tick_peripherals(transfer_cycles);

    // First transfer complete
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);

    // Write second byte before reading DATA - should auto-start after current
    bus.write_data(DATA, 0xBB);
    CHECK((bus.read_data(INTFLAGS) & 0x80) == 0); // Cleared by write
    CHECK((bus.read_data(INTFLAGS) & 0x40) == 0); // No WRCOL

    bus.tick_peripherals(transfer_cycles);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);

    // Write third byte
    bus.write_data(DATA, 0xCC);
    bus.tick_peripherals(transfer_cycles);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);
}

TEST_CASE("AVR8X SPI - Write Collision WRCOL") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Master, PRESC=16, Enable
    bus.write_data(CTRLA, 0x23);
    constexpr u32 transfer_cycles = 128;

    // Write first byte - starts transfer, tx_buffer consumed immediately
    bus.write_data(DATA, 0xAA);
    // Write second byte - goes to tx_buffer (full=true, queued for auto-load)
    bus.write_data(DATA, 0xBB);
    // Write third byte while tx_buffer is still full - WRCOL!
    bus.write_data(DATA, 0xCC);
    CHECK((bus.read_data(INTFLAGS) & 0x40) != 0);

    // Complete the first transfer (auto-loads 0xBB)
    bus.tick_peripherals(transfer_cycles);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);
    // WRCOL persists after transfer
    CHECK((bus.read_data(INTFLAGS) & 0x40) != 0);

    // Clear WRCOL by writing 1
    bus.write_data(INTFLAGS, 0x40);
    CHECK((bus.read_data(INTFLAGS) & 0x40) == 0);
}

TEST_CASE("AVR8X SPI - Interrupt Enable") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Master, PRESC=16, Enable
    bus.write_data(CTRLA, 0x23);
    // Enable interrupt
    bus.write_data(INTCTRL, 0x01);

    // Check that no interrupt is pending yet
    auto spi_list = machine.peripherals_of_type<Spi8x>();
    REQUIRE(!spi_list.empty());
    InterruptRequest req;
    CHECK(!spi_list[0]->pending_interrupt_request(req));

    // Write data to start transfer
    bus.write_data(DATA, 0xAA);
    bus.tick_peripherals(128);

    // Interrupt should now be pending
    CHECK(spi_list[0]->pending_interrupt_request(req));
    CHECK(req.vector_index == 12);

    // Consume it
    CHECK(spi_list[0]->consume_interrupt_request(req));
    CHECK(!spi_list[0]->pending_interrupt_request(req));
}

TEST_CASE("AVR8X SPI - Data Read Clears IF") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0x23); // Master, PRESC=16, Enable

    bus.write_data(DATA, 0xAA);
    bus.tick_peripherals(128);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);

    // Reading DATA should clear IF
    u8 rx = bus.read_data(DATA);
    CHECK((bus.read_data(INTFLAGS) & 0x80) == 0);
    (void)rx;
}

TEST_CASE("AVR8X SPI - INTFLAGS Write to Clear IF") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(CTRLA, 0x23);
    bus.write_data(DATA, 0xAA);
    bus.tick_peripherals(128);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);

    // Write 1 to IF bit to clear
    bus.write_data(INTFLAGS, 0x80);
    CHECK((bus.read_data(INTFLAGS) & 0x80) == 0);
}

TEST_CASE("AVR8X SPI - Prescaler 4 Timing") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Master, PRESC=0 (/4), Enable => value = 0x20 | 0x00 | 0x01 = 0x21
    bus.write_data(CTRLA, 0x21);

    // 8 bits * 4 = 32 cycles
    bus.write_data(DATA, 0xAA);
    bus.tick_peripherals(30);
    CHECK((bus.read_data(INTFLAGS) & 0x80) == 0);
    bus.tick_peripherals(5);
    CHECK((bus.read_data(INTFLAGS) & 0x80) != 0);
}
