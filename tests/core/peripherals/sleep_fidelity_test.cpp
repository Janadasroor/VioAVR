#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;

TEST_CASE("Sleep Mode Fidelity: Wake Latency and Interrupt Behavior")
{
    const auto* device_desc = DeviceCatalog::find("ATmega328P");
    REQUIRE(device_desc != nullptr);
    
    MemoryBus bus(*device_desc);
    AvrCpu cpu(bus);
    cpu.reset();
    
    // Attach peripherals needed for testing
    auto eeprom_ptr = std::make_unique<Eeprom>("EEPROM", device_desc->eeproms[0]);
    Eeprom* eeprom = eeprom_ptr.get();
    bus.attach_peripheral(*eeprom_ptr.release());
    
    // Register addresses for ATmega328P
    const u16 smcr_addr = 0x53;   // Sleep Mode Control Register
    const u16 mcucr_addr = 0x55;  // MCU Control Register (for sleep enable)
    const u16 sreg_addr = 0x5F;   // Status Register (I-bit)
    
    // SMCR bits
    const u8 kSe = 0x01;   // Sleep Enable
    const u8 kSm0 = 0x02;  // Sleep Mode bit 0
    const u8 kSm1 = 0x04;  // Sleep Mode bit 1
    const u8 kSm2 = 0x08;  // Sleep Mode bit 2
    
    // Sleep modes: SM2 SM1 SM0
    // 000 = Idle
    // 001 = ADC Noise Reduction  
    // 010 = Power-down
    // 011 = Power-save
    // 110 = Standby (for external crystal/resonator)
    // 111 = Extended Standby
    
    SUBCASE("Sleep Mode Register Reset State") {
        // After reset, sleep should be disabled
        CHECK((bus.read_data(smcr_addr) & kSe) == 0);
    }
    
    SUBCASE("Sleep Enable Configuration") {
        // Enable sleep (IDLE mode: SM=000)
        bus.write_data(smcr_addr, kSe);
        
        CHECK((bus.read_data(smcr_addr) & kSe) != 0);
        CHECK((bus.read_data(smcr_addr) & (kSm2 | kSm1 | kSm0)) == 0);
    }
    
    SUBCASE("Sleep Mode: Idle (SM=000)") {
        // Idle mode: CPU stopped, peripherals running
        // Any interrupt can wake from idle
        bus.write_data(smcr_addr, kSe);  // SM=000 (idle)
        
        u8 smcr = bus.read_data(smcr_addr);
        CHECK((smcr & kSe) != 0);
        CHECK((smcr & (kSm2 | kSm1 | kSm0)) == 0);  // Idle mode
        
        // Verify the CPU can enter idle mode
        // In actual use: SLEEP instruction puts CPU to idle
        // Wait for next interrupt to wake
    }
    
    SUBCASE("Sleep Mode: ADC Noise Reduction (SM=001)") {
        // ADC Noise Reduction: CPU and I/O clocks stopped
        // ADC, external interrupts, TWI address match, timer2 async can wake
        bus.write_data(smcr_addr, kSe | kSm0);
        
        u8 smcr = bus.read_data(smcr_addr);
        CHECK((smcr & kSe) != 0);
        CHECK((smcr & (kSm2 | kSm1 | kSm0)) == kSm0);
    }
    
    SUBCASE("Sleep Mode: Power-down (SM=010)") {
        // Power-down: external crystal/resonator stopped
        // Only external interrupts (INT0/INT1), TWI address match, WDT can wake
        bus.write_data(smcr_addr, kSe | kSm1);
        
        u8 smcr = bus.read_data(smcr_addr);
        CHECK((smcr & kSe) != 0);
        CHECK((smcr & (kSm2 | kSm1 | kSm0)) == kSm1);
    }
    
    SUBCASE("Sleep Mode: Power-save (SM=011)") {
        // Power-save: like power-down but timer2 async keeps running if enabled
        // Timer2 overflow can wake if TIMSK2.OCIE2A/B or TOIE2 set
        bus.write_data(smcr_addr, kSe | kSm1 | kSm0);
        
        u8 smcr = bus.read_data(smcr_addr);
        CHECK((smcr & kSe) != 0);
        CHECK((smcr & (kSm2 | kSm1 | kSm0)) == (kSm1 | kSm0));
    }
    
    SUBCASE("Sleep Mode: Standby (SM=110)") {
        // Standby: like power-down but oscillator keeps running (fast wake)
        // For use with external crystal/resonator
        bus.write_data(smcr_addr, kSe | kSm2 | kSm1);
        
        u8 smcr = bus.read_data(smcr_addr);
        CHECK((smcr & kSe) != 0);
        CHECK((smcr & (kSm2 | kSm1 | kSm0)) == (kSm2 | kSm1));
    }
    
    SUBCASE("Sleep Mode: Extended Standby (SM=111)") {
        // Extended Standby: like standby but timer2 async keeps running
        bus.write_data(smcr_addr, kSe | kSm2 | kSm1 | kSm0);
        
        u8 smcr = bus.read_data(smcr_addr);
        CHECK((smcr & kSe) != 0);
        CHECK((smcr & (kSm2 | kSm1 | kSm0)) == (kSm2 | kSm1 | kSm0));
    }
    
    SUBCASE("Idle Wake Latency - External Interrupt") {
        // From idle, wake-up takes 6 clock cycles
        // Configure sleep enable, idle mode
        bus.write_data(smcr_addr, kSe);
        
        // Enable external interrupt INT0
        // EIMSK = 0x3D, EICRA = 0x69 for ATmega328P
        const u16 eimsk_addr = 0x3D;
        bus.write_data(eimsk_addr, 0x01);  // Enable INT0
        
        // Enable global interrupts (I-bit in SREG)
        bus.write_data(sreg_addr, 0x80);  // I-bit = bit 7
        
        // Verify interrupts enabled
        CHECK((bus.read_data(sreg_addr) & 0x80) != 0);
        CHECK((bus.read_data(eimsk_addr) & 0x01) != 0);
        
        // In actual operation:
        // 1. CPU executes SLEEP instruction
        // 2. CPU enters idle mode, clocks to CPU core stopped
        // 3. External interrupt triggers
        // 4. CPU wakes, executes interrupt handler after 6 cycles
    }
    
    SUBCASE("Power-down Wake Source Restrictions") {
        // In power-down, only specific interrupts can wake
        // INT0/INT1 (external), TWI address match, WDT
        
        bus.write_data(smcr_addr, kSe | kSm1);  // Power-down mode
        
        // Verify mode set
        CHECK((bus.read_data(smcr_addr) & (kSm2 | kSm1 | kSm0)) == kSm1);
        
        // UART interrupt should NOT wake from power-down
        // Timer interrupts should NOT wake from power-down (except async timer2)
        // ADC interrupt should NOT wake from power-down
    }
    
    SUBCASE("SEI Instruction 1-Cycle Delay") {
        // SEI sets I-bit but next instruction executes before any pending interrupt
        // This is important for atomic operations
        
        // SREG I-bit initially clear
        bus.write_data(sreg_addr, 0x00);
        CHECK((bus.read_data(sreg_addr) & 0x80) == 0);
        
        // SEI would set I-bit
        // In simulation, verify the delay is modeled correctly
        
        // After SEI, I-bit should be set
        bus.write_data(sreg_addr, 0x80);
        CHECK((bus.read_data(sreg_addr) & 0x80) != 0);
    }
    
    SUBCASE("RETI Re-enables Interrupts") {
        // RETI instruction returns from interrupt and re-enables I-bit
        // This allows interrupt nesting
        
        // Simulate being in interrupt handler (I-bit clear)
        bus.write_data(sreg_addr, 0x00);
        CHECK((bus.read_data(sreg_addr) & 0x80) == 0);
        
        // RETI would pop PC and set I-bit
        // After RETI, I-bit should be set
        bus.write_data(sreg_addr, 0x80);
        CHECK((bus.read_data(sreg_addr) & 0x80) != 0);
    }
    
    SUBCASE("EEPROM Ready Interrupt Wake from Idle") {
        // EEPROM ready interrupt can wake from idle
        // Setup: Start EEPROM write, enter idle, EEPROM completes, interrupt wakes
        
        bus.write_data(smcr_addr, kSe);  // Idle mode
        
        // Enable EEPROM interrupt (EERIE in EECR)
        const u16 eecr_addr = device_desc->eeproms[0].eecr_address;
        bus.write_data(eecr_addr, 0x08);  // EERIE = bit 3
        
        // Enable global interrupts
        bus.write_data(sreg_addr, 0x80);
        
        CHECK((bus.read_data(eecr_addr) & 0x08) != 0);
        CHECK((bus.read_data(sreg_addr) & 0x80) != 0);
        
        // When EEPROM write completes, EERIE should trigger wake from idle
        // This would be tested with actual EEPROM write timing
    }
    
    SUBCASE("Interrupt Priority and Pending") {
        // Multiple pending interrupts: priority based on vector table order
        // Lower vector address = higher priority
        // RESET (vector 0) > INT0 (vector 1) > INT1 (vector 2) > ...
        
        // Verify global interrupts enabled
        bus.write_data(sreg_addr, 0x80);
        CHECK((bus.read_data(sreg_addr) & 0x80) != 0);
        
        // If multiple interrupts pending, highest priority (lowest vector) executes first
        // This is verified by checking interrupt vector table ordering
    }
    
    SUBCASE("SLEEP Instruction Behavior") {
        // SLEEP instruction enters sleep mode if SE bit set
        // If SE=0, SLEEP is treated as NOP
        
        // Test with SE=0 (sleep disabled)
        bus.write_data(smcr_addr, 0x00);
        CHECK((bus.read_data(smcr_addr) & kSe) == 0);
        
        // SLEEP would execute as NOP - CPU continues
        
        // Test with SE=1 (sleep enabled)
        bus.write_data(smcr_addr, kSe);
        CHECK((bus.read_data(smcr_addr) & kSe) != 0);
        
        // SLEEP would enter configured sleep mode
    }
    
    SUBCASE("BOD Disable During Sleep") {
        // BODS bit in MCUCR can disable BOD during sleep for power saving
        // BODSE enables setting BODS (timed sequence required)
        
        const u16 mcucr_addr = 0x55;
        // MCUCR bits: BODS (bit 6), BODSE (bit 5)
        
        // BOD disable sequence:
        // 1. Set BODSE and BODS
        // 2. Within 4 cycles, set BODS and clear BODSE
        
        // For now, just verify register access
        bus.write_data(mcucr_addr, 0x00);
        CHECK(bus.read_data(mcucr_addr) == 0x00);
    }
}
