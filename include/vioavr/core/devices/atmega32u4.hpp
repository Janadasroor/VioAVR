#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega32u4 {
    .name = "ATmega32U4",
    .flash_words = 16384U,
    .sram_bytes = 2560U,
    .eeprom_bytes = 1024U,
    .interrupt_vector_count = 43U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 128U,
    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .rampz_address = 0x5BU,
    .eind_address = 0x5CU,
    .spmcsr_address = 0x57U,
    .prr_address = 0x0U,
    .prr0_address = 0x64U,
    .prr1_address = 0x65U,
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    .mcucr_address = 0x55U,
    .pllcsr_address = 0x49U,
    .xmcra_address = 0x0U,
    .xmcrb_address = 0x0U,
    .xmem = {0},
    .pradc_bit = 0x1U,
    .prusart0_bit = 0x2U,
    .prspi_bit = 0x4U,
    .prtwi_bit = 0x80U,
    .prtimer0_bit = 0x20U,
    .prtimer1_bit = 0x8U,
    .prtimer2_bit = 0x40U,
    .smcr_sm_mask = 0xEU,
    .smcr_se_mask = 0x1U,
    .flash_rww_end_word = 0x3800U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x78U, .adch_address = 0x79U, .adcsra_address = 0x7AU, .adcsrb_address = 0x7BU, .admux_address = 0x7CU,
            .vector_index = 29U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x7EU,
            .adc_pin_address = {{ 0x2FU, 0x2FU, 0x0U, 0x0U, 0x2FU, 0x2FU, 0x2FU, 0x2FU, 0x29U, 0x29U, 0x29U, 0x23U, 0x23U, 0x23U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 0U, 1U, 0U, 0U, 4U, 5U, 6U, 7U, 4U, 6U, 7U, 4U, 5U, 6U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare_a, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture, AdcAutoTriggerSource::timer4_overflow, AdcAutoTriggerSource::timer4_compare_a, AdcAutoTriggerSource::timer4_compare_b, AdcAutoTriggerSource::timer4_compare_d, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none, AdcAutoTriggerSource::none }},
            .adsc_mask = 0x40U, .adate_mask = 0x20U, .adif_mask = 0x10U, .adie_mask = 0x8U, .aden_mask = 0x80U, .adlar_mask = 0x20U,
            .adts_mask = 0xFU,
            .pr_address = 100, .pr_bit = 1,
            .mux_table = {{ { 0, 0, 1.0f, false }, { 1, 0, 1.0f, false }, { 2, 0, 1.0f, false }, { 3, 0, 1.0f, false }, { 4, 0, 1.0f, false }, { 5, 0, 1.0f, false }, { 6, 0, 1.0f, false }, { 7, 0, 1.0f, false }, { 0, 1, 10.0f, true }, { 0, 1, 40.0f, true }, { 1, 1, 10.0f, true }, { 1, 1, 40.0f, true }, { 4, 1, 10.0f, true }, { 4, 1, 40.0f, true }, { 5, 1, 10.0f, true }, { 5, 1, 40.0f, true }, { 6, 1, 10.0f, true }, { 6, 1, 40.0f, true }, { 2, 1, 10.0f, true }, { 3, 1, 10.0f, true }, { 2, 1, 40.0f, true }, { 3, 1, 40.0f, true }, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, { 14, 0, 1.0f, false }, { 15, 0, 1.0f, false }, { 8, 0, 1.0f, false }, { 9, 0, 1.0f, false }, { 10, 0, 1.0f, false }, { 11, 0, 1.0f, false }, { 12, 0, 1.0f, false }, { 13, 0, 1.0f, false }, {0xFFU, 0, 1.0f, false}, { 13, 0, 1.0f, false }, { 0, 1, 200.0f, true }, { 0, 1, 200.0f, true }, { 1, 1, 200.0f, true }, {0xFFU, 0, 1.0f, false}, { 4, 5, 10.0f, true }, { 4, 5, 40.0f, true }, { 4, 5, 200.0f, true }, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false}, {0xFFU, 0, 1.0f, false} }}
        } }},
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x50U, .didr1_address = 0x7FU, .vector_index = 28U,
            .ain0_pin_address = 0x2CU, .ain0_pin_bit = 6U, .ain1_pin_address = 0x0U, .ain1_pin_bit = 0U,
            .aci_mask = 0x10U, .acie_mask = 0x8U
        } }},
    .timer8_count = 1U,
    .timers8 = {{ {
            .tcnt_address = 0x46U, .ocra_address = 0x47U, .ocrb_address = 0x48U, .tifr_address = 0x35U, .timsk_address = 0x6EU, .tccra_address = 0x44U, .tccrb_address = 0x45U, .assr_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 21U,
            .compare_b_vector_index = 22U,
            .overflow_vector_index = 23U,
            .ocra_pin_address = 0x25U, .ocra_pin_bit = 7U, .ocrb_pin_address = 0x2BU, .ocrb_pin_bit = 0U,
            .t_pin_address = 0x29U, .t_pin_bit = 7U,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U,
            .wgm0_mask = 0x3U, .wgm2_mask = 0x8U, .cs_mask = 0x7U,
            .as2_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .overflow_enable_mask = 0x1U,
            .foca_mask = 0x80U, .focb_mask = 0x40U,
            .pr_address = 100, .pr_bit = 32,
            .compare_a_trigger_source = AdcAutoTriggerSource::timer0_compare_a,
            .overflow_trigger_source = AdcAutoTriggerSource::timer0_overflow
        } }},
    .timer16_count = 2U,
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
            .foca_mask = 0x80U, .focb_mask = 0x40U, .focc_mask = 0x20U,
            .pr_address = 100, .pr_bit = 8,
            .compare_b_trigger_source = AdcAutoTriggerSource::timer1_compare_b,
            .overflow_trigger_source = AdcAutoTriggerSource::timer1_overflow,
            .capture_trigger_source = AdcAutoTriggerSource::timer1_capture
        },
        {
            .tcnt_address = 0x94U, .ocra_address = 0x98U, .ocrb_address = 0x9AU, .ocrc_address = 0x9CU, .icr_address = 0x96U, .tifr_address = 0x38U, .timsk_address = 0x71U, .tccra_address = 0x90U, .tccrb_address = 0x91U, .tccrc_address = 0x92U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .capture_vector_index = 31U,
            .compare_a_vector_index = 32U,
            .compare_b_vector_index = 33U,
            .compare_c_vector_index = 34U,
            .overflow_vector_index = 35U,
            .ocra_pin_address = 0x28U, .ocra_pin_bit = 6U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .ocrc_pin_address = 0x0U, .ocrc_pin_bit = 0U,
            .icp_pin_address = 0x26U, .icp_pin_bit = 7U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x20U,
            .compare_a_enable_mask = 0x2U,
            .compare_b_enable_mask = 0x4U,
            .compare_c_enable_mask = 0x8U,
            .overflow_enable_mask = 0x1U,
            .foca_mask = 0x80U, .focb_mask = 0x40U, .focc_mask = 0x20U,
            .pr_address = 101, .pr_bit = 8,
            .compare_b_trigger_source = AdcAutoTriggerSource::timer3_compare_b,
            .overflow_trigger_source = AdcAutoTriggerSource::timer3_overflow,
            .capture_trigger_source = AdcAutoTriggerSource::timer3_capture
        } }},
    .timer10_count = 1U,
    .timers10 = {{ {
            .tcnt_address = 0xBEU, .tc4h_address = 0xBFU, .ocra_address = 0xCFU, .ocrb_address = 0xD0U, .ocrc_address = 0xD1U, .ocrd_address = 0xD2U,
            .ocra_pin_address = 0x28U, .ocra_pin_bit = 7U, .ocra_neg_pin_address = 0x28U, .ocra_neg_pin_bit = 6U,
            .ocrb_pin_address = 0x25U, .ocrb_pin_bit = 6U, .ocrb_neg_pin_address = 0x25U, .ocrb_neg_pin_bit = 5U,
            .ocrd_pin_address = 0x2BU, .ocrd_pin_bit = 7U, .ocrd_neg_pin_address = 0x2BU, .ocrd_neg_pin_bit = 6U,
            .tccra_address = 0xC0U, .tccrb_address = 0xC1U, .tccrc_address = 0xC2U, .tccrd_address = 0xC3U, .tccre_address = 0xC4U, .dt4_address = 0xD4U, .pllcsr_address = 0x49U, .timsk_address = 0x72U, .tifr_address = 0x39U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U, .tccrd_reset = 0x0U, .tccre_reset = 0x0U,
            .compare_a_vector_index = 38U,
            .compare_b_vector_index = 39U,
            .compare_d_vector_index = 40U,
            .overflow_vector_index = 41U,
            .pr_address = 101, .pr_bit = 16,
            .compare_a_trigger_source = AdcAutoTriggerSource::timer4_compare_a,
            .compare_b_trigger_source = AdcAutoTriggerSource::timer4_compare_b,
            .compare_d_trigger_source = AdcAutoTriggerSource::timer4_compare_d,
            .overflow_trigger_source = AdcAutoTriggerSource::timer4_overflow
        } }},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x69U, .eicrb_address = 0x6AU, .eimsk_address = 0x3DU, .eifr_address = 0x3CU,
            .vector_indices = {{ 1U, 2U, 3U, 4U, 0U, 0U, 7U, 0U }}
        } }},

    .uart_count = 1U,
    .uarts = {{ {
            .udr_address = 0xCEU, .ucsra_address = 0xC8U, .ucsrb_address = 0xC9U, .ucsrc_address = 0xCAU, .ubrrl_address = 0xCCU, .ubrrh_address = 0xCDU,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 25U,
            .udre_vector_index = 26U,
            .tx_vector_index = 27U,
            .u2x_mask = 0x2U, .rxc_mask = 0x80U, .txc_mask = 0x40U, .udre_mask = 0x20U,
            .rxen_mask = 0x10U, .txen_mask = 0x8U, .rxcie_mask = 0x80U, .txcie_mask = 0x40U, .udrie_mask = 0x20U,
            .pr_address = 100, .pr_bit = 2
        } }},
    
    .pcint_count = 1U,
    .pcints = {{ {
            .pcicr_address = 0x68U,
            .pcifr_address = 0x3BU,
            .pcmsk_address = 0x6BU,
            .pcicr_enable_mask = 0x1U, .pcifr_flag_mask = 0x1U,
            .vector_index = 9U
        } }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x4CU, .spsr_address = 0x4DU, .spdr_address = 0x4EU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 24U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 100, .pr_bit = 4
        } }},
    
    .twi_count = 1U,
    .twis = {{ {
            .twbr_address = 0xB8U, .twsr_address = 0xB9U, .twar_address = 0xBAU, .twdr_address = 0xBBU, .twcr_address = 0xBCU, .twamr_address = 0xBDU,
            .vector_index = 36U,
            .twint_mask = 0x80U, .twen_mask = 0x4U, .twie_mask = 0x1U, .twsto_mask = 0x10U, .twsta_mask = 0x20U, .twea_mask = 0x40U,
            .pr_address = 100, .pr_bit = 128
        } }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
            .eecr_address = 0x3FU, .eedr_address = 0x40U, .eearl_address = 0x41U, .eearh_address = 0x42U,
            .eecr_reset = 0x0U,
            .vector_index = 30U,
            .size = 0x400U
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x60U,
            .wdtcsr_reset = 0x0U,
            .vector_index = 12U,
            .wdie_mask = 0x40U, .wde_mask = 0x8U, .wdce_mask = 0x10U
        } }},

    .can_count = 0U,
    .cans = {{  }},
    
    .usb_count = 1U,
    .usbs = {{ {
            .uhwcon_address = 0xD7U, .usbcon_address = 0xD8U, .usbsta_address = 0xD9U, .usbint_address = 0xDAU,
            .udcon_address = 0xE0U, .udint_address = 0xE1U, .udien_address = 0xE2U, .udaddr_address = 0xE3U, .udfnum_address = 0xE4U, .udmfn_address = 0xE6U,
            .uenum_address = 0xE9U, .uerst_address = 0xEAU, .ueint_address = 0xF4U,
            .ueintx_address = 0xE8U, .ueienx_address = 0xF0U, .uedatx_address = 0xF1U,
            .uebclx_address = 0xF2U, .uebchx_address = 0xF3U, .ueconx_address = 0xEBU,
            .uecfg0x_address = 0xECU, .uecfg1x_address = 0xEDU, .uesta0x_address = 0xEEU, .uesta1x_address = 0xEFU,
            .gen_vector_index = 10U,
            .com_vector_index = 11U,
            .pllcsr_address = 0x49U,
            .usbcon_usbe_mask = 0x80U, .usbcon_frzclk_mask = 0x20U,
            .udint_sofi_mask = 0x4U,
            .pr_address = 101, .pr_bit = 128
        } }},

    .psc_count = 0U,
    .pscs = {{  }},

    .dac_count = 0U,
    .dacs = {{  }},

    .fuse_address = 0x0U,
    .lockbit_address = 0x0U,
    .signature_address = 0x0U,

    .port_count = 5U,
    .ports = {{
        { "PORTB", 0x23U, 0x24U, 0x25U },
        { "PORTC", 0x26U, 0x27U, 0x28U },
        { "PORTD", 0x29U, 0x2AU, 0x2BU },
        { "PORTE", 0x2CU, 0x2DU, 0x2EU },
        { "PORTF", 0x2FU, 0x30U, 0x31U }
    }}
};

} // namespace vioavr::core::devices
