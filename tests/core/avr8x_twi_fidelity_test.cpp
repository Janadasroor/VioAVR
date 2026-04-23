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

TEST_CASE("AVR8X TWI - Master Smart Mode and Stop") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 twi0_mctrla = 0x08A3;
    const u16 twi0_mctrlb = 0x08A4;
    const u16 twi0_mstatus = 0x08A5;
    const u16 twi0_maddr = 0x08A7;
    const u16 twi0_mdata = 0x08A8;

    // 1. Enable Smart Mode
    bus.write_data(twi0_mctrla, 0x09); // ENABLE=1, SMEN=1
    bus.write_data(twi0_mstatus, 0x01); // IDLE
    bus.write_data(0x08A6, 10); // MBAUD
    
    // 2. Start Read
    bus.write_data(twi0_maddr, 0x85); // Read address 0x42
    bus.tick_peripherals(400);
    
    CHECK((bus.read_data(twi0_mstatus) & 0x80) != 0); // RIF set
    
    // 3. Read MDATA - should automatically trigger next byte in Smart Mode
    bus.read_data(twi0_mdata);
    
    // After reading MDATA, RIF should be clear
    CHECK((bus.read_data(twi0_mstatus) & 0x80) == 0); 
    
    // Tick for second byte
    bus.tick_peripherals(400);
    CHECK((bus.read_data(twi0_mstatus) & 0x80) != 0); // RIF set again
    
    // 4. Send STOP command via MCTRLB
    bus.write_data(twi0_mctrlb, 0x03); // MCMD=STOP
    
    CHECK((bus.read_data(twi0_mstatus) & 0x03) == 0x01); // BUSSTATE = IDLE
    CHECK((bus.read_data(twi0_mstatus) & 0x20) == 0);   // CLKHOLD clear
}

TEST_CASE("AVR8X TWI - Master Repeated Start") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 twi0_mctrla = 0x08A3;
    const u16 twi0_mstatus = 0x08A5;
    const u16 twi0_maddr = 0x08A7;

    bus.write_data(twi0_mctrla, 0x01); // ENABLE=1
    bus.write_data(twi0_mstatus, 0x01); // IDLE
    bus.write_data(0x08A6, 10); // MBAUD
    
    // 1. Write byte to slave address 0x42
    bus.write_data(twi0_maddr, 0x84); // 0x42 << 1 | 0
    bus.tick_peripherals(400);
    CHECK((bus.read_data(twi0_mstatus) & 0x40) != 0); // WIF set (Write Addr Finished)
    CHECK((bus.read_data(twi0_mstatus) & 0x20) != 0); // CLKHOLD set
    
    // 2. Perform Repeated Start for Read from 0x42
    bus.write_data(twi0_maddr, 0x85); // 0x42 << 1 | 1
    
    // Check signals - should be in Start condition immediately
    // SDA should be Low, SCL should be High (at the very beginning of the address phase)
    CHECK(machine.pin_mux().get_state_by_address(0x400, 2).drive_level == false); // SDA Low
    CHECK(machine.pin_mux().get_state_by_address(0x400, 3).drive_level == true);  // SCL High
    
    bus.tick_peripherals(400);
    CHECK((bus.read_data(twi0_mstatus) & 0x80) != 0); // RIF set (Read Addr Finished)
}

TEST_CASE("AVR8X TWI - Slave Address Match via Pins") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 twi0_sctrla = 0x08A9;
    const u16 twi0_saddr = 0x08AC;
    const u16 twi0_sstatus = 0x08AB;

    // 1. Setup TWI0 as Slave
    bus.write_data(twi0_sctrla, 0x01); // ENABLE=1
    bus.write_data(twi0_saddr, 0x42 << 1); // Address 0x42
    
    // 2. Simulate Master driving a START condition on PA2/PA3
    // Pins: PORTA (index 0), SDA=2, SCL=3
    machine.pin_mux().update_pin(0, 2, PinOwner::gpio, true, true, true); // SDA High
    machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, true, true); // SCL High
    bus.tick_peripherals(10);
    
    machine.pin_mux().update_pin(0, 2, PinOwner::gpio, true, false, true); // SDA Low (START)
    bus.tick_peripherals(10);
    
    // 3. Drive address 0x42 (Write) bit by bit
    // full_addr = (0x42 << 1) | 0 = 0x84 = 1000 0100
    u8 full_addr = (0x42 << 1) | 0;
    for (int i = 7; i >= 0; --i) {
        bool bit = (full_addr >> i) & 1;
        // SCL Low
        machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, false, true);
        bus.tick_peripherals(10);
        // SDA set
        machine.pin_mux().update_pin(0, 2, PinOwner::gpio, true, bit, true);
        bus.tick_peripherals(10);
        // SCL High (Sample)
        machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, true, true);
        bus.tick_peripherals(10);
    }
    
    // After 8 bits, TWI0 should match address
    u8 sstatus = bus.read_data(twi0_sstatus);
    CHECK((sstatus & 0x40) != 0); // APIF set
    CHECK((sstatus & 0x01) != 0); // AP (Addressed) set
    CHECK((sstatus & 0x02) == 0); // DIR = 0 (Write)
    
    // Should be in CLKHOLD
    CHECK((bus.read_data(twi0_sstatus) & 0x20) != 0);
}

