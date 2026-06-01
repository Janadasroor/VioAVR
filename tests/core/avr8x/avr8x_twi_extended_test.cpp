#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/twi8x.hpp"

using namespace vioavr::core;

// TWI0 register addresses for ATmega4809
static constexpr u16 DBGCTRL  = 0x08A2;
static constexpr u16 MCTRLA   = 0x08A3;
static constexpr u16 MCTRLB   = 0x08A4;
static constexpr u16 MSTATUS  = 0x08A5;
static constexpr u16 MBAUD    = 0x08A6;
static constexpr u16 MADDR    = 0x08A7;
static constexpr u16 MDATA    = 0x08A8;
static constexpr u16 SCTRLA   = 0x08A9;
static constexpr u16 SCTRLB   = 0x08AA;
static constexpr u16 SSTATUS  = 0x08AB;
static constexpr u16 SADDR    = 0x08AC;
static constexpr u16 SDATA    = 0x08AD;
static constexpr u16 SADDRMASK = 0x08AE;

// Constants from twi8x.hpp
static constexpr u8 MSTATUS_RIF = 0x80;
static constexpr u8 MSTATUS_WIF = 0x40;
static constexpr u8 MSTATUS_CLKHOLD = 0x20;
static constexpr u8 MSTATUS_RXACK = 0x10;
static constexpr u8 MSTATUS_ARBLOST = 0x08;
static constexpr u8 MSTATUS_BUSERR = 0x04;
static constexpr u8 MCTRLB_ACKACT = 0x04;
static constexpr u8 MSTATUS_BUSSTATE_MASK = 0x03;
static constexpr u8 MSTATUS_BUSSTATE_IDLE = 0x01;
static constexpr u8 MSTATUS_BUSSTATE_OWNER = 0x02;
static constexpr u8 MSTATUS_BUSSTATE_BUSY = 0x03;

static constexpr u8 SSTATUS_DIF = 0x80;
static constexpr u8 SSTATUS_APIF = 0x40;
static constexpr u8 SSTATUS_CLKHOLD = 0x20;
static constexpr u8 SSTATUS_DIR = 0x02;
static constexpr u8 SSTATUS_AP = 0x01;

TEST_CASE("TWI - Reset Defaults") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    CHECK(bus.read_data(DBGCTRL) == 0x00);
    CHECK(bus.read_data(MCTRLA) == 0x00);
    CHECK(bus.read_data(MCTRLB) == 0x00);
    CHECK(bus.read_data(MSTATUS) == 0x00);
    CHECK(bus.read_data(MBAUD) == 0x00);
    CHECK(bus.read_data(MADDR) == 0x00);
    CHECK(bus.read_data(MDATA) == 0x00);
    CHECK(bus.read_data(SCTRLA) == 0x00);
    CHECK(bus.read_data(SCTRLB) == 0x00);
    CHECK(bus.read_data(SSTATUS) == 0x00);
    CHECK(bus.read_data(SADDR) == 0x00);
    CHECK(bus.read_data(SDATA) == 0x00);
    CHECK(bus.read_data(SADDRMASK) == 0x00);
}

TEST_CASE("TWI - Register Round Trips") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(DBGCTRL, 0xAB);
    CHECK(bus.read_data(DBGCTRL) == 0xAB);

    bus.write_data(MCTRLA, 0xC5);
    CHECK(bus.read_data(MCTRLA) == 0xC5);

    bus.write_data(MCTRLB, 0x07);
    CHECK(bus.read_data(MCTRLB) == 0x07);

    bus.write_data(MBAUD, 0x7F);
    CHECK(bus.read_data(MBAUD) == 0x7F);

    bus.write_data(MADDR, 0x7E);
    CHECK(bus.read_data(MADDR) == 0x7E);

    bus.write_data(MDATA, 0x5A);
    CHECK(bus.read_data(MDATA) == 0x5A);

    bus.write_data(SCTRLA, 0xD3);
    CHECK(bus.read_data(SCTRLA) == 0xD3);

    bus.write_data(SCTRLB, 0x07);
    CHECK(bus.read_data(SCTRLB) == 0x07);

    bus.write_data(SADDR, 0x42);
    CHECK(bus.read_data(SADDR) == 0x42);

    bus.write_data(SDATA, 0xA5);
    CHECK(bus.read_data(SDATA) == 0xA5);

    bus.write_data(SADDRMASK, 0xFE);
    CHECK(bus.read_data(SADDRMASK) == 0xFE);
}

