#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/twi8x.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X TWI - Master Address Timing") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 twi0_mctrla = 0x08A3;
    const u16 twi0_mstatus = 0x08A5;
    const u16 twi0_mbaud = 0x08A6;
    const u16 twi0_maddr = 0x08A7;
    
    // 1. Enable TWI Master and set Bus State to IDLE (0x01 in bits 1:0 of MSTATUS)
    bus.write_data(twi0_mctrla, 0x01); // MCTRLA.ENABLE = 1
    // Force Bus Idle
    bus.write_data(twi0_mstatus, 0x01); // MSTATUS.BUSSTATE = IDLE
    
    // 2. Set MBAUD
    bus.write_data(twi0_mbaud, 10);
    
    // 3. Write Address (0x42 << 1 | 0) = 0x84 to initiate Write transmission
    bus.write_data(twi0_maddr, 0x84); 
    
    // Check initial state
    CHECK((bus.read_data(twi0_mstatus) & 0x03) == 0x02); // BUSSTATE = OWNER
    CHECK((bus.read_data(twi0_mstatus) & 0x60) == 0x00); // No WIF/RIF
    
    // Expected duration: 2 * (10 + MBAUD) per bit. 
    // Total 9 bits (Addr + ACK) = 9 * 2 * (10 + 10) = 360 cycles.
    bus.tick_peripherals(300);
    CHECK((bus.read_data(twi0_mstatus) & 0x60) == 0x00);
    
    bus.tick_peripherals(100);
    CHECK((bus.read_data(twi0_mstatus) & 0x40) != 0); // WIF (Write Interrupt Flag)
    CHECK((bus.read_data(twi0_mstatus) & 0x20) != 0); // CLKHOLD
}

TEST_CASE("AVR8X TWI - Slave Address Match") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 twi0_sctrla = 0x08A9;
    const u16 twi0_saddr = 0x08AC;
    const u16 twi0_sstatus = 0x08AB;

    auto* twi = machine.peripherals_of_type<Twi8x>()[0];

    // 1. Enable TWI Slave with address 0x55
    bus.write_data(twi0_saddr, 0x55 << 1);
    bus.write_data(twi0_sctrla, 0x01); // SCTRLA.ENABLE = 1

    // 2. Simulate Master on bus addressing 0x55 (Write)
    twi->inject_bus_address(0x55 << 1 | 0x00);
    
    // Check flags
    u8 status = bus.read_data(twi0_sstatus);
    CHECK((status & 0x40) != 0); // APIF (Addr/Stop Interrupt)
    CHECK((status & 0x01) != 0); // AP (Addressed)
    CHECK((status & 0x02) == 0); // DIR = Write
    CHECK((status & 0x20) != 0); // CLKHOLD

    // 3. Test Masking (Mask bit 0 of address => match 0x54 and 0x55)
    // SADDRMASK is 0x08AE. Bits 7:1 are mask.
    bus.write_data(0x08AE, 0x01 << 1); // Mask out bit 1 of full byte (bit 0 of address)
    
    // Clear status
    bus.write_data(twi0_sstatus, 0xC0); 
    
    // Address 0x54
    twi->inject_bus_address(0x54 << 1 | 0x00);
    CHECK((bus.read_data(twi0_sstatus) & 0x01) != 0); // Still addressed!
}

TEST_CASE("AVR8X TWI - Master Physical Timing") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 port_mux_twispiroutea = 0x05E3;
    const u16 twi0_mctrla = 0x08A3;
    const u16 twi0_mstatus = 0x08A5;
    const u16 twi0_mbaud = 0x08A6;
    const u16 twi0_maddr = 0x08A7;

    // Use default pins (PORTA2/PA3) for TWI0
    bus.write_data(port_mux_twispiroutea, 0x00);

    // Enable Master
    bus.write_data(twi0_mctrla, 0x01); 
    bus.write_data(twi0_mstatus, 0x01); // Force IDLE
    bus.write_data(twi0_mbaud, 10);     // bit_duration = 40
    
    // Start transaction to address 0xA0 (1010000 0) - Write
    // Data stream bit by bit: Start, 1, 0, 1, 0, 0, 0, 0, 0, ACK
    bus.write_data(twi0_maddr, 0xA0);
    
    // Check internal state: should be BUSY/OWNER (0x02)
    CHECK((bus.read_data(twi0_mstatus) & 0x03) == 0x02);

    // 1. Check Start Condition (immediately after write)
    // SCL should be high, SDA should be low (PA3/PA2)
    // PORTA is at 0x400. PIN2=SDA, PIN3=SCL (default)
    // Actually, TWI0 default routing on 4809 is:
    // TWI0: SDA=PA2, SCL=PA3
    CHECK(machine.pin_mux().get_state_by_address(0x400, 2).drive_level == false);
    CHECK(machine.pin_mux().get_state_by_address(0x400, 3).drive_level == true);

    // 2. Tick to first bit (MSB = 1)
    // Bit duration 40. First bit: 0..20 SCL Low, 20..40 SCL High.
    bus.tick_peripherals(10); 
    CHECK(machine.pin_mux().get_state_by_address(0x400, 3).drive_level == false); // SCL Low
    CHECK(machine.pin_mux().get_state_by_address(0x400, 2).drive_level == true);  // SDA High (bit 7 of 0xA0 is 1)

    bus.tick_peripherals(15);
    CHECK(machine.pin_mux().get_state_by_address(0x400, 3).drive_level == true);  // SCL High
    CHECK(machine.pin_mux().get_state_by_address(0x400, 2).drive_level == true);  // SDA High stable

    // 3. Next bit (bit 6 = 0)
    bus.tick_peripherals(20); // total 45
    CHECK(machine.pin_mux().get_state_by_address(0x400, 2).drive_level == false); // SDA Low (bit 6)
    
    // 4. Fast forward to ACK bit (9th bit)
    // Start + 8 bits * 40 = 360 (+ a bit for start logic)
    bus.tick_peripherals(40 * 7); 
    // Now at ACK bit phase. Master should release SDA.
    CHECK(machine.pin_mux().get_state_by_address(0x400, 2).owner == PinOwner::gpio); // Released
}

TEST_CASE("AVR8X TWI - Master Interrupts") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();
    InterruptRequest irq_request;

    const u16 twi0_mctrla = 0x08A3;
    const u16 twi0_mstatus = 0x08A5;
    const u16 twi0_mbaud = 0x08A6;
    const u16 twi0_maddr = 0x08A7;

    // 1. Test Write Interrupt
    bus.write_data(twi0_mctrla, 0x41); // ENABLE=1, WIEN=1
    bus.write_data(twi0_mstatus, 0x01); // Force IDLE
    bus.write_data(twi0_mbaud, 10);
    
    bus.write_data(twi0_maddr, 0x84); // Write address 0x42
    bus.tick_peripherals(400);
    
    CHECK((bus.read_data(twi0_mstatus) & 0x40) != 0); // WIF set
    CHECK(bus.pending_interrupt_request(irq_request) == true);
    CHECK(irq_request.vector_index == 15); // TWIM vector
    
    // 2. Test Read Interrupt
    bus.write_data(twi0_mctrla, 0x81); // ENABLE=1, RIEN=1
    bus.write_data(twi0_mstatus, 0x01); // Reset to IDLE
    bus.write_data(twi0_maddr, 0x85); // Read address 0x42
    
    bus.tick_peripherals(400);
    CHECK((bus.read_data(twi0_mstatus) & 0x80) != 0); // RIF set
    CHECK(bus.pending_interrupt_request(irq_request) == true);
    CHECK(irq_request.vector_index == 15);
}
