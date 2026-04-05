#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {
inline constexpr DeviceDescriptor atmega64 {
    .name = "ATmega64",
    .flash_words = 32768U, .sram_bytes = 4096U, .eeprom_bytes = 2048U,
    .interrupt_vector_count = 35U, .interrupt_vector_size = 4U, .flash_page_size = 0x100U,
    .spl_address = 0x5DU, .sph_address = 0x5EU, .sreg_address = 0x5FU, .spmcsr_address = 0x68U,
    .smcr_address = 0x0U, .mcusr_address = 0x0U,
    .flash_rww_end_word = 28672U,
    .spl_reset = 0x0U, .sph_reset = 0x0U, .sreg_reset = 0x0U,
    .adc = { 
        .adcl_address = 0x24U, .adch_address = 0x25U, .adcsra_address = 0x26U, .adcsrb_address = 0x8EU, .admux_address = 0x27U, .vector_index = 21U, .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
        .didr0_address = 0x0U,
        .adc_pin_address = {{ 0x20U, 0x20U, 0x20U, 0x20U, 0x20U, 0x20U, 0x20U, 0x20U }},
        .adc_pin_bit = {{ 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U }},
        .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }}
    },
    .ac = { 
        .acsr_address = 0x28U, .didr1_address = 0x0U, .vector_index = 23U,
        .ain0_pin_address = 0x21U, .ain0_pin_bit = 2U,
        .ain1_pin_address = 0x21U, .ain1_pin_bit = 3U
    },
    .timer0 = { .tcnt_address = 0x52U, .ocra_address = 0x51U, .ocrb_address = 0x0U, .tifr_address = 0x56U, .timsk_address = 0x57U, .tccra_address = 0x53U, .tccrb_address = 0x0U, .assr_address = 0x0U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U, .compare_a_vector_index = 15U, .compare_b_vector_index = 0U, .overflow_vector_index = 16U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = 0x0U, .t0_pin_bit = 0U, .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U },
    .timer2 = { .tcnt_address = 0x44U, .ocra_address = 0x43U, .ocrb_address = 0x0U, .tifr_address = 0x0U, .timsk_address = 0x0U, .tccra_address = 0x45U, .tccrb_address = 0x0U, .assr_address = 0x50U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U, .compare_a_vector_index = 9U, .compare_b_vector_index = 0U, .overflow_vector_index = 10U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = 0x0U, .t0_pin_bit = 0U, .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U },
    .timer1 = { .tcnt_address = 0x4CU, .ocra_address = 0x4AU, .ocrb_address = 0x48U, .icr_address = 0x46U, .tifr_address = 0x56U, .timsk_address = 0x57U, .tccra_address = 0x4FU, .tccrb_address = 0x4EU, .tccrc_address = 0x7AU, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U, .capture_vector_index = 11U, .compare_a_vector_index = 12U, .compare_b_vector_index = 13U, .overflow_vector_index = 14U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U },
    .ext_interrupt = { .eicra_address = 0x6AU, .eimsk_address = 0x59U, .eifr_address = 0x58U, .int0_vector_index = 1U, .int1_vector_index = 2U },
    .uart0 = { .udr_address = 0x2CU, .ucsra_address = 0x2BU, .ucsrb_address = 0x2AU, .ucsrc_address = 0x95U, .ubrrl_address = 0x29U, .ubrrh_address = 0x90U, .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U, .rx_vector_index = 18U, .udre_vector_index = 19U, .tx_vector_index = 20U },
    .pin_change_interrupt_0 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x1U, .pcifr_flag_mask = 0x1U, .vector_index = 0U },
    .pin_change_interrupt_1 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x2U, .pcifr_flag_mask = 0x2U, .vector_index = 0U },
    .pin_change_interrupt_2 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x4U, .pcifr_flag_mask = 0x4U, .vector_index = 0U },
    .spi = { .spcr_address = 0x2DU, .spsr_address = 0x2EU, .spdr_address = 0x2FU, .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 17U },
    .twi = { .twbr_address = 0x70U, .twsr_address = 0x71U, .twar_address = 0x72U, .twdr_address = 0x73U, .twcr_address = 0x74U, .twamr_address = 0x0U, .vector_index = 33U },
    .eeprom = { .eecr_address = 0x3CU, .eedr_address = 0x3DU, .eearl_address = 0x3EU, .eearh_address = 0x3FU, .vector_index = 22U },
    .wdt = { .wdtcsr_address = 0x41U, .vector_index = 0U },
    .fuse_address = 0x0U, .lockbit_address = 0x0U, .signature_address = 0x0U,
    .ports = {{
        { "PORTA", 0x39U, 0x3AU, 0x3BU },
        { "PORTB", 0x36U, 0x37U, 0x38U },
        { "PORTC", 0x33U, 0x34U, 0x35U },
        { "PORTD", 0x30U, 0x31U, 0x32U },
        { "PORTE", 0x21U, 0x22U, 0x23U },
        { "PORTF", 0x20U, 0x61U, 0x62U },
        { "PORTG", 0x63U, 0x64U, 0x65U },
        { "", 0x00U, 0x00U, 0x00U }
    }},
};
} // namespace vioavr::core::devices