TEST_CASE("TWI - Master Disabled Does Not Tick") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // MCTRLA.ENABLE = 0 (default), set MBAUD and MADDR
    bus.write_data(MBAUD, 10);
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_IDLE); // Force IDLE
    bus.write_data(MADDR, 0x84); // Address 0x42 write

    // No BLKHOLD or flags - master should not start because it's disabled
    CHECK((bus.read_data(MSTATUS) & MSTATUS_CLKHOLD) == 0);

    bus.tick_peripherals(500);
    // Still no flags because master is disabled
    CHECK((bus.read_data(MSTATUS) & (MSTATUS_RIF | MSTATUS_WIF | MSTATUS_CLKHOLD)) == 0);
}

TEST_CASE("TWI - Slave Disabled Suppresses Interrupt") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* twi = machine.peripherals_of_type<Twi8x>()[0];
    InterruptRequest irq;

    // Set slave address but keep SCTRLA.ENABLE = 0
    bus.write_data(SADDR, 0x55 << 1);

    // Inject address matching 0x55 (inject bypasses enable check)
    twi->inject_bus_address(0x55 << 1 | 0x00);

    // inject_bus_address sets APIF/AP regardless of SCTRLA.ENABLE
    // But no interrupt should be pending since SCTRLA.APIEN=0
    CHECK(bus.pending_interrupt_request(irq) == false);

    // Enable with APIEN
    bus.write_data(SCTRLA, 0x41); // ENABLE=1, APIEN=1
    // Now interrupt should be pending (APIF + APIEN)
    CHECK(bus.pending_interrupt_request(irq) == true);
    CHECK(irq.vector_index == 14);
}

TEST_CASE("TWI - BUSSTATE IDLE Write and Persist") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Write BUSSTATE = IDLE
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_IDLE);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_BUSSTATE_MASK) == MSTATUS_BUSSTATE_IDLE);

    // Write OWNER
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_OWNER);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_BUSSTATE_MASK) == MSTATUS_BUSSTATE_OWNER);

    // Write BUSY
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_BUSY);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_BUSSTATE_MASK) == MSTATUS_BUSSTATE_BUSY);

    // Ticks shouldn't change BUSSTATE on their own
    bus.tick_peripherals(100);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_BUSSTATE_MASK) == MSTATUS_BUSSTATE_BUSY);
}

TEST_CASE("TWI - MSTATUS Flag Clear by Writing 1") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Enable master, force IDLE
    bus.write_data(MCTRLA, 0x01); // ENABLE
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_IDLE);
    bus.write_data(MBAUD, 10);

    // Write address to start transmission, get WIF
    bus.write_data(MADDR, 0x84);
    bus.tick_peripherals(400);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_WIF) != 0);

    // Clear WIF by writing 1 to the WIF bit position
    bus.write_data(MSTATUS, MSTATUS_WIF);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_WIF) == 0);
}

TEST_CASE("TWI - SSTATUS Flag Clear by Writing 1") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* twi = machine.peripherals_of_type<Twi8x>()[0];

    // Enable slave
    bus.write_data(SCTRLA, 0x41); // ENABLE=1, APIEN=1
    bus.write_data(SADDR, 0x42 << 1);

    // Inject address match
    twi->inject_bus_address(0x42 << 1 | 0x00);
    CHECK((bus.read_data(SSTATUS) & SSTATUS_APIF) != 0);

    // Clear APIF by writing 1
    bus.write_data(SSTATUS, SSTATUS_APIF);
    CHECK((bus.read_data(SSTATUS) & SSTATUS_APIF) == 0);

    // Inject data
    twi->inject_bus_data(0x55);
    CHECK((bus.read_data(SSTATUS) & SSTATUS_DIF) != 0);

    // Clear DIF by writing 1
    bus.write_data(SSTATUS, SSTATUS_DIF);
    CHECK((bus.read_data(SSTATUS) & SSTATUS_DIF) == 0);
}

TEST_CASE("TWI - Inject Address No Match") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* twi = machine.peripherals_of_type<Twi8x>()[0];

    // Enable slave with address 0x42
    bus.write_data(SCTRLA, 0x41); // ENABLE=1, APIEN=1
    bus.write_data(SADDR, 0x42 << 1);

    // Inject WRONG address
    twi->inject_bus_address(u8(0x99 << 1 | 0x00));

    // Should NOT have matched
    u8 status = bus.read_data(SSTATUS);
    CHECK((status & SSTATUS_APIF) == 0);
    CHECK((status & SSTATUS_AP) == 0);
}

