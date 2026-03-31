#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {
inline constexpr DeviceDescriptor at90usb82 {
    .name = "AT90USB82",
    .flash_words = 2048U, .sram_bytes = 512U, .eeprom_bytes = 512U,
    .interrupt_vector_count = 29U, .interrupt_vector_size = 2U,
    .adc = { .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x0U, .adcsrb_address = 0x0U, .admux_address = 0x0U, .vector_index = 0U },
    .timer0 = { .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x48U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .compare_a_vector_index = 19U, .compare_b_vector_index = 20U, .overflow_vector_index = 21U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .timer1 = { .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x8AU, .icr_address = 0x86U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x82U, .capture_vector_index = 14U, .compare_a_vector_index = 15U, .compare_b_vector_index = 16U, .overflow_vector_index = 18U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .ext_interrupt = { .eicra_address = 0x69U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU, .int0_vector_index = 1U },
    .uart0 = { .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .rx_vector_index = 0U, .udre_vector_index = 0U, .tx_vector_index = 0U },
    .spi = { .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU, .vector_index = 22U },
    .twi = { .twbr_address = 0x0U, .twsr_address = 0x0U, .twar_address = 0x0U, .twdr_address = 0x0U, .twcr_address = 0x0U, .twamr_address = 0x0U, .vector_index = 0U },
    .eeprom = { .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x42U, .vector_index = 27U },
    .wdt = { .wdtcsr_address = 0x60U, .vector_index = 13U },
    .ports = {{
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTC", 0x26U, 0x27U, 0x28U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U }
    }},
};
} // namespace vioavr::core::devices
