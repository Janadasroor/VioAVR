#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor at90pwm81 {
    .name = "AT90PWM81",
    .flash_words = 4096U,
    .sram_bytes = 256U,
    .eeprom_bytes = 512U,
    .interrupt_vector_count = 20U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 64U,
    .io_range = { 0x20U, 0x5FU },
    .extended_io_range = { 0x60U, 0xFFU },

    .mapped_flash = { 0x0U, 0x0U },
    .mapped_eeprom = { 0x0U, 0x0U },
    .mapped_fuses = { 0x0U, 0x0U },
    .mapped_signatures = { 0x0U, 0x0U },
    .mapped_user_signatures = { 0x0U, 0x0U },

    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .rampz_address = 0x0U,
    .eind_address = 0x0U,
    .spmcsr_address = 0x57U,
    .prr_address = 0x86U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    .mcucr_address = 0x55U,
    .pllcsr_address = 0x87U,
    .xmcra_address = 0x0U,
    .xmcrb_address = 0x0U,
    .xmem = {0},
    .pradc_bit = 0x0U,

    .prusart0_bit = 0xFFU,
    .prspi_bit = 0x2U,
    .prtwi_bit = 0xFFU,
    .prtimer0_bit = 0xFFU,
    .prtimer1_bit = 0x4U,
    .prtimer2_bit = 0xFFU,
    .smcr_sm_mask = 0xEU,
    .smcr_se_mask = 0x1U,
    .flash_rww_end_word = 0xC00U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x4CU, .adch_address = 0x4DU, .adcsra_address = 0x26U, .adcsrb_address = 0x27U, .admux_address = 0x28U,
            .vector_index = 13U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x77U,
            .adc_pin_address = {{ 0x23U, 0x23U, 0x23U, 0x23U, 0x23U, 0x23U, 0x23U, 0x23U, 0x29U, 0x29U, 0x29U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 0U, 1U, 2U, 0U, 0U, 0U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare_a, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture, AdcAutoTriggerSource::psc0_sync, AdcAutoTriggerSource::psc1_sync, AdcAutoTriggerSource::psc2_sync, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none }},
            .adsc_mask = 0x40U, .adate_mask = 0x20U, .adif_mask = 0x10U, .adie_mask = 0x8U, .aden_mask = 0x80U, .adlar_mask = 0x20U,
            .adts_mask = 0xFU,
            .pr_address = 134, .pr_bit = 0,
            .mux_table = {{ { 0, 0, 1.0f, false }, { 1, 0, 1.0f, false }, { 2, 0, 1.0f, false }, { 3, 0, 1.0f, false }, { 4, 0, 1.0f, false }, { 5, 0, 1.0f, false }, { 6, 0, 1.0f, false }, { 7, 0, 1.0f, false }, { 8, 0, 1.0f, false }, { 9, 0, 1.0f, false }, { 10, 0, 1.0f, false }, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, { 11, 0, 1.0f, false }, { 12, 0, 1.0f, false }, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false} }}
        } }},
    .adc8x_count = 0U,
    .adcs8x = {{  }},
    .ac_count = 3U,
    .acs = {{ {
                    .acsr_address = 0x20U, .accon_address = 0x7DU, .didr_address = 0x0U,
                    .vector_index = 7U,
                    .aip_pin_address = 0x29U, .aip_pin_bit = 2U, .aim_pin_address = 0x29U, .aim_pin_bit = 3U,
                    .acd_mask = 0x80U, .acbg_mask = 0x0, .aco_mask = 0x2U,
                    .acif_mask = 0x20U, .acie_mask = 0x40U, .acic_mask = 0x0, .acis_mask = 0x30U
                },
        {
                    .acsr_address = 0x20U, .accon_address = 0x7EU, .didr_address = 0x0U,
                    .vector_index = 7U,
                    .aip_pin_address = 0x23U, .aip_pin_bit = 4U, .aim_pin_address = 0x23U, .aim_pin_bit = 5U,
                    .acd_mask = 0x80U, .acbg_mask = 0x0, .aco_mask = 0x4U,
                    .acif_mask = 0x40U, .acie_mask = 0x40U, .acic_mask = 0x0, .acis_mask = 0x30U
                },
        {
                    .acsr_address = 0x20U, .accon_address = 0x7FU, .didr_address = 0x0U,
                    .vector_index = 7U,
                    .aip_pin_address = 0x0U, .aip_pin_bit = 0U, .aim_pin_address = 0x0U, .aim_pin_bit = 0U,
                    .acd_mask = 0x80U, .acbg_mask = 0x0, .aco_mask = 0x8U,
                    .acif_mask = 0x80U, .acie_mask = 0x40U, .acic_mask = 0x0, .acis_mask = 0x30U
                } }},
    .ac8x_count = 0U,
    .acs8x = {{  }},
    .timer8_count = 0U,
    .timers8 = {{  }},
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x5AU, .ocra_address = 0x0U, .ocrb_address = 0x0U, .ocrc_address = 0x0U, .icr_address = 0x8CU, .tifr_address = 0x22U, .timsk_address = 0x21U, .tccra_address = 0x0U, .tccrb_address = 0x8AU, .tccrc_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .timer_index = 1U,
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
            .foca_mask = 0x0U, .focb_mask = 0x0U, .focc_mask = 0x0U,
            .pr_address = 134, .pr_bit = 4,
            .compare_a_trigger_source = AdcAutoTriggerSource::timer1_compare_a,
            .compare_b_trigger_source = AdcAutoTriggerSource::timer1_compare_b,
            .compare_c_trigger_source = AdcAutoTriggerSource::timer1_compare_c,
            .overflow_trigger_source = AdcAutoTriggerSource::timer1_overflow,
            .capture_trigger_source = AdcAutoTriggerSource::timer1_capture
        } }},
    .timer10_count = 0U,
    .timers10 = {{  }},
    .tca_count = 0U,
    .timers_tca = {{  }},

    .tcb_count = 0U,
    .timers_tcb = {{  }},

    .rtc_count = 0U,
    .timers_rtc = {{  }},

    .evsys = {},

    .ccl = {},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x89U, .eicrb_address = 0x0U, .eimsk_address = 0x41U, .eifr_address = 0x40U,
            .vector_indices = {{ 10U, 14U, 16U, 0U, 0U, 0U, 0U, 0U }}
        } }},

    .uart_count = 0U,
    .uarts = {{  }},
    
    .uart8x_count = 0U,
    .uarts8x = {{  }},

    .nvm_ctrl_count = 0U,
    .nvm_ctrls = {{  }},
    .cpu_int_count = 0U,
    .cpu_ints = {{  }},
    
    .pcint_count = 0U,
    .pcints = {{  }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x37U, .spsr_address = 0x38U, .spdr_address = 0x56U,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 15U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 134, .pr_bit = 2
        } }},

    .spi8x_count = 0U,
    .spis8x = {{  }},
    
    .twi_count = 0U,
    .twis = {{  }},

    .twi8x_count = 0U,
    .twis8x = {{  }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3CU, .eedr_address = 0x3DU, .eearl_address = 0x3EU, .eearh_address = 0x3FU,
            .eecr_reset = 0x0U,
            .vector_index = 18U,
            .size = 0x200U,
            .mapped_data = { 0x0U, 0x0U }
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x82U,
            .wdtcsr_reset = 0x0U,
            .vector_index = 17U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U, .wdce_mask = 0x10U
        } }},

    .wdt8x_count = 0U,
    .wdts8x = {{  }},

    .crc8x_count = 0U,
    .crcs8x = {{  }},

    .can_count = 0U,
    .cans = {{  }},
    
    .usb_count = 0U,
    .usbs = {{  }},

    .psc_count = 2U,
    .pscs = {{ {
            .pctl_address = 0x32U, .psoc_address = 0x6AU, .pconf_address = 0x31U, .pim_address = 0x2FU, .pifr_address = 0x30U, .picr_address = 0x68U,
            .ocrsa_address = 0x60U, .ocrra_address = 0x4AU, .ocrsb_address = 0x42U, .ocrrb_address = 0x44U,
            .pfrc0a_address = 0x62U, .pfrc0b_address = 0x63U,
            .psc_index = 0U,
            .gen_vector_index = 5U,
            .ec_vector_index = 5U,
            .capt_vector_index = 4U,
            .prun_mask = 0x1U, .mode_mask = 0x18U, .clksel_mask = 0x2U,
            .ec_flag_mask = 0x1U, .capt_flag_mask = 0x8U,
            .pr_address = 0, .pr_bit = 255
        },
        {
            .pctl_address = 0x36U, .psoc_address = 0x6EU, .pconf_address = 0x35U, .pim_address = 0x33U, .pifr_address = 0x34U, .picr_address = 0x6CU,
            .ocrsa_address = 0x64U, .ocrra_address = 0x4EU, .ocrsb_address = 0x46U, .ocrrb_address = 0x48U,
            .pfrc0a_address = 0x66U, .pfrc0b_address = 0x67U,
            .psc_index = 2U,
            .gen_vector_index = 2U,
            .ec_vector_index = 2U,
            .capt_vector_index = 1U,
            .prun_mask = 0x1U, .mode_mask = 0x18U, .clksel_mask = 0x2U,
            .ec_flag_mask = 0x1U, .capt_flag_mask = 0x8U,
            .pr_address = 134, .pr_bit = 7
        } }},

    .dac_count = 1U,
    .dacs = {{ {
            .dacon_address = 0x76U, .dacl_address = 0x58U, .dach_address = 0x59U,
            .daen_mask = 0x1U, .daate_mask = 0x80U, .dats_mask = 0x70U, .dacoe_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255
        } }},

    .fuse_address = 0x0U,
    .lockbit_address = 0x0U,
    .signature_address = 0x0U,

    .signature = { 0x1EU, 0x93U, 0x88U },
    .fuses = { 0x62U, 0xD9U, 0xFDU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU },

    .port_count = 3U,
    .ports = {{
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU },
        { "PORTE", 0x2CU, 0x2DU, 0x2EU }
    }}
};

} // namespace vioavr::core::devices
