#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega2560 {
    .name = "ATmega2560",
    .flash_words = 131072U,
    .sram_bytes = 8192U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 57U,
    .interrupt_vector_size = 4U,
    .spl_address = 0x0U,
    .sph_address = 0x0U,
    .sreg_address = 0x5FU,
    .spmcsr_address = 0x0U,
    .prr_address = 0x0U,
    .prr0_address = 0x64U,
    .prr1_address = 0x65U,
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x7AU, .adcsrb_address = 0x7BU, .admux_address = 0x7CU,
            .vector_index = 29U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x7EU,
            .adc_pin_address = {{ 0x31U, 0x31U, 0x31U, 0x31U, 0x31U, 0x31U, 0x31U, 0x31U, 0x108U, 0x108U, 0x108U, 0x108U, 0x108U, 0x108U, 0x108U, 0x108U }},
            .adc_pin_bit = {{ 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }},
            .adsc_mask = 0x40U, .adate_mask = 0x20U, .adif_mask = 0x10U, .adie_mask = 0x8U, .aden_mask = 0x80U, .adlar_mask = 0x20U,
            .pr_address = 0x64U, .pr_bit = 0x1U
        } }},
    
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x50U, .didr1_address = 0x7FU, .vector_index = 28U,
            .ain0_pin_address = 0x2EU, .ain0_pin_bit = 2U, .ain1_pin_address = 0x2EU, .ain1_pin_bit = 3U,
            .aci_mask = 0x10U, .acie_mask = 0x8U
        } }},
    
    .timer8_count = 2U,
    .timers8 = {{ {
            .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x48U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .assr_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 7U, .ocrb_pin_address = 0x34U, .ocrb_pin_bit = 5U,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U,
            .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x20U
        },
        {
            .tcnt_address = 0xB2U, .ocra_address = 0xB3U, .ocrb_address = 0xB4U, .tifr_address = 0x37U, .timsk_address = 0x70U, .tccra_address = 0xB0U, .tccrb_address = 0xB1U, .assr_address = 0xB6U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 4U, .ocrb_pin_address = 0x102U, .ocrb_pin_bit = 6U,
            .tosc1_pin_address = 0x34U, .tosc1_pin_bit = 4U, .tosc2_pin_address = 0x34U, .tosc2_pin_bit = 3U,
            .wgm0_mask = 0x0U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x20U, .tcn2ub_mask = 0x10U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x40U
        } }},
    
    .timer16_count = 4U,
    .timers16 = {{ {
            .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x8AU, .icr_address = 0x86U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x82U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 5U, .ocrb_pin_address = 0x25U, .ocrb_pin_bit = 6U,
            .icp_pin_address = 0x2BU, .icp_pin_bit = 4U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x8U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x8U
        },
        {
            .tcnt_address = 0x94U, .ocra_address = 0x98U, .ocrb_address = 0x9AU, .icr_address = 0x96U, .tifr_address = 0x38U, .timsk_address = 0x71U, .tccra_address = 0x90U, .tccrb_address = 0x91U, .tccrc_address = 0x92U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x2EU, .ocra_pin_bit = 3U, .ocrb_pin_address = 0x2EU, .ocrb_pin_bit = 4U,
            .icp_pin_address = 0x2EU, .icp_pin_bit = 7U,
            .wgm10_mask = 0x0U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x8U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x65U, .pr_bit = 0x8U
        },
        {
            .tcnt_address = 0xA4U, .ocra_address = 0xA8U, .ocrb_address = 0xAAU, .icr_address = 0xA6U, .tifr_address = 0x39U, .timsk_address = 0x72U, .tccra_address = 0xA0U, .tccrb_address = 0xA1U, .tccrc_address = 0xA2U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x102U, .ocra_pin_bit = 3U, .ocrb_pin_address = 0x102U, .ocrb_pin_bit = 4U,
            .icp_pin_address = 0x10BU, .icp_pin_bit = 0U,
            .wgm10_mask = 0x0U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x8U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x65U, .pr_bit = 0x10U
        },
        {
            .tcnt_address = 0x124U, .ocra_address = 0x128U, .ocrb_address = 0x12AU, .icr_address = 0x126U, .tifr_address = 0x3AU, .timsk_address = 0x73U, .tccra_address = 0x120U, .tccrb_address = 0x121U, .tccrc_address = 0x122U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x10BU, .ocra_pin_bit = 3U, .ocrb_pin_address = 0x10BU, .ocrb_pin_bit = 4U,
            .icp_pin_address = 0x10BU, .icp_pin_bit = 1U,
            .wgm10_mask = 0x0U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x8U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x65U, .pr_bit = 0x20U
        } }},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x69U, .eicrb_address = 0x6AU, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U }}
        } }},

    .uart_count = 4U,
    .uarts = {{ {
            .udr_address = 0xC6U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 25U,
            .udre_vector_index = 26U,
            .tx_vector_index = 27U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x65U, .pr_bit = 0x4U
        },
        {
            .udr_address = 0xCEU, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 36U,
            .udre_vector_index = 37U,
            .tx_vector_index = 38U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x65U, .pr_bit = 0x4U
        },
        {
            .udr_address = 0xD6U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 51U,
            .udre_vector_index = 52U,
            .tx_vector_index = 53U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x65U, .pr_bit = 0x4U
        },
        {
            .udr_address = 0x136U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 54U,
            .udre_vector_index = 55U,
            .tx_vector_index = 56U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x65U, .pr_bit = 0x4U
        } }},
    
    .pcint_count = 0U,
    .pcints = {{  }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 24U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x4U
        } }},
    
    .twi_count = 1U,
    .twis = {{ {
            .twbr_address = 0xB8U, .twsr_address = 0xB9U, .twar_address = 0xBAU, .twdr_address = 0xBBU, .twcr_address = 0xBCU, .twamr_address = 0xBDU,
            .vector_index = 39U,
            .twint_mask = 0x80U, .twen_mask = 0x4U, .twie_mask = 0x1U, .twsto_mask = 0x10U, .twsta_mask = 0x20U, .twea_mask = 0x40U,
            .pr_address = 0x64U, .pr_bit = 0x80U
        } }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x0U,
            .vector_index = 30U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .vector_index = 12U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U
        } }},

    .port_count = 11U,
    .ports = {{
        { "PORTA", 0x20U, 0x21U, 0x22U },
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTC", 0x26U, 0x27U, 0x28U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU },
        { "PORTE", 0x2CU, 0x2DU, 0x2EU },
        { "PORTF", 0x2FU, 0x30U, 0x31U },
        { "PORTG", 0x32U, 0x33U, 0x34U },
        { "PORTH", 0x100U, 0x101U, 0x102U },
        { "PORTJ", 0x103U, 0x104U, 0x105U },
        { "PORTK", 0x106U, 0x107U, 0x108U },
        { "PORTL", 0x109U, 0x10AU, 0x10BU }
    }}
};

} // namespace vioavr::core::devices
