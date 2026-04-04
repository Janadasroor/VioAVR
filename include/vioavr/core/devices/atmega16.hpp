#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {
inline constexpr DeviceDescriptor atmega16 {
    .name = "ATmega16",
    .flash_words = 8192U, .sram_bytes = 1024U, .eeprom_bytes = 512U,
    .interrupt_vector_count = 21U, .interrupt_vector_size = 2U,
    .adc = { .adcl_address = 0x24U, .adch_address = 0x25U, .adcsra_address = 0x26U, .adcsrb_address = 0x0U, .admux_address = 0x27U, .vector_index = 14U },
    .timer0 = { .tcnt_address = 0x52U, .ocra_address = 0x5CU, .ocrb_address = 0x0U, .tifr_address = 0x58U, .timsk_address = 0x59U, .tccra_address = 0x53U, .tccrb_address = 0x0U, .compare_a_vector_index = 19U, .compare_b_vector_index = 0U, .overflow_vector_index = 9U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .timer1 = { .tcnt_address = 0x4CU, .ocra_address = 0x4AU, .ocrb_address = 0x48U, .icr_address = 0x46U, .tifr_address = 0x58U, .timsk_address = 0x59U, .tccra_address = 0x4FU, .tccrb_address = 0x4EU, .tccrc_address = 0x0U, .capture_vector_index = 5U, .compare_a_vector_index = 6U, .compare_b_vector_index = 7U, .overflow_vector_index = 8U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .ext_interrupt = { .eicra_address = 0x0U, .eimsk_address = 0x5BU, .eifr_address = 0x5AU, .int0_vector_index = 1U },
    .uart0 = { .udr_address = 0x2CU, .ucsra_address = 0x2BU, .ucsrb_address = 0x2AU, .ucsrc_address = 0x40U, .rx_vector_index = 11U, .udre_vector_index = 12U, .tx_vector_index = 13U },
    .spi = { .spcr_address = 0x2DU, .spsr_address = 0x2EU, .spdr_address = 0x2FU, .vector_index = 10U },
    .twi = { .twbr_address = 0x20U, .twsr_address = 0x21U, .twar_address = 0x22U, .twdr_address = 0x23U, .twcr_address = 0x56U, .twamr_address = 0x0U, .vector_index = 17U },
    .eeprom = { .eecr_address = 0x3CU, .eedr_address = 0x3DU, .eearl_address = 0x3EU, .eearh_address = 0x3FU, .vector_index = 0U },
    .wdt = { .wdtcsr_address = 0x41U, .vector_index = 0U },
    .ports = {{
        { "PORTA", 0x39U, 0x3AU, 0x3BU },
        { "PORTB", 0x36U, 0x37U, 0x38U },
        { "PORTC", 0x33U, 0x34U, 0x35U },
        { "PORTD", 0x30U, 0x31U, 0x32U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U }
    }},
};
} // namespace vioavr::core::devices
