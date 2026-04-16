#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega169p = {
    .name = "ATmega169P",
    .flash_words = 0x2000U,
    .sram_bytes = 0x400U,
    .eeprom_bytes = 0x200U,
    .interrupt_vector_count = 23,
    .interrupt_vector_size = 4U,
    .flash_page_size = 0x80U,
    .register_file_range = { 0x0U, 0x1FU },
    .io_range = { 0x20U, 0xFFU },
    .extended_io_range = { 0x100U, 0xFFU },
    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .spmcsr_address = 0x57U,
    
    .prr_address = 0x0U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    
    .pradc_bit = 0x1U,
    .prusart0_bit = 0x2U,
    .prspi_bit = 0x4U,
    .prtwi_bit = 0xFFU,
    .prtimer0_bit = 0xFFU,
    .prtimer1_bit = 0x8U,
    .prtimer2_bit = 0xFFU,
    .smcr_sm_mask = 0xEU,
    .smcr_se_mask = 0x1U,
    .flash_rww_end_word = 0x0U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .cpu_frequency_hz = 16000000U,

    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x0U, .adch_address = 0x0U, 
            .adcsra_address = 0x7AU, .adcsrb_address = 0x7BU, .admux_address = 0x7CU, 
            .vector_index = 19U,
            .didr0_address = 0x7EU,
            .pr_address = 0x64U, .pr_bit = 0x1U
        } }},
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x50U,
            .didr1_address = 0x7FU,
            .vector_index = 18U
        } }},
    .timer8_count = 1U,
    .timers8 = {{ {
            .tcnt_address = 0x46U,
            .ocra_address = 0x47U, 
            .ocrb_address = 0x0U,
            .tifr_address = 0x35U,
            .timsk_address = 0x6EU,
            .tccra_address = 0x44U, 
            .tccrb_address = 0x0U, 
            .assr_address = 0x0U,
            .compare_a_vector_index = 0U, 
            .compare_b_vector_index = 0U, 
            .overflow_vector_index = 11U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0, 
            .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0,
            .t_pin_address = 0x0U, .t_pin_bit = 0,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0,
            .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0,
            .wgm0_mask = 0x48U, .wgm2_mask = 0x0U, .cs_mask = 0x0U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x2U, .compare_b_enable_mask = 0x0U, .overflow_enable_mask = 0x1U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x0U, 
            .ocra_address = 0x0U, 
            .ocrb_address = 0x0U, 
            .icr_address = 0x0U,
            .tifr_address = 0x36U,
            .timsk_address = 0x6FU,
            .tccra_address = 0x80U, 
            .tccrb_address = 0x81U, 
            .tccrc_address = 0x82U,
            .capture_vector_index = 6U, 
            .compare_a_vector_index = 7U, 
            .compare_b_vector_index = 8U, 
            .overflow_vector_index = 9U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0, 
            .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0,
            .t_pin_address = 0x0U, .t_pin_bit = 0,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x2U, .compare_b_enable_mask = 0x4U, .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x8U
        } }},
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x69U, .eicrb_address = 0x0U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 1U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }}
        } }},
    .uart_count = 1U,
    .uarts = {{ {
            .udr_address = 0xC6U, 
            .ucsra_address = 0xC0U, .ucsrb_address = 0xC1U, .ucsrc_address = 0xC2U,
            .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .rx_vector_index = 13U, 
            .udre_vector_index = 14U,
            .tx_vector_index = 15U, 
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U,
            .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 0x64U, .pr_bit = 0x2U
        } }},
    .pcint_count = 0U,
    .pcints = {{  }},
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU,
            .vector_index = 12U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U,
            .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x4U
        } }},
    .twi_count = 0U,
    .twis = {{  }},
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3FU, 
            .eedr_address = 0x40U, 
            .eearl_address = 0x41U, 
            .eearh_address = 0x42U,
            .vector_index = 20U,
            .size = 512U
        } }},
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .vector_index = 0U,
            .wdie_mask = 0x0U,
            .wde_mask = 0x8U,
            .wdce_mask = 0x10U
        } }},
    
    .fuse_address = 0x3U,
    .lockbit_address = 0x1U,
    .signature_address = 0x3U,
    .port_count = 7U,
    .ports = {{ { "PORTA", 0x20U, 0x21U, 0x22U }, { "PORTB", 0x23U, 0x24U, 0x25U }, { "PORTC", 0x26U, 0x27U, 0x28U }, { "PORTD", 0x29U, 0x2AU, 0x2BU }, { "PORTE", 0x2CU, 0x2DU, 0x2EU }, { "PORTF", 0x2FU, 0x30U, 0x31U }, { "PORTG", 0x32U, 0x33U, 0x34U } }}
};

} // namespace vioavr::core::devices
