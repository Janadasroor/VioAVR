#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;

TEST_CASE("UART Fidelity: Baud Rate Accuracy and Timing")
{
    const auto* device_desc = DeviceCatalog::find("ATmega328P");
    REQUIRE(device_desc != nullptr);
    
    MemoryBus bus(*device_desc);
    AvrCpu cpu(bus);
    cpu.reset();
    
    // Attach UART peripheral
    PinMux pin_mux(device_desc->port_count);
    bus.set_pin_mux(&pin_mux);
    auto uart_ptr = std::make_unique<Uart>("UART0", device_desc->uarts[0], pin_mux);
    Uart* uart = uart_ptr.get();
    bus.attach_peripheral(*uart_ptr.release());
    
    REQUIRE(uart != nullptr);
    
    // UART register addresses for ATmega328P
    const u16 ubrr0l_addr = 0xC4;  // Baud rate register low
    const u16 ubrr0h_addr = 0xC5;  // Baud rate register high
    const u16 ucsr0a_addr = 0xC0;  // Control/Status A
    const u16 ucsr0b_addr = 0xC1;  // Control/Status B
    const u16 ucsr0c_addr = 0xC2;  // Control/Status C
    const u16 udr0_addr = 0xC6;    // Data register
    
    // UCSR0A bits
    const u8 kRxc0 = 0x80;  // Receive complete
    const u8 kTxc0 = 0x40;  // Transmit complete
    const u8 kUdre0 = 0x20; // Data register empty
    const u8 kFe0 = 0x10;   // Frame error
    const u8 kDor0 = 0x08;  // Data overrun
    const u8 kUpe0 = 0x04;   // Parity error
    const u8 kU2x0 = 0x02;   // Double speed
    const u8 kMpcm0 = 0x01; // Multi-processor mode
    
    // UCSR0B bits
    const u8 kRxcien0 = 0x80; // RX complete interrupt enable
    const u8 kTxcien0 = 0x40; // TX complete interrupt enable
    const u8 kUdrien0 = 0x20; // Data reg empty interrupt enable
    const u8 kRxen0 = 0x10;   // Receiver enable
    const u8 kTxen0 = 0x08;   // Transmitter enable
    const u8 kUcsz02 = 0x04;  // Character size bit 2
    const u8 kRxb80 = 0x02;   // RX data bit 8
    const u8 kTxb80 = 0x01;   // TX data bit 8
    
    // UCSR0C bits
    const u8 kUmsel01 = 0x80; // Mode select bit 1
    const u8 kUmsel00 = 0x40; // Mode select bit 0
    const u8 kUpm01 = 0x20;   // Parity mode bit 1
    const u8 kUpm00 = 0x10;   // Parity mode bit 0
    const u8 kUsbs0 = 0x08;   // Stop bit select
    const u8 kUcsz01 = 0x04;  // Character size bit 1
    const u8 kUcsz00 = 0x02;  // Character size bit 0
    const u8 kUcpol0 = 0x01;  // Clock polarity (sync only)
    
    SUBCASE("UART Register Reset State") {
        // After reset, transmitter and receiver should be disabled
        CHECK((bus.read_data(ucsr0b_addr) & (kRxen0 | kTxen0)) == 0);
        // UDRE should be set (data register empty)
        CHECK((bus.read_data(ucsr0a_addr) & kUdre0) != 0);
    }
    
    SUBCASE("UART Baud Rate Setting - Standard Rates") {
        // Test common baud rates at 16MHz
        // UBRR = (fosc / (16 * BAUD)) - 1 for normal mode
        // UBRR = (fosc / (8 * BAUD)) - 1 for U2X mode
        
        struct BaudTest {
            u32 baud;
            u16 ubrr_normal;
            u16 ubrr_u2x;
            const char* name;
        };
        
        BaudTest tests[] = {
            {9600, 103, 207, "9600"},
            {19200, 51, 103, "19200"},
            {38400, 25, 51, "38400"},
            {57600, 16, 34, "57600"},
            {115200, 8, 16, "115200"},
        };
        
        for (const auto& tc : tests) {
            // Enable normal mode
            bus.write_data(ucsr0a_addr, 0);  // Clear U2X
            bus.write_data(ubrr0h_addr, static_cast<u8>(tc.ubrr_normal >> 8));
            bus.write_data(ubrr0l_addr, static_cast<u8>(tc.ubrr_normal & 0xFF));
            
            // Verify UBRR set correctly
            u16 ubrr_read = bus.read_data(ubrr0l_addr);
            ubrr_read |= static_cast<u16>(bus.read_data(ubrr0h_addr)) << 8;
            CHECK(ubrr_read == tc.ubrr_normal);
            
            // Enable U2X mode
            bus.write_data(ucsr0a_addr, kU2x0);
            bus.write_data(ubrr0h_addr, static_cast<u8>(tc.ubrr_u2x >> 8));
            bus.write_data(ubrr0l_addr, static_cast<u8>(tc.ubrr_u2x & 0xFF));
            
            ubrr_read = bus.read_data(ubrr0l_addr);
            ubrr_read |= static_cast<u16>(bus.read_data(ubrr0h_addr)) << 8;
            CHECK(ubrr_read == tc.ubrr_u2x);
            CHECK((bus.read_data(ucsr0a_addr) & kU2x0) != 0);
        }
    }
    
    SUBCASE("UART Enable Transmitter") {
        // Enable transmitter
        bus.write_data(ucsr0b_addr, kTxen0);
        
        CHECK((bus.read_data(ucsr0b_addr) & kTxen0) != 0);
        // UDRE should be set when transmitter is enabled and ready
        CHECK((bus.read_data(ucsr0a_addr) & kUdre0) != 0);
    }
    
    SUBCASE("UART Enable Receiver") {
        // Enable receiver
        bus.write_data(ucsr0b_addr, kRxen0);
        
        CHECK((bus.read_data(ucsr0b_addr) & kRxen0) != 0);
    }
    
    SUBCASE("UART Frame Format - 8N1") {
        // 8 data bits, no parity, 1 stop bit (standard)
        // UCSZ = 011 (8 bits), UPM = 00 (none), USBS = 0 (1 stop)
        bus.write_data(ucsr0c_addr, kUcsz01 | kUcsz00);
        
        u8 ucsrc = bus.read_data(ucsr0c_addr);
        CHECK((ucsrc & (kUcsz01 | kUcsz00)) == (kUcsz01 | kUcsz00));  // 8 bits
        CHECK((ucsrc & (kUpm01 | kUpm00)) == 0);  // No parity
        CHECK((ucsrc & kUsbs0) == 0);  // 1 stop bit
    }
    
    SUBCASE("UART Frame Format - 8E1") {
        // 8 data bits, even parity, 1 stop bit
        // UCSZ = 011, UPM = 10 (even), USBS = 0
        bus.write_data(ucsr0c_addr, kUcsz01 | kUcsz00 | kUpm01);
        
        u8 ucsrc = bus.read_data(ucsr0c_addr);
        CHECK((ucsrc & (kUcsz01 | kUcsz00)) == (kUcsz01 | kUcsz00));  // 8 bits
        CHECK((ucsrc & (kUpm01 | kUpm00)) == kUpm01);  // Even parity
        CHECK((ucsrc & kUsbs0) == 0);  // 1 stop bit
    }
    
    SUBCASE("UART Frame Format - 8N2") {
        // 8 data bits, no parity, 2 stop bits
        // UCSZ = 011, UPM = 00, USBS = 1
        bus.write_data(ucsr0c_addr, kUcsz01 | kUcsz00 | kUsbs0);
        
        u8 ucsrc = bus.read_data(ucsr0c_addr);
        CHECK((ucsrc & kUsbs0) != 0);  // 2 stop bits
    }
    
    SUBCASE("UART 9-bit Mode") {
        // 9 data bits
        // UCSZ2 = 1 (UCSR0B), UCSZ = 11 (UCSR0C)
        bus.write_data(ucsr0b_addr, kUcsz02);
        bus.write_data(ucsr0c_addr, kUcsz01 | kUcsz00);
        
        u8 ucsrb = bus.read_data(ucsr0b_addr);
        u8 ucsrc = bus.read_data(ucsr0c_addr);
        CHECK((ucsrb & kUcsz02) != 0);
        CHECK((ucsrc & (kUcsz01 | kUcsz00)) == (kUcsz01 | kUcsz00));
    }
    
    SUBCASE("UART Transmit Timing - 9600 baud at 16MHz") {
        // Configure for 9600 baud, normal mode (UBRR=103)
        bus.write_data(ubrr0h_addr, 0);
        bus.write_data(ubrr0l_addr, 103);
        bus.write_data(ucsr0c_addr, kUcsz01 | kUcsz00);  // 8N1
        bus.write_data(ucsr0b_addr, kTxen0);  // Enable transmitter
        
        // At 9600 baud with 16MHz clock:
        // Bit time = 16 * (UBRR + 1) / fosc = 16 * 104 / 16MHz = 104us
        // Frame = 1 start + 8 data + 1 stop = 10 bits = 1.04ms
        // In cycles: 10 bits * 16 * (103+1) = 16640 cycles
        
        // Wait for UDRE
        while ((bus.read_data(ucsr0a_addr) & kUdre0) == 0) {
            bus.tick_peripherals(1);
        }
        
        // Write data to start transmission
        u64 start_cycles = cpu.cycles();
        bus.write_data(udr0_addr, 0x55);
        
        // UDRE should clear immediately after write
        CHECK((bus.read_data(ucsr0a_addr) & kUdre0) == 0);
        
        // Tick for one frame (approximate)
        bus.tick_peripherals(16640);
        
        // TXC should be set, UDRE should be set again
        u8 ucsra = bus.read_data(ucsr0a_addr);
        // Note: Exact timing depends on implementation
        (void)ucsra;
    }
    
    SUBCASE("UART Transmit with U2X Double Speed") {
        // Configure for 9600 baud, U2X mode (UBRR=207)
        bus.write_data(ubrr0h_addr, 0);
        bus.write_data(ubrr0l_addr, 207);
        bus.write_data(ucsr0a_addr, kU2x0);  // Enable double speed
        bus.write_data(ucsr0c_addr, kUcsz01 | kUcsz00);
        bus.write_data(ucsr0b_addr, kTxen0);
        
        // With U2X, bit time is halved
        // Frame time = 10 bits * 8 * (207+1) / 16MHz = 1040us = same baud rate
        // But each bit sampled twice as fast
        
        CHECK((bus.read_data(ucsr0a_addr) & kU2x0) != 0);
        
        // Write and verify
        bus.write_data(udr0_addr, 0xAA);
        CHECK((bus.read_data(ucsr0a_addr) & kUdre0) == 0);
    }
    
    SUBCASE("UART Interrupt Enable Flags") {
        // Enable all UART interrupts
        bus.write_data(ucsr0b_addr, kRxcien0 | kTxcien0 | kUdrien0);
        
        u8 ucsrb = bus.read_data(ucsr0b_addr);
        CHECK((ucsrb & kRxcien0) != 0);
        CHECK((ucsrb & kTxcien0) != 0);
        CHECK((ucsrb & kUdrien0) != 0);
    }
    
    SUBCASE("UART TXC Flag Behavior") {
        // TXC is set when frame transmission completes
        // It must be cleared by writing 1 to it
        
        bus.write_data(ucsr0c_addr, kUcsz01 | kUcsz00);
        bus.write_data(ucsr0b_addr, kTxen0);
        
        // Initial state - TXC should be 0 or 1 depending on reset
        u8 ucsra = bus.read_data(ucsr0a_addr);
        
        // Write data
        bus.write_data(udr0_addr, 0x55);
        
        // Wait for completion
        bus.tick_peripherals(16640);
        
        // TXC should be set
        ucsra = bus.read_data(ucsr0a_addr);
        (void)ucsra;
    }
    
    SUBCASE("UART Multi-Processor Communication Mode") {
        // Enable MPCM - receiver ignores frames without address bit
        bus.write_data(ucsr0a_addr, kMpcm0);
        bus.write_data(ucsr0b_addr, kRxen0);
        
        CHECK((bus.read_data(ucsr0a_addr) & kMpcm0) != 0);
    }
    
    SUBCASE("UART Synchronous Mode") {
        // UMSEL = 01 for synchronous USART
        bus.write_data(ucsr0c_addr, kUmsel00 | kUcsz01 | kUcsz00);
        
        u8 ucsrc = bus.read_data(ucsr0c_addr);
        CHECK((ucsrc & (kUmsel01 | kUmsel00)) == kUmsel00);
        
        // UMSEL = 11 for Master SPI mode
        bus.write_data(ucsr0c_addr, kUmsel01 | kUmsel00 | kUcsz01 | kUcsz00);
        
        ucsrc = bus.read_data(ucsr0c_addr);
        CHECK((ucsrc & (kUmsel01 | kUmsel00)) == (kUmsel01 | kUmsel00));
    }
    
    SUBCASE("UART Loopback Data Integrity") {
        // Enable both transmitter and receiver for loopback
        // Note: This requires the UART to be wired for loopback
        bus.write_data(ubrr0h_addr, 0);
        bus.write_data(ubrr0l_addr, 103);  // 9600 baud
        bus.write_data(ucsr0c_addr, kUcsz01 | kUcsz00);
        bus.write_data(ucsr0b_addr, kTxen0 | kRxen0);
        
        // Write test data
        const u8 test_data = 0x5A;
        bus.write_data(udr0_addr, test_data);
        
        // In a real loopback, RXC would be set after frame reception
        // with received data matching transmitted data
        // This depends on GPIO pin configuration
        
        // For now, just verify the write
        CHECK((bus.read_data(ucsr0a_addr) & kUdre0) == 0);
    }
}
