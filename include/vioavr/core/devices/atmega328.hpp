#pragma once
#include "vioavr/core/device.hpp"
namespace vioavr::core::devices {
inline constexpr DeviceDescriptor atmega328 = {
    .name = "ATmega328", .flash_words = 0x4000U, .sram_bytes = 0x800U, .eeprom_bytes = 0x400U,
    .interrupt_vector_count = 26, .interrupt_vector_size = 4U, .flash_page_size = 0x80U,
    .register_file_range = { 0x0U, 0x1FU }, .io_range = { 0x20U, 0xFFU }, .extended_io_range = { 0x100U, 0xFFU },
    .spl_address = 0x5DU, .sph_address = 0x5EU, .sreg_address = 0x5FU, .spmcsr_address = 0x57U,
    .prr_address = 0x64U, .prr0_address = 0x0U, .prr1_address = 0x0U,
    .smcr_address = 0x53U, .mcusr_address = 0x54U,
    .pradc_bit = 0, .prusart0_bit = 1, .prspi_bit = 2, .prtwi_bit = 7, .prtimer0_bit = 5, .prtimer1_bit = 3, .prtimer2_bit = 6,
    .smcr_sm_mask = 0xEU, .smcr_se_mask = 0x1U,
    .flash_rww_end_word = 0x0U, .spl_reset = 0x0U, .sph_reset = 0x0U, .sreg_reset = 0x0U, .cpu_frequency_hz = 16000000U,
    .adc_count = 1U, .adcs = {{ {
            .adcl_address = 0x78U, .adch_address = 0x79U, 
            .adcsra_address = 0x7AU, .adcsrb_address = 0x7BU, .admux_address = 0x7CU, 
            .vector_index = 21U, .didr0_address = 0x7EU,
            .pr_address = 0x64U, .pr_bit = 0
        } }}, .ac_count = 1U, .acs = {{ {
            .acsr_address = 0x50U, .didr1_address = 0x7FU, .vector_index = 23U
        } }},
    .timer8_count = 2U, .timers8 = {{ {
            .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x48U,
            .tifr_address = 0x35U,
            .timsk_address = 0x6EU,
            .tccra_address = 0x44U, .tccrb_address = 0x45U, .assr_address = 0x0U,
            .compare_a_vector_index = 14U, .compare_b_vector_index = 15U, .overflow_vector_index = 16U,
            .ocra_pin_address = 0x2BU, .ocra_pin_bit = 6, 
            .ocrb_pin_address = 0x2BU, .ocrb_pin_bit = 5,
            .t_pin_address = 0x2BU, .t_pin_bit = 4,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0,
            .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0,
            .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x2U, .compare_b_enable_mask = 0x4U, .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 5
        }, {
            .tcnt_address = 0xB2U, .ocra_address = 0xB3U, .ocrb_address = 0xB4U,
            .tifr_address = 0x37U,
            .timsk_address = 0x70U,
            .tccra_address = 0xB0U, .tccrb_address = 0xB1U, .assr_address = 0xB6U,
            .compare_a_vector_index = 7U, .compare_b_vector_index = 8U, .overflow_vector_index = 9U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 3, 
            .ocrb_pin_address = 0x2BU, .ocrb_pin_bit = 3,
            .t_pin_address = 0x0U, .t_pin_bit = 0,
            .tosc1_pin_address = 0x25U, .tosc1_pin_bit = 6,
            .tosc2_pin_address = 0x25U, .tosc2_pin_bit = 7,
            .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x20U, .tcn2ub_mask = 0x10U,
            .compare_a_enable_mask = 0x2U, .compare_b_enable_mask = 0x4U, .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 6
        } }}, .timer16_count = 1U, .timers16 = {{ {
            .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x8AU, .icr_address = 0x86U,
            .tifr_address = 0x36U,
            .timsk_address = 0x6FU,
            .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x82U,
            .capture_vector_index = 10U, .compare_a_vector_index = 11U, .compare_b_vector_index = 12U, .overflow_vector_index = 13U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 1, 
            .ocrb_pin_address = 0x25U, .ocrb_pin_bit = 2,
            .icp_pin_address = 0x25U, .icp_pin_bit = 0,
            .t_pin_address = 0x2BU, .t_pin_bit = 5,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x2U, .compare_b_enable_mask = 0x4U, .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 3
        } }},
    .ext_interrupt_count = 1U, .ext_interrupts = {{ {
            .eicra_address = 0x69U, .eicrb_address = 0x0U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 1U, 2U, 0U, 0U, 0U, 0U, 0U, 0U }}
        } }}, .uart_count = 1U, .uarts = {{ {
            .udr_address = 0xC6U, .ucsra_address = 0xC0U, .ucsrb_address = 0xC1U, .ucsrc_address = 0xC2U,
            .ubrrl_address = 0xC4U, .ubrrh_address = 0xC5U,
            .rx_vector_index = 18U, .udre_vector_index = 19U, .tx_vector_index = 20U, 
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U, .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 0x64U, .pr_bit = 1
        } }},
    .pcint_count = 3U, .pcints = {{ {
            .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6BU,
            .pcicr_enable_mask = 0x1U, .pcifr_flag_mask = 0x1U,
            .vector_index = 3U
        }, {
            .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6CU,
            .pcicr_enable_mask = 0x2U, .pcifr_flag_mask = 0x2U,
            .vector_index = 4U
        }, {
            .pcicr_address = 0x68U, .pcifr_address = 0x3BU, .pcmsk_address = 0x6DU,
            .pcicr_enable_mask = 0x4U, .pcifr_flag_mask = 0x4U,
            .vector_index = 5U
        } }}, .spi_count = 1U, .spis = {{ {
            .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU,
            .vector_index = 17U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U,
            .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 2
        } }}, .twi_count = 1U, .twis = {{ {
            .twbr_address = 0xB8U, .twsr_address = 0xB9U, .twar_address = 0xBAU, .twdr_address = 0xBBU, .twcr_address = 0xBCU, .twamr_address = 0xBDU,
            .vector_index = 24U,
            .twint_mask = 0x80U, .twen_mask = 0x4U, .twie_mask = 0x1U, .twsto_mask = 0x10U, .twsta_mask = 0x20U, .twea_mask = 0x40U,
            .pr_address = 0x64U, .pr_bit = 7
        } }}, .eeprom_count = 1U, .eeproms = {{ {
            .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x42U,
            .vector_index = 22U, .size = 1024U
        } }}, .wdt_count = 1U, .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .vector_index = 6U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U, .wdce_mask = 0x10U
        } }},
    .fuse_address = 0x3U, .lockbit_address = 0x1U, .signature_address = 0x3U, .port_count = 3U, .ports = {{ { "PORTB", 0x23U, 0x24U, 0x25U }, { "PORTC", 0x26U, 0x27U, 0x28U }, { "PORTD", 0x29U, 0x2AU, 0x2BU } }}
};
} // namespace vioavr::core::devices