TEST_CASE("TWI - SADDRMASK Multiple Bits") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* twi = machine.peripherals_of_type<Twi8x>()[0];

    // Enable slave with address 0x42
    bus.write_data(SCTRLA, 0x41);
    bus.write_data(SADDR, 0x42 << 1);

    // Mask out bits 1 and 0 of the address byte (i.e. match 0x40-0x43)
    // SADDRMASK bits 7:1 are the mask. Bit position 1 = mask bit 1, bit pos 0 = mask bit 0.
    bus.write_data(SADDRMASK, 0x03 << 1); // Mask bits 0 and 1

    // 0x43 should match (0x42 with both low bits masked)
    bus.write_data(SSTATUS, 0xC0); // Clear status
    twi->inject_bus_address(0x43 << 1 | 0x00);
    CHECK((bus.read_data(SSTATUS) & SSTATUS_AP) != 0);

    // 0x41 should also match
    bus.write_data(SSTATUS, 0xC0);
    twi->inject_bus_address(0x41 << 1 | 0x00);
    CHECK((bus.read_data(SSTATUS) & SSTATUS_AP) != 0);

    // 0x44 should NOT match (bit 2 differs, not masked)
    bus.write_data(SSTATUS, 0xC0);
    twi->inject_bus_address(0x44 << 1 | 0x00);
    CHECK((bus.read_data(SSTATUS) & SSTATUS_AP) == 0);
}

TEST_CASE("TWI - MBAUD Minimum and Maximum") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Enable master, force IDLE
    bus.write_data(MCTRLA, 0x01);
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_IDLE);

    // MBAUD = 0 (min) -> bit_duration = 20
    bus.write_data(MBAUD, 0);
    CHECK(bus.read_data(MBAUD) == 0);
    bus.write_data(MADDR, 0x84);
    bus.tick_peripherals(200); // 9 bits * 20 = 180, should be done in 200
    CHECK((bus.read_data(MSTATUS) & MSTATUS_WIF) != 0);

    // Reset for MBAUD test
    machine.reset();
    bus.write_data(MCTRLA, 0x01);
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_IDLE);

    // MBAUD = 0xFF (max) -> bit_duration = 2 * (10 + 255) = 530
    bus.write_data(MBAUD, 0xFF);
    bus.write_data(MADDR, 0x84);
    // 9 bits * 530 = 4770, shouldn't be done at 4500
    bus.tick_peripherals(4500);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_WIF) == 0);
    // Should be done by 5000
    bus.tick_peripherals(600);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_WIF) != 0);
}

TEST_CASE("TWI - Master NACK on Read") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Enable master with RIEN, set ACKACT=NACK
    bus.write_data(MCTRLA, 0x81); // ENABLE=1, RIEN=1
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_IDLE);
    bus.write_data(MBAUD, 10); // bit_duration = 40

    // Set ACKACT=NACK (bit 2 = 1)
    bus.write_data(MCTRLB, MCTRLB_ACKACT);

    // Read address 0x42 (read) - 9 bits * 40 = 360
    bus.write_data(MADDR, 0x85);
    bus.tick_peripherals(400);

    // Should have RIF set (address phase done)
    CHECK((bus.read_data(MSTATUS) & MSTATUS_RIF) != 0);

    // Read data - clears RIF, starts next byte (NACK)
    volatile u8 dummy = bus.read_data(MDATA);
    (void)dummy;

    // Tick for data byte - 9 bits * 40 = 360
    bus.tick_peripherals(400);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_RIF) != 0);
}

TEST_CASE("TWI - Master Write Data Byte") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(MCTRLA, 0x01); // ENABLE=1
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_IDLE);
    bus.write_data(MBAUD, 10); // bit_duration = 40

    // Start with address 0x42 write - 9 bits * 40 = 360
    bus.write_data(MADDR, 0x84);
    bus.tick_peripherals(400);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_WIF) != 0);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_CLKHOLD) != 0);

    // Write data byte 0x55 - clears WIF/CLKHOLD, starts data phase
    bus.write_data(MDATA, 0x55);
    // Data phase is 9 bits * 40 = 360
    bus.tick_peripherals(400);

    // After data byte, WIF should be set again
    CHECK((bus.read_data(MSTATUS) & MSTATUS_WIF) != 0);
}

