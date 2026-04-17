#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor at90pwm216 {
    .name = "AT90PWM216",
    .flash_words = 8192U,
    .sram_bytes = 1024U,
    .eeprom_bytes = 512U,
    .interrupt_vector_count = 32U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 128U,
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
    .prusart0_bit = 0x1U,
    .prspi_bit = 0x2U,
    .prtwi_bit = 0xFFU,
    .prtimer0_bit = 0x3U,
    .prtimer1_bit = 0x4U,
    .prtimer2_bit = 0xFFU,
    .smcr_sm_mask = 0xEU,
    .smcr_se_mask = 0x1U,
    .flash_rww_end_word = 0x1800U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x78U, .adch_address = 0x79U, .adcsra_address = 0x7AU, .adcsrb_address = 0x7BU, .admux_address = 0x7CU,
            .vector_index = 18U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x7EU,
            .adc_pin_address = {{ 0x23U, 0x23U, 0x23U, 0x23U, 0x23U, 0x23U, 0x23U, 0x23U, 0x29U, 0x29U, 0x29U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 0U, 1U, 2U, 0U, 0U, 0U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare_a, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture, AdcAutoTriggerSource::psc0_sync, AdcAutoTriggerSource::psc1_sync, AdcAutoTriggerSource::psc2_sync, AdcAutoTriggerSource::analog_comparator_1, AdcAutoTriggerSource::analog_comparator_2, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none }},
            .adsc_mask = 0x40U, .adate_mask = 0x20U, .adif_mask = 0x10U, .adie_mask = 0x8U, .aden_mask = 0x80U, .adlar_mask = 0x20U,
            .adts_mask = 0x7U,
            .pr_address = 100, .pr_bit = 0,
            .mux_table = {{ { 0, 0, 1.0f, false }, { 1, 0, 1.0f, false }, { 2, 0, 1.0f, false }, { 3, 0, 1.0f, false }, { 4, 0, 1.0f, false }, { 5, 0, 1.0f, false }, { 6, 0, 1.0f, false }, { 7, 0, 1.0f, false }, { 8, 0, 1.0f, false }, { 9, 0, 1.0f, false }, { 10, 0, 1.0f, false }, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, { 11, 0, 1.0f, false }, { 12, 0, 1.0f, false }, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false} }}
        } }},
    .ac_count = 3U,
    .acs = {{ {
                    .acsr_address = 0x50U, .accon_address = 0xADU, .didr_address = 0x0U,
                    .vector_index = 7U,
                    .aip_pin_address = 0x23U, .aip_pin_bit = 2U, .aim_pin_address = 0x23U, .aim_pin_bit = 3U,
                    .acd_mask = 0x80U, .acbg_mask = 0x0, .aco_mask = 0x1U,
                    .acif_mask = 0x10U, .acie_mask = 0x40U, .acic_mask = 0x0, .acis_mask = 0x30U
                },
        {
                    .acsr_address = 0x50U, .accon_address = 0xAEU, .didr_address = 0x0U,
                    .vector_index = 7U,
                    .aip_pin_address = 0x29U, .aip_pin_bit = 2U, .aim_pin_address = 0x29U, .aim_pin_bit = 3U,
                    .acd_mask = 0x80U, .acbg_mask = 0x0, .aco_mask = 0x2U,
                    .acif_mask = 0x20U, .acie_mask = 0x40U, .acic_mask = 0x0, .acis_mask = 0x30U
                },
        {
                    .acsr_address = 0x50U, .accon_address = 0xAFU, .didr_address = 0x0U,
                    .vector_index = 7U,
                    .aip_pin_address = 0x23U, .aip_pin_bit = 4U, .aim_pin_address = 0x23U, .aim_pin_bit = 5U,
                    .acd_mask = 0x80U, .acbg_mask = 0x0, .aco_mask = 0x4U,
                    .acif_mask = 0x40U, .acie_mask = 0x40U, .acic_mask = 0x0, .acis_mask = 0x30U
                } }},
    .timer8_count = 1U,
    .timers8 = {{ {
            .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x48U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .assr_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 16U,
            .compare_b_vector_index = 27U,
            .overflow_vector_index = 17U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U,
            .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .overflow_enable_mask = 0x1U,
            .foca_mask = 0x80U, .focb_mask = 0x40U,
            .pr_address = 100, .pr_bit = 3,
            .compare_a_trigger_source = AdcAutoTriggerSource::timer0_compare_a,
            .overflow_trigger_source = AdcAutoTriggerSource::timer0_overflow
        } }},
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x84U, .ocra_address = 0x88U, .ocrb_address = 0x8AU, .ocrc_address = 0x0U, .icr_address = 0x86U, .tifr_address = 0x36U, .timsk_address = 0x6FU, .tccra_address = 0x80U, .tccrb_address = 0x81U, .tccrc_address = 0x82U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 3U,
            .compare_a_vector_index = 12U,
            .compare_b_vector_index = 13U,
            .compare_c_vector_index = 0U,
            .overflow_vector_index = 15U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .ocrc_pin_address = 0x0U, .ocrc_pin_bit = 0U,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x1U,
            .foca_mask = 0x80U, .focb_mask = 0x40U, .focc_mask = 0x0U,
            .pr_address = 100, .pr_bit = 4,
            .compare_b_trigger_source = AdcAutoTriggerSource::timer1_compare_b,
            .overflow_trigger_source = AdcAutoTriggerSource::timer1_overflow,
            .capture_trigger_source = AdcAutoTriggerSource::timer1_capture
        } }},
    .timer10_count = 0U,
    .timers10 = {{  }},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x69U, .eicrb_address = 0x0U, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 10U, 19U, 24U, 28U, 0U, 0U, 0U, 0U }}
        } }},

    .uart_count = 1U,
    .uarts = {{ {
            .udr_address = 0xC6U, .ucsra_address = 0xC0U, .ucsrb_address = 0xC1U, .ucsrc_address = 0xC2U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 21U,
            .udre_vector_index = 22U,
            .tx_vector_index = 23U,
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U, .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 100, .pr_bit = 1
        } }},
    
    .pcint_count = 0U,
    .pcints = {{  }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 20U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 100, .pr_bit = 2
        } }},
    
    .twi_count = 0U,
    .twis = {{  }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x42U,
            .eecr_reset = 0x0U,
            .vector_index = 26U,
            .size = 0x200U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .wdtcsr_reset = 0x0U,
            .vector_index = 25U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U, .wdce_mask = 0x10U
        } }},

    .can_count = 0U,
    .cans = {{  }},
    
    .usb_count = 0U,
    .usbs = {{  }},

    .psc_count = 2U,
    .pscs = {{ {
            .pctl_address = 0xDBU, .psoc_address = 0xD0U, .pconf_address = 0xDAU, .pim_address = 0xA1U, .pifr_address = 0xA0U, .picr_address = 0xDEU,
            .ocrsa_address = 0xD2U, .ocrra_address = 0xD4U, .ocrsb_address = 0xD6U, .ocrrb_address = 0xD8U,
            .pfrc0a_address = 0xDCU, .pfrc0b_address = 0xDDU,
            .psc_index = 0U,
            .gen_vector_index = 6U,
            .ec_vector_index = 6U,
            .capt_vector_index = 5U,
            .prun_mask = 0x1U, .mode_mask = 0x18U, .clksel_mask = 0x2U,
            .ec_flag_mask = 0x1U, .capt_flag_mask = 0x8U,
            .pr_address = 100, .pr_bit = 5
        },
        {
            .pctl_address = 0xFBU, .psoc_address = 0xF0U, .pconf_address = 0xFAU, .pim_address = 0xA5U, .pifr_address = 0xA4U, .picr_address = 0xFEU,
            .ocrsa_address = 0xF2U, .ocrra_address = 0xF4U, .ocrsb_address = 0xF6U, .ocrrb_address = 0xF8U,
            .pfrc0a_address = 0xFCU, .pfrc0b_address = 0xFDU,
            .psc_index = 2U,
            .gen_vector_index = 2U,
            .ec_vector_index = 2U,
            .capt_vector_index = 1U,
            .prun_mask = 0x1U, .mode_mask = 0x18U, .clksel_mask = 0x2U,
            .ec_flag_mask = 0x1U, .capt_flag_mask = 0x8U,
            .pr_address = 100, .pr_bit = 7
        } }},

    .dac_count = 1U,
    .dacs = {{ {
            .dacon_address = 0xAAU, .dacl_address = 0x0U, .dach_address = 0x0U,
            .pr_address = 0, .pr_bit = 255
        } }},

    .fuse_address = 0x0U,
    .lockbit_address = 0x0U,
    .signature_address = 0x0U,

    .port_count = 3U,
    .ports = {{
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU },
        { "PORTE", 0x2CU, 0x2DU, 0x2EU }
    }}
};

} // namespace vioavr::core::devices
