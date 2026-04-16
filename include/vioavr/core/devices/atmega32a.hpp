#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega32a = {
    .name = "ATmega32A",
    .flash_words = 0x4000U,
    .sram_bytes = 0x800U,
    .eeprom_bytes = 0x400U,
    .interrupt_vector_count = 21,
    .interrupt_vector_size = 4U,
    .flash_page_size = 0x80U,
    .register_file_range = { 0x0U, 0x1FU },
    .io_range = { 0x20U, 0x5FU },
    .extended_io_range = { 0x60U, 0x5FU },
    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .spmcsr_address = 0x57U,
    
    .prr_address = 0x0U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    
    .pradc_bit = 0xFFU,
    .prusart0_bit = 0xFFU,
    .prspi_bit = 0xFFU,
    .prtwi_bit = 0xFFU,
    .prtimer0_bit = 0xFFU,
    .prtimer1_bit = 0xFFU,
    .prtimer2_bit = 0xFFU,
    .smcr_sm_mask = 0x0U,
    .smcr_se_mask = 0x0U,
    .flash_rww_end_word = 0x0U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .cpu_frequency_hz = 16000000U,

    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x0U, .adch_address = 0x0U, 
            .adcsra_address = 0x26U, .adcsrb_address = 0x0U, .admux_address = 0x27U, 
            .vector_index = 16U,
            .didr0_address = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x28U,
            .didr1_address = 0x0U,
            .vector_index = 0U
        } }},
    .timer8_count = 1U,
    .timers8 = {{ {
            .tcnt_address = 0x52U,
            .ocra_address = 0x0U, 
            .ocrb_address = 0x0U,
            .tifr_address = 0x0U,
            .timsk_address = 0x0U,
            .tccra_address = 0x0U, 
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
            .wgm0_mask = 0x0U, .wgm2_mask = 0x0U, .cs_mask = 0x0U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x0U, .compare_b_enable_mask = 0x0U, .overflow_enable_mask = 0x1U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x0U, 
            .ocra_address = 0x0U, 
            .ocrb_address = 0x0U, 
            .icr_address = 0x0U,
            .tifr_address = 0x0U,
            .timsk_address = 0x0U,
            .tccra_address = 0x4FU, 
            .tccrb_address = 0x4EU, 
            .tccrc_address = 0x0U,
            .capture_vector_index = 6U, 
            .compare_a_vector_index = 7U, 
            .compare_b_vector_index = 8U, 
            .overflow_vector_index = 9U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0, 
            .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0,
            .t_pin_address = 0x0U, .t_pin_bit = 0,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x0U, .compare_a_enable_mask = 0x10U, .compare_b_enable_mask = 0x8U, .overflow_enable_mask = 0x4U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x0U, .eicrb_address = 0x0U, .eimsk_address = 0x0U, .eifr_address = 0x0U,
            .vector_indices = {{ 1U, 2U, 3U, 0U, 0U, 0U, 0U, 0U }}
        } }},
    .uart_count = 1U,
    .uarts = {{ {
            .udr_address = 0x2CU, 
            .ucsra_address = 0x2BU, .ucsrb_address = 0x2AU, .ucsrc_address = 0x40U,
            .ubrrl_address = 0x29U, .ubrrh_address = 0x40U,
            .rx_vector_index = 0U, 
            .udre_vector_index = 0U,
            .tx_vector_index = 0U, 
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U,
            .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .pcint_count = 0U,
    .pcints = {{  }},
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x2DU, .spsr_address = 0x2EU, .spdr_address = 0x2FU,
            .vector_index = 12U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U,
            .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .twi_count = 1U,
    .twis = {{ {
            .twbr_address = 0x20U, .twsr_address = 0x21U, .twar_address = 0x22U, .twdr_address = 0x23U, .twcr_address = 0x56U, .twamr_address = 0x0U,
            .vector_index = 19U,
            .twint_mask = 0x80U, .twen_mask = 0x4U, .twie_mask = 0x1U, .twsto_mask = 0x10U, .twsta_mask = 0x20U, .twea_mask = 0x40U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3CU, 
            .eedr_address = 0x3DU, 
            .eearl_address = 0x3EU, 
            .eearh_address = 0x3FU,
            .vector_index = 17U,
            .size = 1024U
        } }},
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x41U,
            .vector_index = 0U,
            .wdie_mask = 0x0U,
            .wde_mask = 0x8U,
            .wdce_mask = 0x0U
        } }},
    
    .fuse_address = 0x2U,
    .lockbit_address = 0x1U,
    .signature_address = 0x3U,
    .port_count = 4U,
    .ports = {{ { "PORTA", 0x39U, 0x3AU, 0x3BU }, { "PORTB", 0x36U, 0x37U, 0x38U }, { "PORTC", 0x33U, 0x34U, 0x35U }, { "PORTD", 0x30U, 0x31U, 0x32U } }}
};

} // namespace vioavr::core::devices
