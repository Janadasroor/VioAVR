#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega1280 {
    .name = "ATmega1280",
    .flash_words = 65536U,
    .sram_bytes = 8192U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 57U,
    .interrupt_vector_size = 4U,
    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .rampz_address = 0x5BU,
    .eind_address = 0x5CU,
    .spmcsr_address = 0x0U,
    .prr_address = 0x65U,
    .prr0_address = 0x64U,
    .prr1_address = 0x65U,
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    .mcucr_address = 0x55U,
    .xmcra_address = 0x74U,
    .xmcrb_address = 0x75U,
    .xmem = {
            .xmcra_address = 0x74U, .xmcrb_address = 0x75U,
            .mcucr_address = 0x55U, .sre_mask = 0x80U
        },
    
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x78U, .adch_address = 0x79U, .adcsra_address = 0x7AU, .adcsrb_address = 0x7BU, .admux_address = 0x7CU,
            .vector_index = 29U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x7EU,
            .adc_pin_address = {{ 0x2FU, 0x2FU, 0x2FU, 0x2FU, 0x2FU, 0x2FU, 0x2FU, 0x2FU, 0x106U, 0x106U, 0x106U, 0x106U, 0x106U, 0x106U, 0x106U, 0x106U }},
            .adc_pin_bit = {{ 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare_a, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }},
            .adsc_mask = 0x40U, .adate_mask = 0x20U, .adif_mask = 0x10U, .adie_mask = 0x8U, .aden_mask = 0x80U, .adlar_mask = 0x20U,
            .pr_address = 0x64U, .pr_bit = 0x1U
        } }},
    
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x50U, .didr1_address = 0x7FU, .vector_index = 28U,
            .ain0_pin_address = 0x2CU, .ain0_pin_bit = 2U, .ain1_pin_address = 0x2CU, .ain1_pin_bit = 3U,
            .aci_mask = 0x10U, .acie_mask = 0x8U
        } }},
    
    .timer8_count = 2U,
    .timers8 = {{ {
            .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x48U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .assr_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 21U,
            .compare_b_vector_index = 22U,
            .overflow_vector_index = 23U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 7U, .ocrb_pin_address = 0x34U, .ocrb_pin_bit = 5U,
            .t_pin_address = 0x29U, .t_pin_bit = 7U,
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
            .compare_a_vector_index = 13U,
            .compare_b_vector_index = 14U,
            .overflow_vector_index = 15U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 4U, .ocrb_pin_address = 0x102U, .ocrb_pin_bit = 6U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .tosc1_pin_address = 0x32U, .tosc1_pin_bit = 4U, .tosc2_pin_address = 0x32U, .tosc2_pin_bit = 3U,
            .wgm0_mask = 0x0U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x20U, .tcn2ub_mask = 0x10U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x40U
        } }},
    
    .timer16_count = 4U,
    .timers16 = {{ {
            .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x8AU, .ocrc_address = 0x8CU, .icr_address = 0x86U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x82U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 16U,
            .compare_a_vector_index = 17U,
            .compare_b_vector_index = 18U,
            .compare_c_vector_index = 19U,
            .overflow_vector_index = 20U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 5U, .ocrb_pin_address = 0x25U, .ocrb_pin_bit = 6U, .ocrc_pin_address = 0x25U, .ocrc_pin_bit = 7U,
            .icp_pin_address = 0x29U, .icp_pin_bit = 4U,
            .t_pin_address = 0x29U, .t_pin_bit = 6U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x8U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x8U
        },
        {
            .tcnt_address = 0x94U, .ocra_address = 0x98U, .ocrb_address = 0x9AU, .ocrc_address = 0x9CU, .icr_address = 0x96U, .tifr_address = 0x38U, .timsk_address = 0x71U, .tccra_address = 0x90U, .tccrb_address = 0x91U, .tccrc_address = 0x92U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 31U,
            .compare_a_vector_index = 32U,
            .compare_b_vector_index = 33U,
            .compare_c_vector_index = 34U,
            .overflow_vector_index = 35U,
            .ocra_pin_address = 0x2EU, .ocra_pin_bit = 3U, .ocrb_pin_address = 0x2EU, .ocrb_pin_bit = 4U, .ocrc_pin_address = 0x2EU, .ocrc_pin_bit = 5U,
            .icp_pin_address = 0x2CU, .icp_pin_bit = 7U,
            .t_pin_address = 0x2CU, .t_pin_bit = 6U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x8U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x65U, .pr_bit = 0x8U
        },
        {
            .tcnt_address = 0xA4U, .ocra_address = 0xA8U, .ocrb_address = 0xAAU, .ocrc_address = 0xACU, .icr_address = 0xA6U, .tifr_address = 0x39U, .timsk_address = 0x72U, .tccra_address = 0xA0U, .tccrb_address = 0xA1U, .tccrc_address = 0xA2U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 41U,
            .compare_a_vector_index = 42U,
            .compare_b_vector_index = 43U,
            .compare_c_vector_index = 44U,
            .overflow_vector_index = 45U,
            .ocra_pin_address = 0x102U, .ocra_pin_bit = 3U, .ocrb_pin_address = 0x102U, .ocrb_pin_bit = 4U, .ocrc_pin_address = 0x102U, .ocrc_pin_bit = 5U,
            .icp_pin_address = 0x109U, .icp_pin_bit = 0U,
            .t_pin_address = 0x100U, .t_pin_bit = 7U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x8U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x65U, .pr_bit = 0x10U
        },
        {
            .tcnt_address = 0x124U, .ocra_address = 0x128U, .ocrb_address = 0x12AU, .ocrc_address = 0x12CU, .icr_address = 0x126U, .tifr_address = 0x3AU, .timsk_address = 0x73U, .tccra_address = 0x120U, .tccrb_address = 0x121U, .tccrc_address = 0x122U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 46U,
            .compare_a_vector_index = 47U,
            .compare_b_vector_index = 48U,
            .compare_c_vector_index = 49U,
            .overflow_vector_index = 50U,
            .ocra_pin_address = 0x10BU, .ocra_pin_bit = 3U, .ocrb_pin_address = 0x10BU, .ocrb_pin_bit = 4U, .ocrc_pin_address = 0x10BU, .ocrc_pin_bit = 5U,
            .icp_pin_address = 0x109U, .icp_pin_bit = 1U,
            .t_pin_address = 0x109U, .t_pin_bit = 2U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
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
            .udr_address = 0xC6U, .ucsra_address = 0xC0U, .ucsrb_address = 0xC1U, .ucsrc_address = 0xC2U, .ubrrl_address = 0xC4U, .ubrrh_address = 0xC5U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 25U,
            .udre_vector_index = 26U,
            .tx_vector_index = 27U,
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U, .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 0x65U, .pr_bit = 0x4U
        },
        {
            .udr_address = 0xCEU, .ucsra_address = 0xC8U, .ucsrb_address = 0xC9U, .ucsrc_address = 0xCAU, .ubrrl_address = 0xCCU, .ubrrh_address = 0xCDU,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 25U,
            .udre_vector_index = 26U,
            .tx_vector_index = 27U,
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U, .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 0x65U, .pr_bit = 0x4U
        },
        {
            .udr_address = 0xD6U, .ucsra_address = 0xD0U, .ucsrb_address = 0xD1U, .ucsrc_address = 0xD2U, .ubrrl_address = 0xD4U, .ubrrh_address = 0xD5U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 25U,
            .udre_vector_index = 26U,
            .tx_vector_index = 27U,
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U, .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 0x65U, .pr_bit = 0x4U
        },
        {
            .udr_address = 0x136U, .ucsra_address = 0x130U, .ucsrb_address = 0x131U, .ucsrc_address = 0x132U, .ubrrl_address = 0x134U, .ubrrh_address = 0x135U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 25U,
            .udre_vector_index = 26U,
            .tx_vector_index = 27U,
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U, .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 0x65U, .pr_bit = 0x4U
        } }},
    
    .pcint_count = 3U,
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
        },
        {
            .pcicr_address = 0x68U,
            .pcifr_address = 0x3BU,
            .pcmsk_address = 0x6DU,
            .pcicr_enable_mask = 0x4U, .pcifr_flag_mask = 0x4U,
            .vector_index = 11U
        } }},
    
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
            .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x42U,
            .vector_index = 30U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .vector_index = 12U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U
        } }},

    .can_count = 0U,
    .cans = {{  }},

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
