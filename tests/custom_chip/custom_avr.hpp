#pragma once

#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

// Custom AVR chip definition for testing
// Based on ATmega328P but with modified pin mappings for verification
struct CustomAvrDescriptor {
    static constexpr std::string_view name = "CustomAVR";
    static constexpr u16 flash_words = 0x8000;      // 32KB
    static constexpr u16 sram_bytes = 0x800;      // 2KB
    static constexpr u16 sram_start = 0x100;
    static constexpr u16 eeprom_bytes = 0x400;    // 1KB
    static constexpr u8 sreg_reset = 0x00;
    static constexpr u16 spl_reset = 0x8FF;
    static constexpr u16 sph_reset = 0x08;
    static constexpr u16 mcusr_address = 0x54;
    
    // Memory ranges
    static constexpr AddressRange sram_range() { return {sram_start, static_cast<u16>(sram_start + sram_bytes)}; }
    static constexpr u16 reset_word_address() { return 0; }
    
    // GPIO Ports - intentionally scrambled for testing
    struct PortDescriptor {
        u16 pin_addr;
        u16 ddr_addr;
        u16 port_addr;
        u8 pins;  // number of pins available
        u8 pin_start; // bit offset in pin mux
    };
    
    static constexpr PortDescriptor portb{0x23, 0x24, 0x25, 8, 0};   // Port B - pins 0-7
    static constexpr PortDescriptor portc{0x26, 0x27, 0x28, 7, 8};   // Port C - pins 8-14 (PC6=RESET)
    static constexpr PortDescriptor portd{0x29, 0x2A, 0x2B, 8, 16};  // Port D - pins 15-22
    
    // Timer descriptors
    struct Timer8Desc {
        u16 tcnt;
        u16 ocra;
        u16 ocrb;
        u16 tccra;
        u16 tccrb;
        u16 timsk;
        u16 tifr;
        u16 vector_ovf;
        u16 vector_compa;
        u16 vector_compb;
    };
    
    static constexpr Timer8Desc timer0{
        0x46, 0x47, 0x48, 0x44, 0x45, 0x6E, 0x35,
        16, 14, 15  // OVF, COMPA, COMPB
    };
    
    struct Timer16Desc {
        u16 tcnt;
        u16 ocra;
        u16 ocrb;
        u16 icr;
        u16 tccra;
        u16 tccrb;
        u16 tccrc;
        u16 timsk;
        u16 tifr;
        u16 vector_ovf;
        u16 vector_compa;
        u16 vector_compb;
        u16 vector_ic;
    };
    
    static constexpr Timer16Desc timer1{
        0x84, 0x88, 0x8A, 0x86, 0x80, 0x81, 0x82, 0x6F, 0x36,
        13, 11, 12, 10  // OVF, COMPA, COMPB, CAPT
    };
    
    // UART descriptor
    struct UartDesc {
        u16 udr;
        u16 ucsra;
        u16 ucsrb;
        u16 ucsrc;
        u16 ubrr;
        u16 vector_rx;
        u16 vector_udre;
        u16 vector_tx;
    };
    
    static constexpr UartDesc uart0{
        0xC6, 0xC0, 0xC1, 0xC2, 0xC4,
        18, 19, 20  // RX, UDRE, TX
    };
    
    // ADC descriptor
    struct AdcDesc {
        u16 adc;
        u16 adcsra;
        u16 adcsrb;
        u16 admux;
        u16 didr0;
        u8 vector;
    };
    
    static constexpr AdcDesc adc{
        0x78, 0x7A, 0x7B, 0x7C, 0x7E, 21
    };
    
    // Interrupt vectors (must match real hardware)
    static constexpr std::array<std::string_view, 26> interrupt_names = {
        "RESET", "INT0", "INT1", "PCINT0", "PCINT1", "PCINT2",
        "WDT", "TIMER2_COMPA", "TIMER2_COMPB", "TIMER2_OVF",
        "TIMER1_CAPT", "TIMER1_COMPA", "TIMER1_COMPB", "TIMER1_OVF",
        "TIMER0_COMPA", "TIMER0_COMPB", "TIMER0_OVF",
        "SPI_STC", "USART_RX", "USART_UDRE", "USART_TX",
        "ADC", "EE_READY", "ANALOG_COMP", "TWI", "SPM_READY"
    };
    
    // Pin mappings for peripherals (critical for accuracy)
    // Format: {port_pin_index, function_name}
    struct PinMapping {
        u8 pin_mux_index;
        std::string_view function;
    };
    
    // UART pins on Port D: RX=PD0 (pin 16), TX=PD1 (pin 17)
    static constexpr PinMapping uart_rx{16, "RXD"};
    static constexpr PinMapping uart_tx{17, "TXD"};
    
    // Timer1: OC1A=PB1 (pin 1), OC1B=PB2 (pin 2), ICP=PB0 (pin 0)
    static constexpr PinMapping timer1_oc1a{1, "OC1A"};
    static constexpr PinMapping timer1_oc1b{2, "OC1B"};
    static constexpr PinMapping timer1_icp{0, "ICP1"};
    
    // SPI pins on Port B
    static constexpr PinMapping spi_mosi{3, "MOSI"};
    static constexpr PinMapping spi_miso{4, "MISO"};
    static constexpr PinMapping spi_sck{5, "SCK"};
    static constexpr PinMapping spi_ss{2, "SS"};
    
    // ADC channels on Port C (ADC0-ADC5 = PC0-PC5, pins 8-13)
    static constexpr std::array<u8, 6> adc_pins = {8, 9, 10, 11, 12, 13};
};

// Static instance for use in tests
inline constexpr CustomAvrDescriptor custom_avr;

} // namespace vioavr::core::devices
