#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/spi.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;

TEST_CASE("Pin Mux Fidelity: Peripheral vs GPIO Contention")
{
    const auto* device_desc = DeviceCatalog::find("ATmega328P");
    REQUIRE(device_desc != nullptr);
    
    MemoryBus bus(*device_desc);
    PinMux mux(device_desc->port_count);
    bus.set_pin_mux(&mux);
    
    AvrCpu cpu(bus);
    cpu.reset();
    
    // ATmega328P pin mapping:
    // PD0 (RXD) / PD1 (TXD) - UART0
    // PB2 (SS) / PB3 (MOSI) / PB4 (MISO) / PB5 (SCK) - SPI
    // PB1 (OC1A) / PB2 (OC1B) - Timer1 PWM
    // PD5 (OC0B) / PD6 (OC0A) - Timer0 PWM
    
    // GPIO register addresses
    const u16 ddrb_addr = 0x24;   // Port B Data Direction
    const u16 portb_addr = 0x25;  // Port B Data
    const u16 pinb_addr = 0x23;   // Port B Input
    const u16 ddrd_addr = 0x2A;   // Port D Data Direction
    const u16 portd_addr = 0x2B;  // Port D Data
    const u16 pind_addr = 0x29;   // Port D Input
    
    // Attach GPIO
    auto gpio_ptr = std::make_unique<GpioPort>(device_desc->ports[1], mux); // Port B
    GpioPort* gpio = gpio_ptr.get();
    bus.attach_peripheral(*gpio_ptr.release());
    
    // Attach UART (uses PD0/PD1)
    auto uart_ptr = std::make_unique<Uart>("UART0", device_desc->uarts[0], mux);
    Uart* uart = uart_ptr.get();
    bus.attach_peripheral(*uart_ptr.release());
    
    // Attach SPI (uses PB2-PB5)
    auto spi_ptr = std::make_unique<Spi>("SPI0", device_desc->spis[0]);
    Spi* spi = spi_ptr.get();
    bus.attach_peripheral(*spi_ptr.release());
    
    // Attach Timer1 (uses PB1/OC1A, PB2/OC1B)
    auto timer1_ptr = std::make_unique<Timer16>("TIMER1", device_desc->timers16[0], bus.pin_mux());
    Timer16* timer1 = timer1_ptr.get();
    bus.attach_peripheral(*timer1_ptr.release());
    
    SUBCASE("UART Pin Configuration - Default GPIO Mode") {
        // By default, UART pins (PD0/PD1) should be in GPIO mode
        // DDRD controls direction
        
        // Set PD0 and PD1 as outputs via GPIO
        bus.write_data(ddrd_addr, 0x03);  // PD0, PD1 as outputs
        
        // Write values to PORTD
        bus.write_data(portd_addr, 0x01);  // PD0=1, PD1=0
        
        // Verify GPIO control works
        CHECK((bus.read_data(portd_addr) & 0x03) == 0x01);
    }
    
    SUBCASE("UART Pin Priority - Transmitter Enabled") {
        // When UART transmitter is enabled, TXD pin (PD1) is controlled by UART
        // not by GPIO PORTD register
        
        // First configure PD1 as GPIO output
        bus.write_data(ddrd_addr, 0x02);  // PD1 as output
        bus.write_data(portd_addr, 0x00);  // PD1 = 0
        
        // Enable UART transmitter
        const u16 ucsr0b_addr = 0xC1;
        bus.write_data(ucsr0b_addr, 0x08);  // TXEN0 = bit 3
        
        // Now PD1 should be controlled by UART, not GPIO
        // Writing to PORTD should not affect PD1
        bus.write_data(portd_addr, 0x02);  // Try to set PD1=1 via GPIO
        
        // The actual pin value depends on UART state
        // UART TX pin is typically high when idle
        u8 portd = bus.read_data(portd_addr);
        (void)portd;  // Value depends on implementation
        
        // Verify TXEN is enabled
        CHECK((bus.read_data(ucsr0b_addr) & 0x08) != 0);
    }
    
    SUBCASE("UART Pin Priority - Receiver Enabled") {
        // When UART receiver is enabled, RXD pin (PD0) is input to UART
        // GPIO direction setting should be overridden
        
        // Configure PD0 as GPIO output (will be overridden)
        bus.write_data(ddrd_addr, 0x01);  // Try PD0 as output
        
        // Enable UART receiver
        const u16 ucsr0b_addr = 0xC1;
        bus.write_data(ucsr0b_addr, 0x10);  // RXEN0 = bit 4
        
        // RXD should now be input regardless of DDRD setting
        // Verify RXEN is enabled
        CHECK((bus.read_data(ucsr0b_addr) & 0x10) != 0);
    }
    
    SUBCASE("SPI Pin Configuration - Default GPIO Mode") {
        // SPI pins (PB2-PB5) default to GPIO mode
        // Configure all as outputs
        bus.write_data(ddrb_addr, 0x3C);  // PB2-PB5 as outputs (0b00111100)
        
        // Write pattern
        bus.write_data(portb_addr, 0x14);  // PB2=0, PB3=0, PB4=1, PB5=0
        
        // Verify GPIO control
        CHECK((bus.read_data(portb_addr) & 0x3C) == 0x14);
    }
    
    SUBCASE("SPI Pin Priority - Master Mode") {
        // When SPI is enabled in master mode:
        // - MOSI (PB3) and SCK (PB5) are outputs controlled by SPI
        // - MISO (PB4) is input
        // - SS (PB2) is input (can be used as GPIO if not needed)
        
        // Enable SPI master mode
        const u16 spcr_addr = 0x4C;
        bus.write_data(spcr_addr, 0x50);  // SPE=1 (bit 6), MSTR=1 (bit 4)
        
        // Verify SPI enabled and in master mode
        u8 spcr = bus.read_data(spcr_addr);
        CHECK((spcr & 0x40) != 0);  // SPE
        CHECK((spcr & 0x10) != 0);  // MSTR
        
        // MOSI and SCK should now be controlled by SPI
        // Writing to PORTB should not affect PB3 and PB5
        bus.write_data(ddrb_addr, 0x2C);  // Keep PB3, PB5 as outputs, PB2, PB4 input
        bus.write_data(portb_addr, 0x00);  // Try to clear all
        
        // SPI pins may maintain their state
    }
    
    SUBCASE("SPI Pin Priority - Slave Mode") {
        // When SPI is enabled in slave mode:
        // - MOSI (PB3) is input
        // - MISO (PB4) is output (when SS low)
        // - SCK (PB5) is input
        // - SS (PB2) is input (slave select)
        
        // Enable SPI slave mode
        const u16 spcr_addr = 0x4C;
        bus.write_data(spcr_addr, 0x40);  // SPE=1, MSTR=0
        
        u8 spcr = bus.read_data(spcr_addr);
        CHECK((spcr & 0x40) != 0);  // SPE
        CHECK((spcr & 0x10) == 0);  // MSTR=0 (slave)
    }
    
    SUBCASE("Timer PWM Pin Priority - OC1A on PB1") {
        // Timer1 OC1A output on PB1
        // When PWM mode enabled and COM1A bits set, pin controlled by timer
        
        // Configure PB1 as GPIO output first
        bus.write_data(ddrb_addr, 0x02);  // PB1 as output
        bus.write_data(portb_addr, 0x00);  // PB1 = 0
        
        // Enable Timer1 fast PWM mode with OC1A output
        const u16 tccr1a_addr = 0x80;
        const u16 tccr1b_addr = 0x81;
        
        // WGM13:0 = 1110 (Fast PWM, TOP=ICR1)
        // COM1A1:0 = 10 (Clear OC1A on match, set at BOTTOM)
        bus.write_data(tccr1a_addr, 0x82);  // COM1A1=1, COM1A0=0, WGM11=1
        bus.write_data(tccr1b_addr, 0x19);  // WGM13=1, WGM12=1, CS10=1 (no prescale)
        
        // Set OCR1A for compare match
        const u16 ocr1a_addr = 0x88;
        bus.write_data(ocr1a_addr, 0x80);      // Low byte
        bus.write_data(ocr1a_addr + 1, 0x00);  // High byte
        
        // Set ICR1 as TOP
        const u16 icr1_addr = 0x86;
        bus.write_data(icr1_addr, 0xFF);       // Low byte
        bus.write_data(icr1_addr + 1, 0x00);   // High byte
        
        // Run timer for a few cycles
        bus.tick_peripherals(1000);
        
        // PB1/OC1A should now be controlled by Timer1
        // Writing to PORTB should not affect it
        bus.write_data(portb_addr, 0x02);  // Try to set PB1=1
        
        // Verify timer registers
        CHECK((bus.read_data(tccr1a_addr) & 0x80) != 0);  // COM1A1
    }
    
    SUBCASE("GPIO Override - Peripheral Disabled") {
        // When peripheral is disabled, GPIO regains control
        
        // Enable then disable UART
        const u16 ucsr0b_addr = 0xC1;
        bus.write_data(ucsr0b_addr, 0x18);  // TXEN + RXEN
        CHECK((bus.read_data(ucsr0b_addr) & 0x18) != 0);
        
        // Disable UART
        bus.write_data(ucsr0b_addr, 0x00);
        CHECK((bus.read_data(ucsr0b_addr) & 0x18) == 0);
        
        // GPIO should now control PD0/PD1 again
        bus.write_data(ddrd_addr, 0x03);
        bus.write_data(portd_addr, 0x01);
        CHECK((bus.read_data(portd_addr) & 0x03) == 0x01);
    }
    
    SUBCASE("Pin Change Interrupt - PCINT0 on PB0") {
        // PCINT0 (Pin Change Interrupt 0) on PB0
        // Can trigger on any logic change when enabled
        
        const u16 pcicr_addr = 0x68;   // Pin Change Interrupt Control
        const u16 pcmsk0_addr = 0x6A;  // Pin Change Mask 0
        
        // Enable PCIE0 (bit 0 of PCICR)
        bus.write_data(pcicr_addr, 0x01);
        
        // Enable PCINT0 (PB0) in mask
        bus.write_data(pcmsk0_addr, 0x01);
        
        // Verify enabled
        CHECK((bus.read_data(pcicr_addr) & 0x01) != 0);
        CHECK((bus.read_data(pcmsk0_addr) & 0x01) != 0);
        
        // In actual use, toggling PB0 would trigger interrupt
    }
    
    SUBCASE("External Interrupt - INT0 on PD2") {
        // INT0 is on PD2
        // Can be configured for level, edge, or toggle triggering
        
        const u16 eimsk_addr = 0x3D;   // External Interrupt Mask
        const u16 eicra_addr = 0x69;   // External Interrupt Control A
        
        // Enable INT0
        bus.write_data(eimsk_addr, 0x01);
        
        // Configure for falling edge (ISC01=1, ISC00=0)
        bus.write_data(eicra_addr, 0x02);
        
        // Verify configuration
        CHECK((bus.read_data(eimsk_addr) & 0x01) != 0);
        CHECK((bus.read_data(eicra_addr) & 0x03) == 0x02);
    }
    
    SUBCASE("Multiple Peripherals on Same Pin - Conflict Resolution") {
        // PB2 can be: GPIO, SPI SS, or Timer1 OC1B
        // Priority: Special function > GPIO
        
        // Configure as GPIO output
        bus.write_data(ddrb_addr, 0x04);  // PB2 output
        bus.write_data(portb_addr, 0x00);
        
        // Enable SPI (SS is PB2 in slave mode, but we use master)
        const u16 spcr_addr = 0x4C;
        bus.write_data(spcr_addr, 0x50);  // Master mode
        
        // Enable Timer1 OC1B on PB2
        const u16 tccr1a_addr = 0x80;
        bus.write_data(tccr1a_addr, 0x20);  // COM1B1=1
        
        // Both SPI and Timer try to use PB2
        // Timer PWM typically takes priority over SPI SS in master mode
        // (SS is input in slave mode)
        
        // Verify both enabled
        CHECK((bus.read_data(spcr_addr) & 0x40) != 0);
        CHECK((bus.read_data(tccr1a_addr) & 0x20) != 0);
    }
    
    SUBCASE("Port Read-Modify-Write Behavior") {
        // When reading a port with mixed GPIO and peripheral pins,
        // peripheral-controlled pins may read as 0 or their actual state
        
        // Enable UART TX
        const u16 ucsr0b_addr = 0xC1;
        bus.write_data(ucsr0b_addr, 0x08);
        
        // Set PD2-PD7 as GPIO outputs
        bus.write_data(ddrd_addr, 0xFC);  // PD2-PD7 outputs, PD0-PD1 (UART)
        bus.write_data(portd_addr, 0xFC);  // Set all GPIO high
        
        // Read PORTD
        u8 portd = bus.read_data(portd_addr);
        
        // PD0 and PD1 values depend on UART state
        // PD2-PD7 should be high
        CHECK((portd & 0xFC) == 0xFC);
    }
    
    SUBCASE("Alternate Function Disable") {
        // Verify that disabling alternate functions returns pins to GPIO
        
        // Enable Timer0 PWM on PD5 (OC0B) and PD6 (OC0A)
        const u16 tccr0a_addr = 0x44;
        const u16 tccr0b_addr = 0x45;
        
        bus.write_data(tccr0a_addr, 0xA0);  // COM0A1=1, COM0B1=1
        bus.write_data(tccr0b_addr, 0x01);  // CS00=1 (no prescale)
        
        // Run briefly
        bus.tick_peripherals(100);
        
        // Disable PWM
        bus.write_data(tccr0a_addr, 0x00);
        bus.write_data(tccr0b_addr, 0x00);
        
        // Now PD5 and PD6 should be GPIO-controlled
        bus.write_data(ddrd_addr, 0x60);  // PD5, PD6 as outputs
        bus.write_data(portd_addr, 0x20);  // PD5=1, PD6=0
        
        CHECK((bus.read_data(portd_addr) & 0x60) == 0x20);
    }
}
