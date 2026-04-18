#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega32m1 {
    .name = "ATmega32M1",
    .flash_words = 16384U,
    .sram_bytes = 2048U,
    .eeprom_bytes = 1024U,
    .interrupt_vector_count = 31U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 128U,
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
    .prr_address = 0x64U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    .mcucr_address = 0x55U,
    .pllcsr_address = 0x49U,
    .xmcra_address = 0x0U,
    .xmcrb_address = 0x0U,
    .xmem = {0},
    .pradc_bit = 0x0U,

    .prusart0_bit = 0xFFU,
    .prspi_bit = 0x2U,
    .prtwi_bit = 0xFFU,
    .prtimer0_bit = 0x3U,
    .prtimer1_bit = 0x4U,
    .prtimer2_bit = 0xFFU,
    .smcr_sm_mask = 0xEU,
    .smcr_se_mask = 0x1U,
    .flash_rww_end_word = 0x3800U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x78U, .adch_address = 0x79U, .adcsra_address = 0x7AU, .adcsrb_address = 0x7BU, .admux_address = 0x7CU,
            .vector_index = 27U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x7EU,
            .adc_pin_address = {{ 0x2CU, 0x29U, 0x29U, 0x29U, 0x23U, 0x23U, 0x23U, 0x23U, 0x26U, 0x26U, 0x26U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 2U, 4U, 5U, 6U, 7U, 2U, 5U, 6U, 4U, 5U, 6U, 0U, 0U, 0U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare_a, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture, AdcAutoTriggerSource::psc0_sync, AdcAutoTriggerSource::psc1_sync, AdcAutoTriggerSource::psc2_sync, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none }},
            .adsc_mask = 0x40U, .adate_mask = 0x20U, .adif_mask = 0x10U, .adie_mask = 0x8U, .aden_mask = 0x80U, .adlar_mask = 0x20U,
            .adts_mask = 0xFU,
            .pr_address = 100, .pr_bit = 0,
            .mux_table = {{ { 0, 0, 1.0f, false }, { 1, 0, 1.0f, false }, { 2, 0, 1.0f, false }, { 3, 0, 1.0f, false }, { 4, 0, 1.0f, false }, { 5, 0, 1.0f, false }, { 6, 0, 1.0f, false }, { 7, 0, 1.0f, false }, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false} }}
        } }},
    .adc8x_count = 0U,
    .adcs8x = {{  }},
    .ac_count = 4U,
    .acs = {{ {
                    .acsr_address = 0x50U, .accon_address = 0x94U, .didr_address = 0x0U,
                    .vector_index = 0U,
                    .aip_pin_address = 0x0U, .aip_pin_bit = 0U, .aim_pin_address = 0x0U, .aim_pin_bit = 0U,
                    .acd_mask = 0x80U, .acbg_mask = 0x0, .aco_mask = 0x1U,
                    .acif_mask = 0x10U, .acie_mask = 0x40U, .acic_mask = 0x0, .acis_mask = 0x30U
                },
        {
                    .acsr_address = 0x50U, .accon_address = 0x95U, .didr_address = 0x0U,
                    .vector_index = 0U,
                    .aip_pin_address = 0x0U, .aip_pin_bit = 0U, .aim_pin_address = 0x0U, .aim_pin_bit = 0U,
                    .acd_mask = 0x80U, .acbg_mask = 0x0, .aco_mask = 0x2U,
                    .acif_mask = 0x20U, .acie_mask = 0x40U, .acic_mask = 0x0, .acis_mask = 0x30U
                },
        {
                    .acsr_address = 0x50U, .accon_address = 0x96U, .didr_address = 0x0U,
                    .vector_index = 0U,
                    .aip_pin_address = 0x0U, .aip_pin_bit = 0U, .aim_pin_address = 0x0U, .aim_pin_bit = 0U,
                    .acd_mask = 0x80U, .acbg_mask = 0x0, .aco_mask = 0x4U,
                    .acif_mask = 0x40U, .acie_mask = 0x40U, .acic_mask = 0x0, .acis_mask = 0x30U
                },
        {
                    .acsr_address = 0x50U, .accon_address = 0x97U, .didr_address = 0x0U,
                    .vector_index = 0U,
                    .aip_pin_address = 0x0U, .aip_pin_bit = 0U, .aim_pin_address = 0x0U, .aim_pin_bit = 0U,
                    .acd_mask = 0x80U, .acbg_mask = 0x0, .aco_mask = 0x8U,
                    .acif_mask = 0x80U, .acie_mask = 0x40U, .acic_mask = 0x0, .acis_mask = 0x30U
                } }},
    .ac8x_count = 0U,
    .acs8x = {{  }},
    .timer8_count = 1U,
    .timers8 = {{ {
            .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x48U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .assr_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 15U,
            .compare_b_vector_index = 16U,
            .overflow_vector_index = 17U,
            .ocra_pin_address = 0x2BU, .ocra_pin_bit = 3U, .ocrb_pin_address = 0x2EU, .ocrb_pin_bit = 1U,
            .t_pin_address = 0x26U, .t_pin_bit = 2U,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U,
            .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x0U, .exclk_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .ocr2aub_mask = 0x0U, .ocr2bub_mask = 0x0U,
            .tcr2aub_mask = 0x0U, .tcr2bub_mask = 0x0U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .overflow_enable_mask = 0x1U,
            .foca_mask = 0x80U, .focb_mask = 0x40U,
            .pr_address = 100, .pr_bit = 3,
            .timer_index = 0U,
            .compare_a_trigger_source = AdcAutoTriggerSource::timer0_compare_a,
            .compare_b_trigger_source = AdcAutoTriggerSource::timer0_compare_b,
            .overflow_trigger_source = AdcAutoTriggerSource::timer0_overflow
        } }},
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x8AU, .ocrc_address = 0x0U, .icr_address = 0x86U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x82U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .timer_index = 1U,
            .capture_vector_index = 11U,
            .compare_a_vector_index = 12U,
            .compare_b_vector_index = 13U,
            .compare_c_vector_index = 0U,
            .overflow_vector_index = 14U,
            .ocra_pin_address = 0x2BU, .ocra_pin_bit = 2U, .ocrb_pin_address = 0x28U, .ocrb_pin_bit = 1U, .ocrc_pin_address = 0x0U, .ocrc_pin_bit = 0U,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0U,
            .t_pin_address = 0x26U, .t_pin_bit = 3U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
            .foca_mask = 0x80U, .focb_mask = 0x40U, .focc_mask = 0x0U,
            .pr_address = 100, .pr_bit = 4,
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
            .eicra_address = 0x69U, .eicrb_address = 0x0U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 7U, 8U, 9U, 10U, 0U, 0U, 0U, 0U }}
        } }},

    .uart_count = 0U,
    .uarts = {{  }},
    
    .uart8x_count = 0U,
    .uarts8x = {{  }},

    .nvm_ctrl_count = 0U,
    .nvm_ctrls = {{  }},
    .cpu_int_count = 0U,
    .cpu_ints = {{  }},
    
    .pcint_count = 4U,
    .pcints = {{ {
            .pcicr_address = 0x68U,
            .pcifr_address = 0x3BU,
            .pcmsk_address = 0x6AU,
            .pcicr_enable_mask = 0x1U, .pcifr_flag_mask = 0x1U,
            .vector_index = 22U
        },
        {
            .pcicr_address = 0x68U,
            .pcifr_address = 0x3BU,
            .pcmsk_address = 0x6BU,
            .pcicr_enable_mask = 0x2U, .pcifr_flag_mask = 0x2U,
            .vector_index = 23U
        },
        {
            .pcicr_address = 0x68U,
            .pcifr_address = 0x3BU,
            .pcmsk_address = 0x6CU,
            .pcicr_enable_mask = 0x4U, .pcifr_flag_mask = 0x4U,
            .vector_index = 24U
        },
        {
            .pcicr_address = 0x68U,
            .pcifr_address = 0x3BU,
            .pcmsk_address = 0x6DU,
            .pcicr_enable_mask = 0x8U, .pcifr_flag_mask = 0x8U,
            .vector_index = 25U
        } }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 26U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 100, .pr_bit = 2
        } }},

    .spi8x_count = 0U,
    .spis8x = {{  }},
    
    .twi_count = 0U,
    .twis = {{  }},

    .twi8x_count = 0U,
    .twis8x = {{  }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x42U,
            .eecr_reset = 0x0U,
            .vector_index = 29U,
            .size = 0x400U,
            .mapped_data = { 0x0U, 0x0U }
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .wdtcsr_reset = 0x0U,
            .vector_index = 28U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U, .wdce_mask = 0x10U
        } }},

    .wdt8x_count = 0U,
    .wdts8x = {{  }},

    .crc8x_count = 0U,
    .crcs8x = {{  }},

    .can_count = 1U,
    .cans = {{ {
            .cangcon_address = 0xD8U, .cangsta_address = 0xD9U, .cangit_address = 0xDAU, .cangie_address = 0xDBU, .canen1_address = 0xDDU, .canen2_address = 0xDCU, .canie1_address = 0xDFU, .canie2_address = 0xDEU, .cansit1_address = 0xE1U, .cansit2_address = 0xE0U, .canbt1_address = 0xE2U, .canbt2_address = 0xE3U, .canbt3_address = 0xE4U, .cantcon_address = 0xE5U,
            .cantim_address = 0xE6U, .canttc_address = 0xE8U, .cantec_address = 0xEAU, .canrec_address = 0xEBU, .canhpmob_address = 0xECU, .canpage_address = 0xEDU, .canstmob_address = 0xEEU, .cancdmob_address = 0xEFU, .canidt_address = 0xF0U, .canidm_address = 0xF4U, .canstm_address = 0xF8U, .canmsg_address = 0xFAU,
            .canit_vector_index = 0U,
            .ovrit_vector_index = 0U,
            .mob_count = 15U,
            .pr_address = 100, .pr_bit = 6
        } }},
    
    .usb_count = 0U,
    .usbs = {{  }},

    .psc_count = 1U,
    .pscs = {{ {
            .pctl_address = 0xB7U, .psoc_address = 0x0U, .pconf_address = 0xB5U, .pim_address = 0xBBU, .pifr_address = 0xBCU, .picr_address = 0x0U,
            .ocrsa_address = 0x0U, .ocrra_address = 0x0U, .ocrsb_address = 0x0U, .ocrrb_address = 0x0U,
            .pfrc0a_address = 0x0U, .pfrc0b_address = 0x0U,
            .psc_index = 0U,
            .gen_vector_index = 10U,
            .ec_vector_index = 10U,
            .capt_vector_index = 11U,
            .prun_mask = 0x1U, .mode_mask = 0x10U, .clksel_mask = 0x0U,
            .ec_flag_mask = 0x1U, .capt_flag_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255
        } }},

    .dac_count = 1U,
    .dacs = {{ {
            .dacon_address = 0x90U, .dacl_address = 0x91U, .dach_address = 0x92U,
            .daen_mask = 0x1U, .daate_mask = 0x80U, .dats_mask = 0x70U, .dacoe_mask = 0x2U,
            .pr_address = 0, .pr_bit = 255
        } }},

    .fuse_address = 0x0U,
    .lockbit_address = 0x0U,
    .signature_address = 0x0U,

    .signature = { 0x1EU, 0x95U, 0x84U },
    .fuses = { 0x62U, 0xD9U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU },

    .port_count = 4U,
    .ports = {{
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTC", 0x26U, 0x27U, 0x28U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU },
        { "PORTE", 0x2CU, 0x2DU, 0x2EU }
    }}
};

} // namespace vioavr::core::devices
