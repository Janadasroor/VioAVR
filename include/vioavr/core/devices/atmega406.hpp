#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega406 = {
    .name = "ATmega406",
    .flash_words = 0x5000U,
    .sram_bytes = 0x800U,
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
    
    .pradc_bit = 0xFFU,
    .prusart0_bit = 0xFFU,
    .prspi_bit = 0xFFU,
    .prtwi_bit = 0x8U,
    .prtimer0_bit = 0x2U,
    .prtimer1_bit = 0x4U,
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
            .adcsra_address = 0x0U, .adcsrb_address = 0x0U, .admux_address = 0x0U, 
            .vector_index = 17U,
            .didr0_address = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .ac_count = 0U,
    .acs = {{  }},
    .timer8_count = 1U,
    .timers8 = {{ {
            .tcnt_address = 0x46U,
            .ocra_address = 0x47U, 
            .ocrb_address = 0x48U,
            .tifr_address = 0x35U,
            .timsk_address = 0x6EU,
            .tccra_address = 0x44U, 
            .tccrb_address = 0x45U, 
            .assr_address = 0x0U,
            .compare_a_vector_index = 0U, 
            .compare_b_vector_index = 0U, 
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0, 
            .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0,
            .t_pin_address = 0x0U, .t_pin_bit = 0,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0,
            .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0,
            .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x2U, .compare_b_enable_mask = 0x4U, .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x2U
        } }},
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x0U, 
            .ocra_address = 0x88U, 
            .ocrb_address = 0x0U, 
            .icr_address = 0x0U,
            .tifr_address = 0x36U,
            .timsk_address = 0x6FU,
            .tccra_address = 0x0U, 
            .tccrb_address = 0x81U, 
            .tccrc_address = 0x0U,
            .capture_vector_index = 0U, 
            .compare_a_vector_index = 0U, 
            .compare_b_vector_index = 0U, 
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0, 
            .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0,
            .t_pin_address = 0x0U, .t_pin_bit = 0,
            .wgm10_mask = 0x0U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x0U, .icnc_mask = 0x0U,
            .capture_enable_mask = 0x0U, .compare_a_enable_mask = 0x2U, .compare_b_enable_mask = 0x0U, .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x4U
        } }},
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x69U, .eicrb_address = 0x0U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 2U, 3U, 4U, 5U, 0U, 0U, 0U, 0U }}
        } }},
    .uart_count = 0U,
    .uarts = {{  }},
    .pcint_count = 0U,
    .pcints = {{  }},
    .spi_count = 0U,
    .spis = {{  }},
    .twi_count = 1U,
    .twis = {{ {
            .twbr_address = 0xB8U, .twsr_address = 0xB9U, .twar_address = 0xBAU, .twdr_address = 0xBBU, .twcr_address = 0xBCU, .twamr_address = 0xBDU,
            .vector_index = 15U,
            .twint_mask = 0x80U, .twen_mask = 0x4U, .twie_mask = 0x1U, .twsto_mask = 0x10U, .twsta_mask = 0x20U, .twea_mask = 0x40U,
            .pr_address = 0x64U, .pr_bit = 0x8U
        } }},
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3FU, 
            .eedr_address = 0x40U, 
            .eearl_address = 0x41U, 
            .eearh_address = 0x42U,
            .vector_index = 21U,
            .size = 512U
        } }},
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .vector_index = 8U,
            .wdie_mask = 0x40U,
            .wde_mask = 0x8U,
            .wdce_mask = 0x10U
        } }},
    
    .fuse_address = 0x2U,
    .lockbit_address = 0x1U,
    .signature_address = 0x3U,
    .port_count = 3U,
    .ports = {{ { "PORTA", 0x20U, 0x21U, 0x22U }, { "PORTB", 0x23U, 0x24U, 0x25U }, { "PORTD", 0x29U, 0x2AU, 0x2BU } }}
};

} // namespace vioavr::core::devices
