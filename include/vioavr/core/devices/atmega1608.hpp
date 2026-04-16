#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega1608 = {
    .name = "ATmega1608",
    .flash_words = 0x2000U,
    .sram_bytes = 0x800U,
    .eeprom_bytes = 0x100U,
    .interrupt_vector_count = 39,
    .interrupt_vector_size = 4U,
    .flash_page_size = 0x40U,
    .register_file_range = { 0x0U, 0x1FU },
    .io_range = { 0x0U, 0x10FFU },
    .extended_io_range = { 0x1100U, 0x37FFU },
    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .spmcsr_address = 0x57U,
    
    .prr_address = 0x0U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    
    .smcr_address = 0x53U,
    .mcusr_address = 0x54U,
    
    .pradc_bit = 0xFFU,
    .prusart0_bit = 0xFFU,
    .prspi_bit = 0xFFU,
    .prtwi_bit = 0xFFU,
    .prtimer0_bit = 0xFFU,
    .prtimer1_bit = 0xFFU,
    .prtimer2_bit = 0xFFU,
    .smcr_sm_mask = 0x0U,
    .smcr_se_mask = 0x0U,
    .flash_rww_end_word = 0x0U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .cpu_frequency_hz = 16000000U,

    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x0U, .adch_address = 0x0U, 
            .adcsra_address = 0x0U, .adcsrb_address = 0x0U, .admux_address = 0x0U, 
            .vector_index = 0U,
            .didr0_address = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x0U,
            .didr1_address = 0x0U,
            .vector_index = 0U
        } }},
    .timer8_count = 0U,
    .timers8 = {{  }},
    .timer16_count = 0U,
    .timers16 = {{  }},
    .ext_interrupt_count = 0U,
    .ext_interrupts = {{  }},
    .uart_count = 3U,
    .uarts = {{ {
            .udr_address = 0x0U, 
            .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U,
            .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .rx_vector_index = 0U, 
            .udre_vector_index = 0U,
            .tx_vector_index = 0U, 
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U,
            .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        }, {
            .udr_address = 0x0U, 
            .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U,
            .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .rx_vector_index = 0U, 
            .udre_vector_index = 0U,
            .tx_vector_index = 0U, 
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U,
            .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        }, {
            .udr_address = 0x0U, 
            .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U,
            .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .rx_vector_index = 0U, 
            .udre_vector_index = 0U,
            .tx_vector_index = 0U, 
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U,
            .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .pcint_count = 0U,
    .pcints = {{  }},
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x0U, .spsr_address = 0x0U, .spdr_address = 0x0U,
            .vector_index = 0U,
            .spe_mask = 0x0U, .spie_mask = 0x0U, .mstr_mask = 0x0U,
            .spif_mask = 0x0U, .wcol_mask = 0x0U, .sp2x_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .twi_count = 1U,
    .twis = {{ {
            .twbr_address = 0x0U, .twsr_address = 0x0U, .twar_address = 0x0U, .twdr_address = 0x0U, .twcr_address = 0x0U, .twamr_address = 0x0U,
            .vector_index = 14U,
            .twint_mask = 0x0U, .twen_mask = 0x0U, .twie_mask = 0x0U, .twsto_mask = 0x0U, .twsta_mask = 0x0U, .twea_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    .eeprom_count = 0U,
    .eeproms = {{  }},
    .wdt_count = 1U,
    .wdts = {{ {
            .wdtcsr_address = 0x0U,
            .vector_index = 0U,
            .wdie_mask = 0x0U,
            .wde_mask = 0x0U,
            .wdce_mask = 0x0U
        } }},
    
    .fuse_address = 0xAU,
    .lockbit_address = 0x1U,
    .signature_address = 0x3U,
    .port_count = 0U,
    .ports = {{  }}
};

} // namespace vioavr::core::devices
