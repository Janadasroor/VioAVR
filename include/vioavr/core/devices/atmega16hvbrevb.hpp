#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega16hvbrevb {
    .name = "ATmega16HVBrevB",
    .flash_words = 8192U,
    .sram_bytes = 1024U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 29U,
    .interrupt_vector_size = 2U,
    .spl_address = 0x0U,
    .sph_address = 0x0U,
    .sreg_address = 0x5FU,
    .spmcsr_address = 0x0U,
    .prr_address = 0x0U,
    .prr0_address = 0x64U,
    .prr1_address = 0x0U,
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x0U, .adcsrb_address = 0x0U, .admux_address = 0x7CU,
            .vector_index = 23U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x0U,
            .adc_pin_address = {{ 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }},
            .adsc_mask = 0x0U, .adate_mask = 0x0U, .adif_mask = 0x0U, .adie_mask = 0x0U, .aden_mask = 0x0U, .adlar_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .ac_count = 0U,
    .acs = {{  }},
    
    .timer8_count = 0U,
    .timers8 = {{  }},
    
    .timer16_count = 2U,
    .timers16 = {{ {
            .tcnt_address = 0x46U, .ocra_address = 0x48U, .ocrb_address = 0x49U, .icr_address = 0x0U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .tccrc_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0U,
            .wgm10_mask = 0x1U, .wgm12_mask = 0x0U, .cs_mask = 0x4U, .ices_mask = 0x0U, .icnc_mask = 0x0U,
            .capture_enable_mask = 0x8U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x2U
        },
        {
            .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x89U, .icr_address = 0x0U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 0U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0U,
            .wgm10_mask = 0x1U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x0U, .icnc_mask = 0x0U,
            .capture_enable_mask = 0x8U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x4U
        } }},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x69U, .eicrb_address = 0x0U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 3U, 4U, 5U, 6U, 0U, 0U, 0U, 0U }}
        } }},

    .uart_count = 0U,
    .uarts = {{  }},
    
    .pcint_count = 0U,
    .pcints = {{  }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 22U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x64U, .pr_bit = 0x8U
        } }},
    
    .twi_count = 1U,
    .twis = {{ {
            .twbr_address = 0xB8U, .twsr_address = 0xB9U, .twar_address = 0xBAU, .twdr_address = 0xBBU, .twcr_address = 0xBCU, .twamr_address = 0xBDU,
            .vector_index = 20U,
            .twint_mask = 0x80U, .twen_mask = 0x4U, .twie_mask = 0x1U, .twsto_mask = 0x10U, .twsta_mask = 0x20U, .twea_mask = 0x40U,
            .pr_address = 0x64U, .pr_bit = 0x40U
        } }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x0U,
            .vector_index = 27U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .vector_index = 9U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U
        } }},

    .port_count = 3U,
    .ports = {{
        { "PORTA", 0x20U, 0x21U, 0x22U },
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTC", 0x26U, 0x0U, 0x28U }
    }}
};

} // namespace vioavr::core::devices
