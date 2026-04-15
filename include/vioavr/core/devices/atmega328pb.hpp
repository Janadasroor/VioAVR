#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega328pb {
    .name = "ATmega328PB",
    .flash_words = 16384U,
    .sram_bytes = 2048U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 45U,
    .interrupt_vector_size = 4U,
    .spl_address = 0x0U,
    .sph_address = 0x0U,
    .sreg_address = 0x5FU,
    .spmcsr_address = 0x57U,
    .prr_address = 0x0U,
    .prr0_address = 0x64U,
    .prr1_address = 0x65U,
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x7AU, .adcsrb_address = 0x7BU, .admux_address = 0x7CU,
            .vector_index = 21U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x7EU,
            .adc_pin_address = {{ 0x28U, 0x28U, 0x28U, 0x28U, 0x28U, 0x28U, 0x2EU, 0x2EU, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 0U, 1U, 2U, 3U, 4U, 5U, 2U, 3U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }},
            .adsc_mask = 0x40U, .adate_mask = 0x20U, .adif_mask = 0x10U, .adie_mask = 0x8U, .aden_mask = 0x80U, .adlar_mask = 0x20U,
            .pr_address = 0x64U, .pr_bit = 0x1U
        } }},
    
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x50U, .didr1_address = 0x7FU, .vector_index = 23U,
            .ain0_pin_address = 0x2BU, .ain0_pin_bit = 6U, .ain1_pin_address = 0x2BU, .ain1_pin_bit = 7U,
            .aci_mask = 0x0U, .acie_mask = 0x0U
        } }},
    
    .timer8_count = 2U,
    .timers8 = {{ {
            .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x48U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .assr_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x2BU, .ocra_pin_bit = 6U, .ocrb_pin_address = 0x2BU, .ocrb_pin_bit = 5U,
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
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 3U, .ocrb_pin_address = 0x2BU, .ocrb_pin_bit = 3U,
            .tosc1_pin_address = 0x25U, .tosc1_pin_bit = 6U, .tosc2_pin_address = 0x25U, .tosc2_pin_bit = 7U,
            .wgm0_mask = 0x0U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x20U, .tcn2ub_mask = 0x10U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x40U
        } }},
    
    .timer16_count = 3U,
    .timers16 = {{ {
            .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x8AU, .icr_address = 0x86U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x82U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 1U, .ocrb_pin_address = 0x25U, .ocrb_pin_bit = 2U,
            .icp_pin_address = 0x25U, .icp_pin_bit = 0U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x0U,
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
            .ocra_pin_address = 0x2BU, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x2BU, .ocrb_pin_bit = 2U,
            .icp_pin_address = 0x2EU, .icp_pin_bit = 2U,
            .wgm10_mask = 0x0U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x65U, .pr_bit = 0x1U
        },
        {
            .tcnt_address = 0xA4U, .ocra_address = 0xA8U, .ocrb_address = 0xAAU, .icr_address = 0xA6U, .tifr_address = 0x39U, .timsk_address = 0x72U, .tccra_address = 0xA0U, .tccrb_address = 0xA1U, .tccrc_address = 0xA2U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x2BU, .ocra_pin_bit = 1U, .ocrb_pin_address = 0x2BU, .ocrb_pin_bit = 2U,
            .icp_pin_address = 0x2EU, .icp_pin_bit = 0U,
            .wgm10_mask = 0x0U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x65U, .pr_bit = 0x8U
        } }},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x69U, .eicrb_address = 0x0U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 1U, 2U, 0U, 0U, 0U, 0U, 0U, 0U }}
        } }},

    .uart_count = 2U,
    .uarts = {{ {
            .udr_address = 0xC6U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 18U,
            .udre_vector_index = 19U,
            .tx_vector_index = 20U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x64U, .pr_bit = 0x10U
        },
        {
            .udr_address = 0xCEU, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 28U,
            .udre_vector_index = 29U,
            .tx_vector_index = 30U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x64U, .pr_bit = 0x10U
        } }},
    
    .pcint_count = 0U,
    .pcints = {{  }},
    
    .spi_count = 2U,
    .spis = {{ {
            .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 17U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x4U
        },
        {
            .spcr_address = 0xACU, .spsr_address = 0xADU, .spdr_address = 0xAEU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 17U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x4U
        } }},
    
    .twi_count = 2U,
    .twis = {{ {
            .twbr_address = 0xB8U, .twsr_address = 0xB9U, .twar_address = 0xBAU, .twdr_address = 0xBBU, .twcr_address = 0xBCU, .twamr_address = 0xBDU,
            .vector_index = 24U,
            .twint_mask = 0x80U, .twen_mask = 0x4U, .twie_mask = 0x1U, .twsto_mask = 0x10U, .twsta_mask = 0x20U, .twea_mask = 0x40U,
            .pr_address = 0x64U, .pr_bit = 0x80U
        },
        {
            .twbr_address = 0xD8U, .twsr_address = 0xD9U, .twar_address = 0xDAU, .twdr_address = 0xDBU, .twcr_address = 0xDCU, .twamr_address = 0xDDU,
            .vector_index = 24U,
            .twint_mask = 0x80U, .twen_mask = 0x4U, .twie_mask = 0x1U, .twsto_mask = 0x10U, .twsta_mask = 0x20U, .twea_mask = 0x40U,
            .pr_address = 0x64U, .pr_bit = 0x80U
        } }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x0U,
            .vector_index = 22U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .vector_index = 6U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U
        } }},

    .port_count = 4U,
    .ports = {{
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTC", 0x26U, 0x27U, 0x28U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU },
        { "PORTE", 0x2CU, 0x2DU, 0x2EU }
    }}
};

} // namespace vioavr::core::devices
