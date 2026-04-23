#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega8515 {
    .name = "ATmega8515",
    .flash_words = 4096U,
    .sram_bytes = 512U,
    .sram_start = 0x60U,
    .eeprom_bytes = 512U,
    .interrupt_vector_count = 17U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 64U,
    .io_range = { 0x20U, 0x5FU },
    .extended_io_range = { 0x60U, 0x5FU },

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
    .spmcsr_address = 0x0U,
    .sigrd_mask = 0x0U,
    .blbset_mask = 0x0U,
    .spmen_mask = 0x0U,
    .prr_address = 0x0U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    .smcr_address = 0x0U,
    .mcusr_address = 0x0U,
    .mcucr_address = 0x55U,
    .ccp_address = 0x0U,
    .pllcsr_address = 0x0U,
    .xmcra_address = 0x0U,
    .xmcrb_address = 0x0U,
    .xmem = {0},
    .pradc_bit = 0xFFU,

    .prusart0_bit = 0xFFU,
    .prspi_bit = 0xFFU,
    .prtwi_bit = 0xFFU,
    .prtimer0_bit = 0xFFU,
    .prtimer1_bit = 0xFFU,
    .prtimer2_bit = 0xFFU,
    .smcr_sm_mask = 0x0U,
    .smcr_se_mask = 0x0U,
    .flash_rww_end_word = 0xC00U,
    .boot_start_address = 0xC00U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .adc_count = 0U,
    .adcs = {{  }},
    .adc8x_count = 0U,
    .adcs8x = {{  }},
    .ac_count = 1U,
    .acs = {{ {
                .acsr_address = 0x28U, .accon_address = 0, .didr_address = 0x0U,
                .vector_index = 0U,
                .aip_pin_address = 0x0U, .aip_pin_bit = 0U, .aim_pin_address = 0x0U, .aim_pin_bit = 0U,
                .acd_mask = 0x80U, .acbg_mask = 0x40U, .aco_mask = 0x20U,
                .acif_mask = 0x10U, .acie_mask = 0x8U, .acic_mask = 0x4U, .acis_mask = 0x3U
            } }},
    .ac8x_count = 0U,
    .acs8x = {{  }},
    .timer8_count = 1U,
    .timers8 = {{ {
            .tcnt_address = 0x52U, .ocra_address = 0x0U, .ocrb_address = 0x0U, .tifr_address = 0x58U, .timsk_address = 0x59U, .tccra_address = 0x0U, .tccrb_address = 0x0U, .assr_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .assr_reset = 0x0U,
            .compare_a_vector_index = 0U,
            .compare_b_vector_index = 0U,
            .overflow_vector_index = 7U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .tosc1_pin_address = 0x0U, .tosc1_pin_bit = 0U, .tosc2_pin_address = 0x0U, .tosc2_pin_bit = 0U,
            .wgm0_mask = 0x0U, .wgm2_mask = 0x0U, .cs_mask = 0x0U,
            .as2_mask = 0x0U, .exclk_mask = 0x0U, .tcn2ub_mask = 0x0U,
            .ocr2aub_mask = 0x0U, .ocr2bub_mask = 0x0U,
            .tcr2aub_mask = 0x0U, .tcr2bub_mask = 0x0U,
            .compare_a_enable_mask = 0x0U,
            .compare_b_enable_mask = 0x0U,
            .overflow_enable_mask = 0x2U,
            .foca_mask = 0x0U, .focb_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255,
            .timer_index = 0U,
            .compare_a_trigger_source = AdcAutoTriggerSource::timer0_compare_a,
            .compare_b_trigger_source = AdcAutoTriggerSource::timer0_compare_b,
            .overflow_trigger_source = AdcAutoTriggerSource::timer0_overflow
        } }},
    .timer16_count = 1U,
    .timers16 = {{ {
            .tcnt_address = 0x4CU, .ocra_address = 0x4AU, .ocrb_address = 0x48U, .ocrc_address = 0x0U, .icr_address = 0x44U, .tifr_address = 0x58U, .timsk_address = 0x59U, .tccra_address = 0x4FU, .tccrb_address = 0x4EU, .tccrc_address = 0x0U,
            .tccra_reset = 0x0U, .tccrb_reset = 0x0U, .tccrc_reset = 0x0U,
            .timer_index = 1U,
            .capture_vector_index = 3U,
            .compare_a_vector_index = 4U,
            .compare_b_vector_index = 5U,
            .compare_c_vector_index = 0U,
            .overflow_vector_index = 6U,
            .ocra_pin_address = 0x0U, .ocra_pin_bit = 0U, .ocrb_pin_address = 0x0U, .ocrb_pin_bit = 0U, .ocrc_pin_address = 0x0U, .ocrc_pin_bit = 0U,
            .icp_pin_address = 0x0U, .icp_pin_bit = 0U,
            .t_pin_address = 0x0U, .t_pin_bit = 0U,
            .wgm10_mask = 0x3U, .wgm12_mask = 0x18U, .cs_mask = 0x7U, .ices_mask = 0x40U, .icnc_mask = 0x80U,
            .capture_enable_mask = 0x8U,
            .compare_a_enable_mask = 0x40U,
            .compare_b_enable_mask = 0x20U,
            .compare_c_enable_mask = 0x0U,
            .overflow_enable_mask = 0x80U,
            .foca_mask = 0x0U, .focb_mask = 0x0U, .focc_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255,
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
    .portmux = {},
    
    .vref = {},
    .clkctrl = {},
    .slpctrl = {},
    .rstctrl = {},
    .syscfg = {},
    .bod = {},
    
    .ext_interrupt_count = 1U,
    .ext_interrupts = {{ {
            .eicra_address = 0x0U, .eicrb_address = 0x0U, .eimsk_address = 0x0U, .eifr_address = 0x0U,
            .vector_indices = {{ 1U, 2U, 13U, 0U, 0U, 0U, 0U, 0U }}
        } }},

    .uart_count = 1U,
    .uarts = {{ {
            .udr_address = 0x2CU, .ucsra_address = 0x2BU, .ucsrb_address = 0x2AU, .ucsrc_address = 0x40U, .ubrrl_address = 0x0U, .ubrrh_address = 0x1U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 9U,
            .udre_vector_index = 10U,
            .tx_vector_index = 11U,
            .u2x_mask = 0x2U, 
            .rxc_mask = 0x80U, 
            .txc_mask = 0x40U, 
            .udre_mask = 0x20U,
            .rxen_mask = 0x10U, 
            .txen_mask = 0x8U, 
            .rxcie_mask = 0x80U, 
            .txcie_mask = 0x40U, 
            .udrie_mask = 0x20U,
            .pr_address = 0, .pr_bit = 255,
            .uart_index = 0U,
            .txd_pin_address = 0x0U, .txd_pin_bit = 0U,
            .rxd_pin_address = 0x0U, .rxd_pin_bit = 0U
        } }},
    
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
            .spcr_address = 0x2DU, .spsr_address = 0x2EU, .spdr_address = 0x2FU,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 8U,
            .spe_mask = 0x40U, .spie_mask = 0x80U, .mstr_mask = 0x10U, .spif_mask = 0x80U, .wcol_mask = 0x40U, .sp2x_mask = 0x1U,
            .pr_address = 0, .pr_bit = 255
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
            .vector_index = 0U,
            .size = 0x200U,
            .mapped_data = { 0x0U, 0x0U }
        } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x41U,
            .wdtcsr_reset = 0x0U,
            .vector_index = 0U,
            .wdie_mask = 0x0U, .wde_mask = 0x8U, .wdce_mask = 0x10U
        } }},

    .wdt8x_count = 0U,
    .wdts8x = {{  }},

    .crc8x_count = 0U,
    .crcs8x = {{  }},

    .can_count = 0U,
    .cans = {{  }},
    
    .usb_count = 0U,
    .usbs = {{  }},

    .psc_count = 0U,
    .pscs = {{  }},

    .dac_count = 0U,
    .dacs = {{  }},
    
    .dma_count = 0U,
    .dmas = {{  }},

    .fuse_address = 0x0U,
    .lockbit_address = 0x0U,
    .signature_address = 0x0U,

    .signature = { 0x1EU, 0x93U, 0x6U },
    .fuses = { 0xE1U, 0xD9U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU },
    .lockbit_reset = 0xFFU,

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 5U,
    .ports = {{
        { "PORTA", 0x39U, 0x3AU, 0x3BU, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0xFFFFU, 255U },
        { "PORTB", 0x36U, 0x37U, 0x38U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0xFFFFU, 255U },
        { "PORTC", 0x33U, 0x34U, 0x35U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0xFFFFU, 255U },
        { "PORTD", 0x30U, 0x31U, 0x32U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0xFFFFU, 255U },
        { "PORTE", 0x25U, 0x26U, 0x27U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
