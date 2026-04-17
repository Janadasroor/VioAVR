#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega8hva {
    .name = "ATmega8HVA",
    .flash_words = 4096U,
    .sram_bytes = 512U,
    .eeprom_bytes = 256U,
    .interrupt_vector_count = 21U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 128U,
    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .rampz_address = 0x0U,
    .eind_address = 0x0U,
    .spmcsr_address = 0x57U,
    .prr_address = 0x0U,
    .prr0_address = 0x64U,
    .prr1_address = 0x0U,
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    .mcucr_address = 0x55U,
    .pllcsr_address = 0x0U,
    .xmcra_address = 0x0U,
    .xmcrb_address = 0x0U,
    .xmem = {0},
    .pradc_bit = 0xFFU,
    .prusart0_bit = 0xFFU,
    .prspi_bit = 0x3U,
    .prtwi_bit = 0xFFU,
    .prtimer0_bit = 0x1U,
    .prtimer1_bit = 0x2U,
    .prtimer2_bit = 0xFFU,
    .smcr_sm_mask = 0xEU,
    .smcr_se_mask = 0x1U,
    .flash_rww_end_word = 0x1000U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x0U, .adcsrb_address = 0x0U, .admux_address = 0x0U,
            .vector_index = 16U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x0U,
            .adc_pin_address = {{ 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none }},
            .adsc_mask = 0x0U, .adate_mask = 0x0U, .adif_mask = 0x0U, .adie_mask = 0x0U, .aden_mask = 0x0U, .adlar_mask = 0x0U,
            .adts_mask = 0x7U,
            .pr_address = 0, .pr_bit = 255,
            .mux_table = {{ { 0, 0, 1.0f, false }, { 1, 0, 1.0f, false }, { 2, 0, 1.0f, false }, { 3, 0, 1.0f, false }, { 4, 0, 1.0f, false }, { 5, 0, 1.0f, false }, { 6, 0, 1.0f, false }, { 7, 0, 1.0f, false }, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false} }}
        } }},
    .ac_count = 0U,
    .acs = {{  }},
    .timer8_count = 0U,
    .timers8 = {{  }},
    .timer16_count = 2U,
    .timers16 = {{ {
            .tcnt_address = 0x46U, .ocra_address = 0x48U, .ocrb_address = 0x49U, .ocrc_address = 0x0U, .icr_address = 0x0U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .tccrc_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 11U,
            .compare_a_vector_index = 12U,
            .compare_b_vector_index = 13U,
            .compare_c_vector_index = 0U,
            .overflow_vector_index = 14U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .ocrc_pin_address = 0x0U, .ocrc_pin_bit = 0U,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .wgm10_mask = 0x1U, .wgm12_mask = 0x0U, .cs_mask = 0x4U, .ices_mask = 0x0U, .icnc_mask = 0x0U,
            .capture_enable_mask = 0x8U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
            .foca_mask = 0x0U, .focb_mask = 0x0U, .focc_mask = 0x0U,
            .pr_address = 100, .pr_bit = 1,
            .compare_b_trigger_source = AdcAutoTriggerSource::none,
            .overflow_trigger_source = AdcAutoTriggerSource::none,
            .capture_trigger_source = AdcAutoTriggerSource::none
        },
        {
            .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x89U, .ocrc_address = 0x0U, .icr_address = 0x0U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 0U,
            .compare_a_vector_index = 8U,
            .compare_b_vector_index = 9U,
            .compare_c_vector_index = 0U,
            .overflow_vector_index = 10U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .ocrc_pin_address = 0x0U, .ocrc_pin_bit = 0U,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .wgm10_mask = 0x1U, .wgm12_mask = 0x0U, .cs_mask = 0x7U, .ices_mask = 0x0U, .icnc_mask = 0x0U,
            .capture_enable_mask = 0x8U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
            .foca_mask = 0x0U, .focb_mask = 0x0U, .focc_mask = 0x0U,
            .pr_address = 100, .pr_bit = 2,
            .compare_b_trigger_source = AdcAutoTriggerSource::timer1_compare_b,
            .overflow_trigger_source = AdcAutoTriggerSource::timer1_overflow,
            .capture_trigger_source = AdcAutoTriggerSource::timer1_capture
        } }},
    .timer10_count = 0U,
    .timers10 = {{  }},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x69U, .eicrb_address = 0x0U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 3U, 4U, 5U, 0U, 0U, 0U, 0U, 0U }}
        } }},

    .uart_count = 0U,
    .uarts = {{  }},
    
    .pcint_count = 0U,
    .pcints = {{  }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 15U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 100, .pr_bit = 3
        } }},
    
    .twi_count = 0U,
    .twis = {{  }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x41U,
            .eecr_reset = 0x0U,
            .vector_index = 20U,
            .size = 0x100U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .wdtcsr_reset = 0x0U,
            .vector_index = 6U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U, .wdce_mask = 0x10U
        } }},

    .can_count = 0U,
    .cans = {{  }},
    
    .usb_count = 0U,
    .usbs = {{  }},

    .psc_count = 0U,
    .pscs = {{  }},

    .dac_count = 0U,
    .dacs = {{  }},

    .fuse_address = 0x0U,
    .lockbit_address = 0x0U,
    .signature_address = 0x0U,

    .port_count = 3U,
    .ports = {{
        { "PORTA", 0x20U, 0x21U, 0x22U },
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTC", 0x26U, 0x0U, 0x28U }
    }}
};

} // namespace vioavr::core::devices
