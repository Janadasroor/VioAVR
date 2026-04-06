#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {
inline constexpr DeviceDescriptor atmega16a {
    .name = "ATmega16A",
    .flash_words = 8192U, .sram_bytes = 1024U, .eeprom_bytes = 512U,
    .interrupt_vector_count = 21U, .interrupt_vector_size = 2U, .flash_page_size = 0x80U,
    .spl_address = 0x5DU, .sph_address = 0x5EU, .sreg_address = 0x5FU, .spmcsr_address = 0x57U,
    .prr_address = 0x0U, .smcr_address = 0x0U, .mcusr_address = 0x0U,
    .flash_rww_end_word = 7168U,
    .spl_reset = 0x0U, .sph_reset = 0x0U, .sreg_reset = 0x0U,
    .adc = { 
        .adcl_address = 0x24U, .adch_address = 0x25U, .adcsra_address = 0x26U, .adcsrb_address = 0x0U, .admux_address = 0x27U, .vector_index = 14U, .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
        .didr0_address = 0x0U,
        .adc_pin_address = {{ 0x3BU, 0x3BU, 0x3BU, 0x3BU, 0x3BU, 0x3BU, 0x3BU, 0x3BU }},
        .adc_pin_bit = {{ 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U }},
        .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }},
        .adsc_mask = 0x40U, .adif_mask = 0x10U, .adie_mask = 0x8U
    },
    .ac = { 
        .acsr_address = 0x28U, .didr1_address = 0x0U, .vector_index = 0U,
        .ain0_pin_address = 0x38U, .ain0_pin_bit = 2U,
        .ain1_pin_address = 0x38U, .ain1_pin_bit = 3U,
        .aci_mask = 0x10U, .acie_mask = 0x8U
    },
    .timer0 = { .tcnt_address = 0x52U, .ocra_address = 0x5CU, .ocrb_address = 0x0U, .tifr_address = 0x58U, .timsk_address = 0x59U, .tccra_address = 0x53U, .tccrb_address = 0x0U, .assr_address = 0x0U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U, .compare_a_vector_index = 19U, .compare_b_vector_index = 0U, .overflow_vector_index = 9U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t0_pin_address = 0x38U, .t0_pin_bit = 0U, .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U, .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U },
    .timer2 = { .tcnt_address = 0x44U, .ocra_address = 0x43U, .ocrb_address = 0x0U, .tifr_address = 0x0U, .timsk_address = 0x0U, .tccra_address = 0x45U, .tccrb_address = 0x0U, .assr_address = 0x42U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U, .compare_a_vector_index = 3U, .compare_b_vector_index = 0U, .overflow_vector_index = 4U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .tosc1_pin_address = 0x35U, .tosc1_pin_bit = 6U, .tosc2_pin_address = 0x35U, .tosc2_pin_bit = 7U, .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U, .as2_mask = 0x8U, .tcn2ub_mask = 0x4U },
    .timer1 = { .tcnt_address = 0x4CU, .ocra_address = 0x4AU, .ocrb_address = 0x48U, .icr_address = 0x46U, .tifr_address = 0x58U, .timsk_address = 0x59U, .tccra_address = 0x4FU, .tccrb_address = 0x4EU, .tccrc_address = 0x0U, .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U, .capture_vector_index = 5U, .compare_a_vector_index = 6U, .compare_b_vector_index = 7U, .overflow_vector_index = 8U, .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x02U, .compare_b_enable_mask = 0x04U, .overflow_enable_mask = 0x01U, .t1_pin_address = 0x38U, .t1_pin_bit = 1U, .icp1_pin_address = 0x32U, .icp1_pin_bit = 6U, .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices1_mask = 0x40U, .icnc1_mask = 0x80U },
    .ext_interrupt = { .eicra_address = 0x0U, .eimsk_address = 0x5BU, .eifr_address = 0x5AU, .int0_vector_index = 1U, .int1_vector_index = 2U },
    .uart0 = { .udr_address = 0x2CU, .ucsra_address = 0x2BU, .ucsrb_address = 0x2AU, .ucsrc_address = 0x40U, .ubrrl_address = 0x0U, .ubrrh_address = 0x40U, .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U, .rx_vector_index = 11U, .udre_vector_index = 12U, .tx_vector_index = 13U },
    .pin_change_interrupt_0 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x1U, .pcifr_flag_mask = 0x1U, .vector_index = 0U },
    .pin_change_interrupt_1 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x2U, .pcifr_flag_mask = 0x2U, .vector_index = 0U },
    .pin_change_interrupt_2 = { .pcicr_address = 0x0U, .pcifr_address = 0x0U, .pcmsk_address = 0x0U, .pcicr_enable_mask = 0x4U, .pcifr_flag_mask = 0x4U, .vector_index = 0U },
    .spi = { .spcr_address = 0x2DU, .spsr_address = 0x2EU, .spdr_address = 0x2FU, .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 10U },
    .twi = { .twbr_address = 0x20U, .twsr_address = 0x21U, .twar_address = 0x22U, .twdr_address = 0x23U, .twcr_address = 0x56U, .twamr_address = 0x0U, .vector_index = 17U },
    .eeprom = { .eecr_address = 0x3CU, .eedr_address = 0x3DU, .eearl_address = 0x3EU, .eearh_address = 0x3FU, .vector_index = 0U },
    .wdt = { .wdtcsr_address = 0x41U, .vector_index = 0U },
    .fuse_address = 0x0U, .lockbit_address = 0x0U, .signature_address = 0x0U,
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
