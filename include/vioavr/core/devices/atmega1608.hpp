#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega1608 {
    .name = "ATmega1608",
    .flash_words = 8192U,
    .sram_bytes = 2048U,
    .eeprom_bytes = 256U,
    .interrupt_vector_count = 39U,
    .interrupt_vector_size = 2U,
    .spl_address = 0x3DU,
    .sph_address = 0x3EU,
    .sreg_address = 0x3FU,
    .spmcsr_address = 0x0U,
    .prr_address = 0x0U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    .smcr_address = 0x0U,
    .mcusr_address = 0x0U,
    
    .adc_count = 1U,
    .adcs = {{ {
            .adcl_address = 0x0U, .adch_address = 0x0U, .adcsra_address = 0x0U, .adcsrb_address = 0x0U, .admux_address = 0x0U,
            .vector_index = 0U,
            .adcsra_reset = 0x0U, .adcsrb_reset = 0x0U, .admux_reset = 0x0U,
            .didr0_address = 0x0U,
            .adc_pin_address = {{ 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U }},
            .adc_pin_bit = {{ 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U }},
            .auto_trigger_map = {{ AdcAutoTriggerSource::free_running, AdcAutoTriggerSource::analog_comparator, AdcAutoTriggerSource::external_interrupt_0, AdcAutoTriggerSource::timer0_compare, AdcAutoTriggerSource::timer0_overflow, AdcAutoTriggerSource::timer1_compare_b, AdcAutoTriggerSource::timer1_overflow, AdcAutoTriggerSource::timer1_capture }},
            .adsc_mask = 0x0U, .adate_mask = 0x0U, .adif_mask = 0x0U, .adie_mask = 0x0U, .aden_mask = 0x0U, .adlar_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .ac_count = 1U,
    .acs = {{ {
            .acsr_address = 0x0U, .didr1_address = 0x0U, .vector_index = 0U,
            .ain0_pin_address = 0x0U, .ain0_pin_bit = 0U, .ain1_pin_address = 0x0U, .ain1_pin_bit = 0U,
            .aci_mask = 0x0U, .acie_mask = 0x0U
        } }},
    
    .timer8_count = 0U,
    .timers8 = {{  }},
    
    .timer16_count = 0U,
    .timers16 = {{  }},
    
    .ext_interrupt_count = 0U,
    .ext_interrupts = {{  }},

    .uart_count = 3U,
    .uarts = {{ {
            .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 0U,
            .udre_vector_index = 0U,
            .tx_vector_index = 0U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        },
        {
            .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 0U,
            .udre_vector_index = 0U,
            .tx_vector_index = 0U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        },
        {
            .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 0U,
            .udre_vector_index = 0U,
            .tx_vector_index = 0U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0x0U, .pr_bit = 0xFFU
        } }},
    
    .pcint_count = 0U,
    .pcints = {{  }},
    
    .spi_count = 1U,
    .spis = {{ {
            .spcr_address = 0x0U, .spsr_address = 0x0U, .spdr_address = 0x0U,
            .spcr_reset = 0x0U, .spsr_reset = 0x0U, .vector_index = 0U,
            .spe_mask = 0x0U, .spie_mask = 0x0U, .mstr_mask = 0x0U, .spif_mask = 0x0U, .wcol_mask = 0x0U, .sp2x_mask = 0x0U,
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
            .wdie_mask = 0x0U, .wde_mask = 0x0U
        } }},

    .port_count = 6U,
    .ports = {{
        { "PORTCTRL", 0x0U, 0x0U, 0x40AU },
        { "PORTCTRL", 0x0U, 0x0U, 0x42AU },
        { "PORTCTRL", 0x0U, 0x0U, 0x44AU },
        { "PORTCTRL", 0x0U, 0x0U, 0x46AU },
        { "PORTCTRL", 0x0U, 0x0U, 0x48AU },
        { "PORTCTRL", 0x0U, 0x0U, 0x4AAU }
    }}
};

} // namespace vioavr::core::devices
