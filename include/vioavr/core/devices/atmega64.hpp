#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega64 {
    .name = "ATmega64",
    .flash_words = 32768U,
    .sram_bytes = 4096U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 35U,
    .interrupt_vector_size = 4U,
    .spl_address = 0x0U,
    .sph_address = 0x0U,
    .sreg_address = 0x5FU,
    .spmcsr_address = 0x0U,
    .prr_address = 0x0U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    .smcr_address = 0x0U,
    .mcusr_address = 0x0U,
    
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x26U, .adcsrb_address = 0x8EU, .admux_address = 0x27U,
            .vector_index = 21U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x0U,
            .adc_pin_address = {{ 0x62U, 0x62U, 0x62U, 0x62U, 0x62U, 0x62U, 0x62U, 0x62U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }},
            .adsc_mask = 0x40U, .adate_mask = 0x20U, .adif_mask = 0x10U, .adie_mask = 0x8U, .aden_mask = 0x80U, .adlar_mask = 0x20U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x28U, .didr1_address = 0x0U, .vector_index = 23U,
            .ain0_pin_address = 0x23U, .ain0_pin_bit = 2U, .ain1_pin_address = 0x23U, .ain1_pin_bit = 3U,
            .aci_mask = 0x10U, .acie_mask = 0x8U
        } }},
    
    .timer8_count = 2U,
    .timers8 = {{ {
            .tcnt_address = 0x44U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .tifr_address = 0x56U, .timsk_address = 0x57U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .assr_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U,
            .wgm0_mask = 0x0U, .wgm2_mask = 0x0U, .cs_mask = 0x0U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x0U,
            .compare_b_enable_mask = 0x0U,
            .overflow_enable_mask = 0x40U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        },
        {
            .tcnt_address = 0x52U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .tifr_address = 0x56U, .timsk_address = 0x57U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .assr_address = 0x50U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U,
            .tosc1_pin_address = 0x65U, .tosc1_pin_bit = 4U, .tosc2_pin_address = 0x65U, .tosc2_pin_bit = 3U,
            .wgm0_mask = 0x0U, .wgm2_mask = 0x0U, .cs_mask = 0x0U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x0U,
            .compare_b_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .timer16_count = 2U,
    .timers16 = {{ {
            .tcnt_address = 0x4CU, .ocra_address = 0x4AU, .ocrb_address = 0x48U, .icr_address = 0x46U, .tifr_address = 0x56U, .timsk_address = 0x57U, .tccra_address = 0x4FU, .tccrb_address = 0x4EU, .tccrc_address = 0x7AU,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x38U, .ocra_pin_bit = 5U, .ocrb_pin_address = 0x38U, .ocrb_pin_bit = 6U,
            .icp_pin_address = 0x32U, .icp_pin_bit = 4U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x10U,
            .compare_b_enable_mask = 0x8U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x4U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        },
        {
            .tcnt_address = 0x88U, .ocra_address = 0x86U, .ocrb_address = 0x84U, .icr_address = 0x80U, .tifr_address = 0x7CU, .timsk_address = 0x7DU, .tccra_address = 0x8BU, .tccrb_address = 0x8AU, .tccrc_address = 0x8CU,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x23U, .ocra_pin_bit = 3U, .ocrb_pin_address = 0x23U, .ocrb_pin_bit = 4U,
            .icp_pin_address = 0x23U, .icp_pin_bit = 7U,
            .wgm10_mask = 0x0U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x10U,
            .compare_b_enable_mask = 0x8U,
            .compare_c_enable_mask = 0x2U,
            .overflow_enable_mask = 0x4U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x6AU, .eicrb_address = 0x5AU, .eimsk_address = 0x59U, .eifr_address = 0x58U,
            .vector_indices = {{ 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U }}
        } }},

    .uart_count = 2U,
    .uarts = {{ {
            .udr_address = 0x2CU, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x29U, .ubrrh_address = 0x90U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 18U,
            .udre_vector_index = 19U,
            .tx_vector_index = 20U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        },
        {
            .udr_address = 0x9CU, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x99U, .ubrrh_address = 0x98U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 30U,
            .udre_vector_index = 31U,
            .tx_vector_index = 32U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .pcint_count = 0U,
    .pcints = {{  }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x2DU, .spsr_address = 0x2EU, .spdr_address = 0x2FU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 17U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .twi_count = 1U,
    .twis = {{ {
            .twbr_address = 0x70U, .twsr_address = 0x71U, .twar_address = 0x72U, .twdr_address = 0x73U, .twcr_address = 0x74U, .twamr_address = 0x0U,
            .vector_index = 33U,
            .twint_mask = 0x80U, .twen_mask = 0x4U, .twie_mask = 0x1U, .twsto_mask = 0x10U, .twsta_mask = 0x20U, .twea_mask = 0x40U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3CU, .eedr_address = 0x3DU, .eearl_address = 0x3EU, .eearh_address = 0x0U,
            .vector_index = 22U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x41U,
            .vector_index = 0U,
            .wdie_mask = 0x0U, .wde_mask = 0x8U
        } }},

    .port_count = 7U,
    .ports = {{
        { "PORTA", 0x39U, 0x3AU, 0x3BU },
        { "PORTB", 0x36U, 0x37U, 0x38U },
        { "PORTC", 0x33U, 0x34U, 0x35U },
        { "PORTD", 0x30U, 0x31U, 0x32U },
        { "PORTE", 0x21U, 0x22U, 0x23U },
        { "PORTF", 0x20U, 0x61U, 0x62U },
        { "PORTG", 0x63U, 0x64U, 0x65U }
    }}
};

} // namespace vioavr::core::devices
