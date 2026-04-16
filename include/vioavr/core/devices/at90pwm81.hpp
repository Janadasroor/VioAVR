#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor at90pwm81 = {
    .name = "AT90PWM81",
    .flash_words = 0x1000U,
    .sram_bytes = 0x100U,
    .eeprom_bytes = 0x200U,
    .interrupt_vector_count = 20,
    .interrupt_vector_size = 4U,
    .flash_page_size = 0x40U,
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
    .prusart0_bit = 0xFFU,
    .prspi_bit = 0x4U,
    .prtwi_bit = 0xFFU,
    .prtimer0_bit = 0xFFU,
    .prtimer1_bit = 0x10U,
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
            .adcsra_address = 0x26U, .adcsrb_address = 0x27U, .admux_address = 0x28U, 
            .vector_index = 13U,
            .didr0_address = 0x77U,
            .pr_address = 0x86U, .pr_bit = 0x1U
        } }},
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x20U,
            .didr1_address = 0x0U,
            .vector_index = 7U
        } }},
    .timer8_count = 0U,
    .timers8 = {{  }},
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x0U, 
            .ocra_address = 0x0U, 
            .ocrb_address = 0x0U, 
            .icr_address = 0x0U,
            .tifr_address = 0x22U,
            .timsk_address = 0x21U,
            .tccra_address = 0x0U, 
            .tccrb_address = 0x8AU, 
            .tccrc_address = 0x0U,
            .capture_vector_index = 11U, 
            .compare_a_vector_index = 0U, 
            .compare_b_vector_index = 0U, 
            .overflow_vector_index = 12U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0, 
            .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0,
            .t_pin_address = 0x0U, .t_pin_bit = 0,
            .wgm10_mask = 0x0U, .wgm12_mask = 0x10U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U, .compare_a_enable_mask = 0x0U, .compare_b_enable_mask = 0x0U, .overflow_enable_mask = 0x1U,
            .pr_address = 0x86U, .pr_bit = 0x10U
        } }},
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x89U, .eicrb_address = 0x0U, .eimsk_address = 0x41U, .eifr_address = 0x40U,
            .vector_indices = {{ 10U, 14U, 16U, 0U, 0U, 0U, 0U, 0U }}
        } }},
    .uart_count = 0U,
    .uarts = {{  }},
    .pcint_count = 0U,
    .pcints = {{  }},
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x37U, .spsr_address = 0x38U, .spdr_address = 0x56U,
            .vector_index = 15U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U,
            .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x86U, .pr_bit = 0x4U
        } }},
    .twi_count = 0U,
    .twis = {{  }},
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3CU, 
            .eedr_address = 0x3DU, 
            .eearl_address = 0x3EU, 
            .eearh_address = 0x3FU,
            .vector_index = 3U,
            .size = 512U
        } }},
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x82U,
            .vector_index = 17U,
            .wdie_mask = 0x40U,
            .wde_mask = 0x8U,
            .wdce_mask = 0x10U
        } }},
    
    .fuse_address = 0x3U,
    .lockbit_address = 0x1U,
    .signature_address = 0x3U,
    .port_count = 3U,
    .ports = {{ { "PORTB", 0x23U, 0x24U, 0x25U }, { "PORTD", 0x29U, 0x2AU, 0x2BU }, { "PORTE", 0x2CU, 0x2DU, 0x2EU } }}
};

} // namespace vioavr::core::devices