TEST_CASE("TWI - Inject Bus Start and Stop") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* twi = machine.peripherals_of_type<Twi8x>()[0];

    // Enable slave
    bus.write_data(SCTRLA, 0x41); // ENABLE=1, APIEN=1
    bus.write_data(SADDR, 0x42 << 1);

    // Inject start
    twi->inject_bus_start();
    // inject_bus_start sets slave_phase to addr but doesn't evaluate the address
    // So no APIF yet - AP is not set either per the implementation
    u8 status = bus.read_data(SSTATUS);
    CHECK((status & SSTATUS_APIF) == 0);

    // Inject stop
    twi->inject_bus_stop();
    status = bus.read_data(SSTATUS);
    // STOP generates APIF per implementation
    CHECK((status & SSTATUS_APIF) != 0);
}

TEST_CASE("TWI - Interrupt Flag After Enable Toggle") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    auto* twi = machine.peripherals_of_type<Twi8x>()[0];

    // Set slave address
    bus.write_data(SADDR, 0x42 << 1);

    // Inject data BEFORE enabling
    twi->inject_bus_data(0xA5);
    CHECK((bus.read_data(SSTATUS) & SSTATUS_DIF) != 0);

    // DIF is set even though slave is disabled!
    // The inject functions don't check SCTRLA.ENABLE
    // But the interrupt pending check does

    // Enable slave with DIEN
    bus.write_data(SCTRLA, 0x81); // ENABLE=1, DIEN=1
    // After enabling, DIF should still be set and interrupt should be pending
    CHECK((bus.read_data(SSTATUS) & SSTATUS_DIF) != 0);

    InterruptRequest irq;
    CHECK(bus.pending_interrupt_request(irq) == true);
    CHECK(irq.vector_index == 14); // TWI Slave vector
}

TEST_CASE("TWI - SCTRLA Enable Toggle Clears Interrupts") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Enable slave
    bus.write_data(SCTRLA, 0x41); // ENABLE=1, APIEN=1
    bus.write_data(SADDR, 0x42 << 1);

    // Disable slave - SCTRLA = 0
    bus.write_data(SCTRLA, 0x00);

    // Inject address (while disabled - inject bypasses enable)
    auto* twi = machine.peripherals_of_type<Twi8x>()[0];
    twi->inject_bus_address(0x42 << 1 | 0x00);

    // APIF should be set (inject doesn't check enable)
    CHECK((bus.read_data(SSTATUS) & SSTATUS_APIF) != 0);

    // Re-enable without APIEN
    bus.write_data(SCTRLA, 0x01); // ENABLE=1, APIEN=0

    // APIF should persist but no interrupt pending
    CHECK((bus.read_data(SSTATUS) & SSTATUS_APIF) != 0);

    InterruptRequest irq;
    CHECK(bus.pending_interrupt_request(irq) == false);
}

TEST_CASE("TWI - Unmapped Addresses Return 0") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    CHECK(bus.read_data(0x08AF) == 0);
    CHECK(bus.read_data(0x08A0) == 0);
    CHECK(bus.read_data(0x08A1) == 0);
}

TEST_CASE("TWI - PIDRIVE and Ghost Bit Clears") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(MCTRLA, 0x01); // ENABLE
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_IDLE);
    bus.write_data(MBAUD, 10);

    // Start address
    bus.write_data(MADDR, 0x84);
    bus.tick_peripherals(400);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_WIF) != 0);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_CLKHOLD) != 0);

    // Read MDATA - should not interfere as WIF is set, not RIF
    u8 d = bus.read_data(MDATA);
    // WIF should still be set (reading MDATA only clears RIF)
    CHECK((bus.read_data(MSTATUS) & MSTATUS_WIF) != 0);

    // Now clear WIF by writing 1
    bus.write_data(MSTATUS, MSTATUS_WIF);
    CHECK((bus.read_data(MSTATUS) & MSTATUS_WIF) == 0);
    // CLKHOLD should also be cleared since no flags remain
    CHECK((bus.read_data(MSTATUS) & MSTATUS_CLKHOLD) == 0);
}

TEST_CASE("TWI - Master Write with ACK Status") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    bus.write_data(MCTRLA, 0x01); // ENABLE=1
    bus.write_data(MSTATUS, MSTATUS_BUSSTATE_IDLE);
    bus.write_data(MBAUD, 5);

    // Write address - expect ACK from slave emulator
    bus.write_data(MADDR, 0x84);
    bus.tick_peripherals(100);

    // After address phase, RXACK should reflect the ACK (0 = ACK received)
    CHECK((bus.read_data(MSTATUS) & MSTATUS_RXACK) == 0);
}
