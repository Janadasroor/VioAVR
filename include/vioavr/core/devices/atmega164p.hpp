#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {
inline constexpr DeviceDescriptor atmega164p {
    .name = "ATmega164P",
    .flash_words = 8192U, .sram_bytes = 1024U, .eeprom_bytes = 512U,
    .interrupt_vector_count = 31U, .interrupt_vector_size = 2U, .flash_page_size = 0x80U,
    .spl_address = 0x5DU, .sph_address = 0x5EU, .sreg_address = 0x5FU, .spmcsr_address = 0x57U,
    .spl_reset = 0x0U, .sph_reset = 0x0U, .sreg_reset = 0x0U,
    .adc = { 
        .adcl_address = 0x78U, .adch_address = 0x79U, .adcsra_address = 0x7AU, .adcsrb_address = 0x7BU, .admux_address = 0x7CU, .vector_index = 24U, .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
        .didr0_address = 0x7EU,
        .adc_pin_address = {{ 0x20U, 0x20U, 0x20U, 0x20U, 0x20U, 0x20U, 0x20U, 0x20U }},
        .adc_pin_bit = {{ 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U }}
    },
    .ac = { 
        .acsr_address = 0x50U, .didr1_address = 0x7FU, .vector_index = 23U,
        .ain0_pin_address = 0x23U, .ain0_pin_bit = 2U,
        .ain1_pin_address = 0x23U, .ain1_pin_bit = 3U
    },
    .timer0 = { .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x48U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .assr_address = 0x0U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U, .compare_a_vector_index = 16U, .compare_b_vector_index = 17U, .overflow_vector_index = 18U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = 0x23U, .t0_pin_bit = 0U, .ocra_pin_address = 0x23U, .ocra_pin_bit = 3U, .ocrb_pin_address = 0x23U, .ocrb_pin_bit = 4U, .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U },
    .timer2 = { .tcnt_address = 0xB2U, .ocra_address = 0xB3U, .ocrb_address = 0xB4U, .tifr_address = 0x37U, .timsk_address = 0x70U, .tccra_address = 0xB0U, .tccrb_address = 0xB1U, .assr_address = 0xB6U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U, .compare_a_vector_index = 9U, .compare_b_vector_index = 10U, .overflow_vector_index = 11U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = 0x0U, .t0_pin_bit = 0U, .ocra_pin_address = 0x29U, .ocra_pin_bit = 7U, .ocrb_pin_address = 0x29U, .ocrb_pin_bit = 6U, .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U },
    .timer1 = { .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x8AU, .icr_address = 0x86U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x82U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U, .capture_vector_index = 12U, .compare_a_vector_index = 13U, .compare_b_vector_index = 14U, .overflow_vector_index = 15U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .ext_interrupt = { .eicra_address = 0x69U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU, .int0_vector_index = 1U },
    .uart0 = { .udr_address = 0xC6U, .ucsra_address = 0xC0U, .ucsrb_address = 0xC1U, .ucsrc_address = 0xC2U, .ubrrl_address = 0xC4U, .ubrrh_address = 0xC5U, .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U, .rx_vector_index = 20U, .udre_vector_index = 21U, .tx_vector_index = 22U },
    .pin_change_interrupt_0 = { .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6BU, .pcicr_enable_mask = 0x1U, .pcifr_flag_mask = 0x1U, .vector_index = 4U },
    .pin_change_interrupt_1 = { .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6CU, .pcicr_enable_mask = 0x2U, .pcifr_flag_mask = 0x2U, .vector_index = 5U },
    .pin_change_interrupt_2 = { .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6DU, .pcicr_enable_mask = 0x4U, .pcifr_flag_mask = 0x4U, .vector_index = 6U },
    .spi = { .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU, .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 19U },
    .twi = { .twbr_address = 0xB8U, .twsr_address = 0xB9U, .twar_address = 0xBAU, .twdr_address = 0xBBU, .twcr_address = 0xBCU, .twamr_address = 0xBDU, .vector_index = 26U },
    .eeprom = { .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x42U, .vector_index = 25U },
    .wdt = { .wdtcsr_address = 0x60U, .vector_index = 8U },
    .fuse_address = 0x0U, .lockbit_address = 0x0U, .signature_address = 0x0U,
    .ports = {{
        { "PORTA", 0x20U, 0x21U, 0x22U },
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTC", 0x26U, 0x27U, 0x28U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U }
    }},
};
} // namespace vioavr::core::devices
