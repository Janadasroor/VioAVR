#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega8u2 {
    .name = "ATmega8U2",
    .flash_words = 4096U,
    .sram_bytes = 512U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 29U,
    .interrupt_vector_size = 4U,
    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .rampz_address = 0x0U,
    .eind_address = 0x5CU,
    .spmcsr_address = 0x0U,
    .prr_address = 0x65U,
    .prr0_address = 0x64U,
    .prr1_address = 0x65U,
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    
    .adc_count = 0U,
    .adcs = {{  }},
    
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x50U, .didr1_address = 0x7FU, .vector_index = 26U,
            .ain0_pin_address = 0x29U, .ain0_pin_bit = 1U, .ain1_pin_address = 0x29U, .ain1_pin_bit = 2U,
            .aci_mask = 0x10U, .acie_mask = 0x8U
        } }},
    
    .timer8_count = 1U,
    .timers8 = {{ {
            .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x48U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .assr_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 19U,
            .compare_b_vector_index = 20U,
            .overflow_vector_index = 21U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 7U, .ocrb_pin_address = 0x2BU, .ocrb_pin_bit = 0U,
            .t_pin_address = 0x29U, .t_pin_bit = 7U,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U,
            .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x20U
        } }},
    
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x8AU, .icr_address = 0x86U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x82U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 14U,
            .compare_a_vector_index = 15U,
            .compare_b_vector_index = 15U,
            .overflow_vector_index = 18U,
            .ocra_pin_address = 0x28U, .ocra_pin_bit = 6U, .ocrb_pin_address = 0x28U, .ocrb_pin_bit = 5U,
            .icp_pin_address = 0x26U, .icp_pin_bit = 7U,
            .t_pin_address = 0x23U, .t_pin_bit = 4U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x8U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x8U
        } }},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x69U, .eicrb_address = 0x6AU, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U }}
        } }},

    .uart_count = 1U,
    .uarts = {{ {
            .udr_address = 0xCEU, .ucsra_address = 0xC8U, .ucsrb_address = 0xC9U, .ucsrc_address = 0xCAU, .ubrrl_address = 0xCCU, .ubrrh_address = 0xCDU,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 23U,
            .udre_vector_index = 24U,
            .tx_vector_index = 25U,
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U, .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 0x65U, .pr_bit = 0x1U
        } }},
    
    .pcint_count = 2U,
    .pcints = {{ {
            .pcicr_address = 0x68U,
            .pcifr_address = 0x3BU,
            .pcmsk_address = 0x6BU,
            .pcicr_enable_mask = 0x1U, .pcifr_flag_mask = 0x1U,
            .vector_index = 9U
        },
        {
            .pcicr_address = 0x68U,
            .pcifr_address = 0x3BU,
            .pcmsk_address = 0x6CU,
            .pcicr_enable_mask = 0x2U, .pcifr_flag_mask = 0x2U,
            .vector_index = 10U
        } }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 22U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x4U
        } }},
    
    .twi_count = 0U,
    .twis = {{  }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x42U,
            .vector_index = 27U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .vector_index = 13U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U
        } }},

    .port_count = 3U,
    .ports = {{
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTC", 0x26U, 0x27U, 0x28U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU }
    }}
};

} // namespace vioavr::core::devices
