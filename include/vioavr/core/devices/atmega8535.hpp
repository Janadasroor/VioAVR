#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega8535 {
    .name = "ATmega8535",
    .flash_words = 4096U,
    .sram_bytes = 512U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 21U,
    .interrupt_vector_size = 4U,
    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .rampz_address = 0x0U,
    .eind_address = 0x0U,
    .spmcsr_address = 0x57U,
    .prr_address = 0x0U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    .smcr_address = 0x0U,
    .mcusr_address = 0x0U,
    .mcucr_address = 0x55U,
    .xmcra_address = 0x0U,
    .xmcrb_address = 0x0U,
    .xmem = {
            .xmcra_address = 0x0U, .xmcrb_address = 0x0U,
            .mcucr_address = 0x55U, .sre_mask = 0x80U
        },
    
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x26U, .adch_address = 0x26U, .adcsra_address = 0x26U, .adcsrb_address = 0x0U, .admux_address = 0x27U,
            .vector_index = 14U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x0U,
            .adc_pin_address = {{ 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare_a, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }},
            .adsc_mask = 0x40U, .adate_mask = 0x20U, .adif_mask = 0x10U, .adie_mask = 0x8U, .aden_mask = 0x80U, .adlar_mask = 0x20U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x28U, .didr1_address = 0x0U, .vector_index = 0U,
            .ain0_pin_address = 0x0U, .ain0_pin_bit = 0U, .ain1_pin_address = 0x0U, .ain1_pin_bit = 0U,
            .aci_mask = 0x10U, .acie_mask = 0x8U
        } }},
    
    .timer8_count = 2U,
    .timers8 = {{ {
            .tcnt_address = 0x52U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .tifr_address = 0x58U, .timsk_address = 0x59U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .assr_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 9U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U,
            .wgm0_mask = 0x0U, .wgm2_mask = 0x0U, .cs_mask = 0x0U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x0U,
            .compare_b_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        },
        {
            .tcnt_address = 0x44U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .tifr_address = 0x58U, .timsk_address = 0x59U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .assr_address = 0x42U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 4U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U,
            .wgm0_mask = 0x0U, .wgm2_mask = 0x0U, .cs_mask = 0x0U,
            .as2_mask = 0x8U, .tcn2ub_mask = 0x4U,
            .compare_a_enable_mask = 0x0U,
            .compare_b_enable_mask = 0x0U,
            .overflow_enable_mask = 0x40U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x4CU, .ocra_address = 0x4AU, .ocrb_address = 0x48U, .ocrc_address = 0x0U, .icr_address = 0x46U, .tifr_address = 0x58U, .timsk_address = 0x59U, .tccra_address = 0x4FU, .tccrb_address = 0x4EU, .tccrc_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 5U,
            .compare_a_vector_index = 6U,
            .compare_b_vector_index = 7U,
            .compare_c_vector_index = 0U,
            .overflow_vector_index = 8U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .ocrc_pin_address = 0x0U, .ocrc_pin_bit = 0U,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x10U,
            .compare_b_enable_mask = 0x8U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x4U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x0U, .eicrb_address = 0x0U, .eimsk_address = 0x0U, .eifr_address = 0x0U,
            .vector_indices = {{ 1U, 2U, 18U, 0U, 0U, 0U, 0U, 0U }}
        } }},

    .uart_count = 1U,
    .uarts = {{ {
            .udr_address = 0x2CU, .ucsra_address = 0x2BU, .ucsrb_address = 0x2AU, .ucsrc_address = 0x40U, .ubrrl_address = 0x40U, .ubrrh_address = 0x40U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 11U,
            .udre_vector_index = 12U,
            .tx_vector_index = 13U,
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U, .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .pcint_count = 0U,
    .pcints = {{  }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x2DU, .spsr_address = 0x2EU, .spdr_address = 0x2FU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 10U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .twi_count = 1U,
    .twis = {{ {
            .twbr_address = 0x20U, .twsr_address = 0x21U, .twar_address = 0x22U, .twdr_address = 0x23U, .twcr_address = 0x56U, .twamr_address = 0x0U,
            .vector_index = 17U,
            .twint_mask = 0x80U, .twen_mask = 0x4U, .twie_mask = 0x1U, .twsto_mask = 0x10U, .twsta_mask = 0x20U, .twea_mask = 0x40U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3CU, .eedr_address = 0x3DU, .eearl_address = 0x3EU, .eearh_address = 0x3FU,
            .vector_index = 0U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x41U,
            .vector_index = 0U,
            .wdie_mask = 0x0U, .wde_mask = 0x8U
        } }},

    .can_count = 0U,
    .cans = {{  }},

    .port_count = 4U,
    .ports = {{
        { "PORTA", 0x39U, 0x3AU, 0x3BU },
        { "PORTB", 0x36U, 0x37U, 0x38U },
        { "PORTC", 0x33U, 0x34U, 0x35U },
        { "PORTD", 0x30U, 0x31U, 0x32U }
    }}
};

} // namespace vioavr::core::devices
