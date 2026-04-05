#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {
inline constexpr DeviceDescriptor atmega64hve2 {
    .name = "ATmega64HVE2",
    .flash_words = 32768U, .sram_bytes = 4096U, .eeprom_bytes = 1024U,
    .interrupt_vector_count = 25U, .interrupt_vector_size = 4U, .flash_page_size = 0x80U,
    .spl_address = 0x5DU, .sph_address = 0x5EU, .sreg_address = 0x5FU, .spmcsr_address = 0x57U,
    .spl_reset = 0x0U, .sph_reset = 0x0U, .sreg_reset = 0x0U,
    .adc = { 
        .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x0U, .adcsrb_address = 0x0U, .admux_address = 0x0U, .vector_index = 17U, .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
        .didr0_address = 0x7EU,
        .adc_pin_address = {{ 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
        .adc_pin_bit = {{ 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }},
        .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }}
    },
    .ac = { 
        .acsr_address = 0x0U, .didr1_address = 0x0U, .vector_index = 0U,
        .ain0_pin_address = 0x0U, .ain0_pin_bit = 0U,
        .ain1_pin_address = 0x0U, .ain1_pin_bit = 0U
    },
    .timer0 = { .tcnt_address = 0x46U, .ocra_address = 0x48U, .ocrb_address = 0x49U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .assr_address = 0x0U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U, .compare_a_vector_index = 11U, .compare_b_vector_index = 12U, .overflow_vector_index = 13U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = 0x0U, .t0_pin_bit = 0U, .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U },
    .timer2 = { .tcnt_address = 0x0U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .tifr_address = 0x0U, .timsk_address = 0x0U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .assr_address = 0x0U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U, .compare_a_vector_index = 0U, .compare_b_vector_index = 0U, .overflow_vector_index = 0U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = 0x0U, .t0_pin_bit = 0U, .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U },
    .timer1 = { .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x89U, .icr_address = 0x0U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x0U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U, .capture_vector_index = 0U, .compare_a_vector_index = 7U, .compare_b_vector_index = 8U, .overflow_vector_index = 9U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .ext_interrupt = { .eicra_address = 0x69U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU, .int0_vector_index = 1U, .int1_vector_index = 3U },
    .uart0 = { .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U, .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U, .rx_vector_index = 0U, .udre_vector_index = 0U, .tx_vector_index = 0U },
    .pin_change_interrupt_0 = { .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6BU, .pcicr_enable_mask = 0x1U, .pcifr_flag_mask = 0x1U, .vector_index = 2U },
    .pin_change_interrupt_1 = { .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6CU, .pcicr_enable_mask = 0x2U, .pcifr_flag_mask = 0x2U, .vector_index = 3U },
    .pin_change_interrupt_2 = { .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x4U, .pcifr_flag_mask = 0x4U, .vector_index = 0U },
    .spi = { .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU, .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 16U },
    .twi = { .twbr_address = 0x0U, .twsr_address = 0x0U, .twar_address = 0x0U, .twdr_address = 0x0U, .twcr_address = 0x0U, .twamr_address = 0x0U, .vector_index = 0U },
    .eeprom = { .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x42U, .vector_index = 22U },
    .wdt = { .wdtcsr_address = 0x60U, .vector_index = 4U },
    .fuse_address = 0x0U, .lockbit_address = 0x0U, .signature_address = 0x0U,
    .ports = {{
        { "PORTA", 0x20U, 0x21U, 0x22U },
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U }
    }},
};
} // namespace vioavr::core::devices
