#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega3209 {
    .name = "ATmega3209",
    .flash_words = 16384U,
    .sram_bytes = 4096U,
    .eeprom_bytes = 256U,
    .interrupt_vector_count = 43U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 128U,
    .io_range = { 0x0U, 0x3FU },
    .extended_io_range = { 0x40U, 0x10FFU },

    .mapped_flash = { 0x4000U, 0x8000U },
    .mapped_eeprom = { 0x1400U, 0x100U },
    .mapped_fuses = { 0x1280U, 0xAU },
    .mapped_signatures = { 0x1100U, 0x3U },
    .mapped_user_signatures = { 0x1300U, 0x40U },

    .spl_address = 0x3DU,
    .sph_address = 0x3EU,
    .sreg_address = 0x3FU,
    .rampz_address = 0x0U,
    .eind_address = 0x0U,
    .spmcsr_address = 0x0U,
    .prr_address = 0x0U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    .smcr_address = 0x0U,
    .mcusr_address = 0x0U,
    .mcucr_address = 0x0U,
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
    .flash_rww_end_word = 0x4000U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x0U, .adcsrb_address = 0x0U, .admux_address = 0x0U,
            .vector_index = 0U,
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
    .ac_count = 1U,
    .acs = {{ {
                .acsr_address = 0x0U, .accon_address = 0, .didr_address = 0x0U,
                .vector_index = 0U,
                .aip_pin_address = 0x0U, .aip_pin_bit = 0U, .aim_pin_address = 0x0U, .aim_pin_bit = 0U,
                .acd_mask = 0x0U, .acbg_mask = 0x0U, .aco_mask = 0x0U,
                .acif_mask = 0x0U, .acie_mask = 0x0U, .acic_mask = 0x0U, .acis_mask = 0x0U
            } }},
    .timer8_count = 0U,
    .timers8 = {{  }},
    .timer16_count = 0U,
    .timers16 = {{  }},
    .timer10_count = 0U,
    .timers10 = {{  }},
    
    .ext_interrupt_count = 0U,
    .ext_interrupts = {{  }},

    .uart_count = 4U,
    .uarts = {{ {
            .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 0U,
            .udre_vector_index = 0U,
            .tx_vector_index = 0U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255
        },
        {
            .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 0U,
            .udre_vector_index = 0U,
            .tx_vector_index = 0U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255
        },
        {
            .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 0U,
            .udre_vector_index = 0U,
            .tx_vector_index = 0U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255
        },
        {
            .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 0U,
            .udre_vector_index = 0U,
            .tx_vector_index = 0U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255
        } }},
    
    .pcint_count = 0U,
    .pcints = {{  }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x0U, .spsr_address = 0x0U, .spdr_address = 0x0U,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 0U,
            .spe_mask = 0x0U, .spie_mask = 0x0U, .mstr_mask = 0x0U, .spif_mask = 0x0U, .wcol_mask = 0x0U, .sp2x_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255
        } }},
    
    .twi_count = 1U,
    .twis = {{ {
            .twbr_address = 0x0U, .twsr_address = 0x0U, .twar_address = 0x0U, .twdr_address = 0x0U, .twcr_address = 0x0U, .twamr_address = 0x0U,
            .vector_index = 14U,
            .twint_mask = 0x0U, .twen_mask = 0x0U, .twie_mask = 0x0U, .twsto_mask = 0x0U, .twsta_mask = 0x0U, .twea_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255
        } }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
                .size = 0x100U,
                .mapped_data = { 0x1400U, 0x100U }
            } }},
    
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x0U,
            .wdtcsr_reset = 0x0U,
            .vector_index = 0U,
            .wdie_mask = 0x0U, .wde_mask = 0x0U, .wdce_mask = 0x0U
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

    .port_count = 9U,
    .ports = {{
        { "PORT0", 0x4B0U, 0x0U, 0x0U },
        { "PORT1", 0x4B1U, 0x0U, 0x0U },
        { "PORT2", 0x4B2U, 0x0U, 0x0U },
        { "PORT3", 0x4B3U, 0x0U, 0x0U },
        { "PORT4", 0x4B4U, 0x0U, 0x0U },
        { "PORT5", 0x4B5U, 0x0U, 0x0U },
        { "PORT6", 0x4B6U, 0x0U, 0x0U },
        { "PORT7", 0x4B7U, 0x0U, 0x0U },
        { "PORTC", 0x0U, 0x0U, 0x4AAU }
    }}
};

} // namespace vioavr::core::devices
