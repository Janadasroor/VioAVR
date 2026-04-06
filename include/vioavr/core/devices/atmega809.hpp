#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {
inline constexpr DeviceDescriptor atmega809 {
    .name = "ATmega809",
    .flash_words = 0U, .sram_bytes = 1024U, .eeprom_bytes = 256U,
    .interrupt_vector_count = 40U, .interrupt_vector_size = 2U, .flash_page_size = 0x0U,
    .spl_address = 0xDU, .sph_address = 0xEU, .sreg_address = 0xFU, .spmcsr_address = 0x0U,
    .prr_address = 0x0U, .smcr_address = 0x0U, .mcusr_address = 0x0U,
    .flash_rww_end_word = 0U,
    .spl_reset = 0x0U, .sph_reset = 0x0U, .sreg_reset = 0x0U,
    .adc = { 
        .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x0U, .adcsrb_address = 0x0U, .admux_address = 0x0U, .vector_index = 0U, .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
        .didr0_address = 0x0U,
        .adc_pin_address = {{ 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
        .adc_pin_bit = {{ 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }},
        .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }},
        .adsc_mask = 0x40U, .adif_mask = 0x10U, .adie_mask = 0x8U
    },
    .ac = { 
        .acsr_address = 0x0U, .didr1_address = 0x0U, .vector_index = 0U,
        .ain0_pin_address = 0x0U, .ain0_pin_bit = 0U,
        .ain1_pin_address = 0x0U, .ain1_pin_bit = 0U,
        .aci_mask = 0x10U, .acie_mask = 0x8U
    },
    .timer0 = { .tcnt_address = 0x0U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .tifr_address = 0x0U, .timsk_address = 0x0U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .assr_address = 0x0U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U, .compare_a_vector_index = 0U, .compare_b_vector_index = 0U, .overflow_vector_index = 0U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = 0x0U, .t0_pin_bit = 0U, .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U, .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U },
    .timer2 = { .tcnt_address = 0x0U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .tifr_address = 0x0U, .timsk_address = 0x0U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .assr_address = 0x0U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U, .compare_a_vector_index = 0U, .compare_b_vector_index = 0U, .overflow_vector_index = 0U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U, .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U, .as2_mask = 0x20U, .tcn2ub_mask = 0x10U },
    .timer1 = { .tcnt_address = 0x0U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .icr_address = 0x0U, .tifr_address = 0x0U, .timsk_address = 0x0U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .tccrc_address = 0x0U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U, .capture_vector_index = 0U, .compare_a_vector_index = 0U, .compare_b_vector_index = 0U, .overflow_vector_index = 0U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t1_pin_address = 0x0U, .t1_pin_bit = 0U, .icp1_pin_address = 0x0U, .icp1_pin_bit = 0U, .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices1_mask = 0x40U, .icnc1_mask = 0x80U },
    .ext_interrupt = { .eicra_address = 0x0U, .eimsk_address = 0x0U, .eifr_address = 0x0U, .int0_vector_index = 0U, .int1_vector_index = 0U },
    .uart0 = { .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U, .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U, .rx_vector_index = 0U, .udre_vector_index = 0U, .tx_vector_index = 0U },
    .pin_change_interrupt_0 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x1U, .pcifr_flag_mask = 0x1U, .vector_index = 0U },
    .pin_change_interrupt_1 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x2U, .pcifr_flag_mask = 0x2U, .vector_index = 0U },
    .pin_change_interrupt_2 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x4U, .pcifr_flag_mask = 0x4U, .vector_index = 0U },
    .spi = { .spcr_address = 0x0U, .spsr_address = 0x0U, .spdr_address = 0x0U, .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 0U },
    .twi = { .twbr_address = 0x0U, .twsr_address = 0x0U, .twar_address = 0x0U, .twdr_address = 0x0U, .twcr_address = 0x0U, .twamr_address = 0x0U, .vector_index = 14U },
    .eeprom = { .eecr_address = 0x0U, .eedr_address = 0x0U, .eearl_address = 0x0U, .eearh_address = 0x0U, .vector_index = 0U },
    .wdt = { .wdtcsr_address = 0x0U, .vector_index = 0U },
    .fuse_address = 0x1280U, .lockbit_address = 0x128AU, .signature_address = 0x1103U,
    .ports = {{
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U },
        { "", 0x00U, 0x00U, 0x00U }
    }},
};
} // namespace vioavr::core::devices
