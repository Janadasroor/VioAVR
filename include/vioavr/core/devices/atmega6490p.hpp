#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {
inline constexpr DeviceDescriptor atmega6490p {
    .name = "ATmega6490P",
    .flash_words = 4096U, .sram_bytes = 4096U, .eeprom_bytes = 2048U,
    .interrupt_vector_count = 25U, .interrupt_vector_size = 2U,
    .adc = { .adcl_address = 0x78U, .adch_address = 0x79U, .adcsra_address = 0x7AU, .adcsrb_address = 0x7BU, .admux_address = 0x7CU, .vector_index = 19U },
    .timer0 = { .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x0U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x0U, .compare_a_vector_index = 10U, .compare_b_vector_index = 0U, .overflow_vector_index = 11U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .timer1 = { .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x8AU, .icr_address = 0x86U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x82U, .capture_vector_index = 6U, .compare_a_vector_index = 7U, .compare_b_vector_index = 8U, .overflow_vector_index = 9U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .ext_interrupt = { .eicra_address = 0x69U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU, .int0_vector_index = 1U },
    .uart0 = { .udr_address = 0xC6U, .ucsra_address = 0xC0U, .ucsrb_address = 0xC1U, .ucsrc_address = 0xC2U, .rx_vector_index = 13U, .udre_vector_index = 14U, .tx_vector_index = 15U },
    .spi = { .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU, .vector_index = 12U },
    .twi = { .twbr_address = 0x0U, .twsr_address = 0x0U, .twar_address = 0x0U, .twdr_address = 0x0U, .twcr_address = 0x0U, .twamr_address = 0x0U, .vector_index = 0U },
    .eeprom = { .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x42U, .vector_index = 20U },
    .wdt = { .wdtcsr_address = 0x60U, .vector_index = 0U },
    .ports = {{
        { "PORTA", 0x20U, 0x21U, 0x22U },
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTC", 0x26U, 0x27U, 0x28U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU },
        { "PORTE", 0x2CU, 0x2DU, 0x2EU },
        { "PORTF", 0x2FU, 0x30U, 0x31U },
        { "PORTG", 0x32U, 0x33U, 0x34U },
        { "PORTH", 0xD8U, 0xD9U, 0xDAU }
    }},
};
} // namespace vioavr::core::devices