TEST_CASE("AVR8X TWI - Slave Full Data Reception") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 twi0_sctrla = 0x08A9;
    const u16 twi0_sctrlb = 0x08AA;
    const u16 twi0_sstatus = 0x08AB;
    const u16 twi0_saddr = 0x08AC;
    const u16 twi0_sdata = 0x08AD;

    // 1. Setup Slave
    bus.write_data(twi0_sctrla, 0x41); // ENABLE=1, APIEN=1
    bus.write_data(twi0_saddr, 0x42 << 1);

    // 2. Drive Address matching 0x42 (Write)
    // START
    machine.pin_mux().update_pin(0, 2, PinOwner::gpio, true, true, true);
    machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, true, true);
    bus.tick_peripherals(10);
    machine.pin_mux().update_pin(0, 2, PinOwner::gpio, true, false, true); 
    bus.tick_peripherals(10);

    u8 full_addr = 0x84;
    for (int i = 7; i >= 0; --i) {
        machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, false, true);
        bus.tick_peripherals(10);
        machine.pin_mux().update_pin(0, 2, PinOwner::gpio, true, (full_addr >> i) & 1, true);
        bus.tick_peripherals(10);
        machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, true, true);
        bus.tick_peripherals(10);
    }
    
    CHECK((bus.read_data(twi0_sstatus) & 0x40) != 0); // APIF set

    // 3. Slave Software responds to APIF
    bus.write_data(twi0_sctrlb, 0x02); // SCMD=COMPTRANS, ACKACT=ACK
    
    // 4. Drive 9th SCL pulse (ACK phase)
    machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, false, true);
    bus.tick_peripherals(10);
    // Slave should drive SDA Low for ACK 
    CHECK(machine.pin_mux().get_state_by_address(0x400, 2).drive_level == false);
    
    machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, true, true);
    bus.tick_peripherals(10);

    // 5. Drive Data Byte 0x55
    u8 data = 0x55;
    for (int i = 7; i >= 0; --i) {
        machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, false, true);
        bus.tick_peripherals(10);
        machine.pin_mux().update_pin(0, 2, PinOwner::gpio, true, (data >> i) & 1, true);
        bus.tick_peripherals(10);
        machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, true, true);
        bus.tick_peripherals(10);
    }

    CHECK((bus.read_data(twi0_sstatus) & 0x80) != 0); // DIF set
    CHECK(bus.read_data(twi0_sdata) == 0x55);
}

TEST_CASE("AVR8X TWI - Slave Full Data Transmission") {
    auto device = DeviceCatalog::find("ATmega4809");
    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    const u16 twi0_sctrla = 0x08A9;
    const u16 twi0_sctrlb = 0x08AA;
    const u16 twi0_sstatus = 0x08AB;
    const u16 twi0_saddr = 0x08AC;
    const u16 twi0_sdata = 0x08AD;

    bus.write_data(twi0_sctrla, 0x41); // ENABLE=1, APIEN=1
    bus.write_data(twi0_saddr, 0x42 << 1); 

    // 2. Drive Address matching 0x42 (Read)
    // START
    machine.pin_mux().update_pin(0, 2, PinOwner::gpio, true, true, true);
    machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, true, true);
    bus.tick_peripherals(10);
    machine.pin_mux().update_pin(0, 2, PinOwner::gpio, true, false, true); 
    bus.tick_peripherals(10);

    u8 addr_byte = (0x42 << 1) | 0x01; // 0x85
    for (int i = 7; i >= 0; --i) {
        machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, false, true); // SCL Low
        bus.tick_peripherals(10);
        machine.pin_mux().update_pin(0, 2, PinOwner::gpio, true, (addr_byte >> i) & 1, true); // SDA set
        bus.tick_peripherals(10);
        machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, true, true); // SCL High (Sample)
        bus.tick_peripherals(10);
    }

    CHECK((bus.read_data(twi0_sstatus) & 0x40) != 0); // APIF set
    CHECK((bus.read_data(twi0_sstatus) & 0x02) != 0); // DIR set (Slave transmits)
    
    // 2. ACK the address and load data
    bus.write_data(twi0_sdata, 0xAA); 
    bus.write_data(twi0_sctrlb, 0x02); // COMPTRANS + ACK

    // 3. 9th SCL pulse (ACK phase)
    machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, false, true); 
    bus.tick_peripherals(10);
    CHECK(machine.pin_mux().get_state_by_address(0x400, 2).drive_level == false); // Slave ACKs
    
    machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, true, true);
    bus.tick_peripherals(10);

    // 4. Sample 8 bits from Slave
    u8 rx_data = 0;
    for (int i = 7; i >= 0; --i) {
        machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, false, true); // SCL Low (Slave drives bit)
        bus.tick_peripherals(10);
        
        machine.pin_mux().update_pin(0, 3, PinOwner::gpio, true, true, true); // SCL High (Master samples)
        bus.tick_peripherals(10);
        if (machine.pin_mux().get_state_by_address(0x400, 2).drive_level) {
            rx_data |= (1 << i);
        }
    }

    CHECK(rx_data == 0xAA);
}
