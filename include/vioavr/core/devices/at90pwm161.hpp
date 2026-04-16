#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor at90pwm161 {
    .name = "AT90PWM161",
    .flash_words = 8192U,
    .sram_bytes = 1024U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 20U,
    .interrupt_vector_size = 4U,
    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .rampz_address = 0x0U,
    .eind_address = 0x0U,
    .spmcsr_address = 0x0U,
    .prr_address = 0x86U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    .mcucr_address = 0x55U,
    .xmcra_address = 0x0U,
    .xmcrb_address = 0x0U,
    .xmem = {
            .xmcra_address = 0x0U, .xmcrb_address = 0x0U,
            .mcucr_address = 0x55U, .sre_mask = 0x80U
        },
    
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x26U, .adch_address = 0x26U, .adcsra_address = 0x26U, .adcsrb_address = 0x27U, .admux_address = 0x28U,
            .vector_index = 13U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x77U,
            .adc_pin_address = {{ 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }},
            .adsc_mask = 0x40U, .adate_mask = 0x20U, .adif_mask = 0x10U, .adie_mask = 0x8U, .aden_mask = 0x80U, .adlar_mask = 0x20U,
            .pr_address = 0x86U, .pr_bit = 0x1U
        } }},
    
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x20U, .didr1_address = 0x0U, .vector_index = 7U,
            .ain0_pin_address = 0x0U, .ain0_pin_bit = 0U, .ain1_pin_address = 0x0U, .ain1_pin_bit = 0U,
            .aci_mask = 0x0U, .acie_mask = 0x0U
        } }},
    
    .timer8_count = 0U,
    .timers8 = {{  }},
    
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x5AU, .ocra_address = 0x0U, .ocrb_address = 0x0U, .ocrc_address = 0x0U, .icr_address = 0x8CU, .tifr_address = 0x22U, .timsk_address = 0x21U, .tccra_address = 0x0U, .tccrb_address = 0x8AU, .tccrc_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 11U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .compare_c_vector_index = 0U,
            .overflow_vector_index = 12U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .ocrc_pin_address = 0x0U, .ocrc_pin_bit = 0U,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .wgm10_mask = 0x0U, .wgm12_mask = 0x10U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x0U,
            .compare_b_enable_mask = 0x0U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
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
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 15U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0x86U, .pr_bit = 0x4U
        } }},
    
    .twi_count = 0U,
    .twis = {{  }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3CU, .eedr_address = 0x3DU, .eearl_address = 0x3EU, .eearh_address = 0x3FU,
            .vector_index = 18U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x82U,
            .vector_index = 17U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U
        } }},

    .can_count = 0U,
    .cans = {{  }},

    .port_count = 3U,
    .ports = {{
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU },
        { "PORTE", 0x2CU, 0x2DU, 0x2EU }
    }}
};

} // namespace vioavr::core::devices
