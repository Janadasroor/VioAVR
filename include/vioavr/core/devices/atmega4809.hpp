#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega4809 {
    .name = "ATmega4809",
    .flash_words = 24576U,
    .sram_bytes = 6144U,
    .eeprom_bytes = 256U,
    .interrupt_vector_count = 43U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 128U,
    .io_range = { 0x0U, 0x3FU },
    .extended_io_range = { 0x40U, 0x10FFU },

    .mapped_flash = { 0x4000U, 0xC000U },
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
    .flash_rww_end_word = 0x6000U,
    .spl_reset = 0x0U,
    .sph_reset = 0x0U,
    .sreg_reset = 0x0U,
    .adc_count = 0U,
    .adcs = {{  }},
    .adc8x_count = 1U,
    .adcs8x = {{ {
            .ctrla_address = 0x600U, .ctrlb_address = 0x601U, .ctrlc_address = 0x602U, .ctrld_address = 0x603U, .ctrle_address = 0x604U,
            .sampctrl_address = 0x605U, .muxpos_address = 0x606U, .muxneg_address = 0x0U, .command_address = 0x608U, .evctrl_address = 0x609U,
            .intctrl_address = 0x60AU, .intflags_address = 0x60BU, .dbgctrl_address = 0x60CU, .temp_address = 0x60DU,
            .res_address = 0x610U, .winlt_address = 0x612U, .winht_address = 0x614U,
            .res_ready_vector_index = 0U, .wcomp_vector_index = 0U,
            .user_event_address = 0x1A8U
        } }},
    .ac_count = 0U,
    .acs = {{  }},
    .ac8x_count = 1U,
    .acs8x = {{ {
            .ctrla_address = 0x680U, .muxctrla_address = 0x682U,
            .dacctrla_address = 0x684U, .intctrl_address = 0x686U,
            .status_address = 0x687U, .vector_index = 10U,
            .user_event_address = 0x0U
        } }},
    .timer8_count = 0U,
    .timers8 = {{  }},
    .timer16_count = 0U,
    .timers16 = {{  }},
    .timer10_count = 0U,
    .timers10 = {{  }},
    .tca_count = 1U,
    .timers_tca = {{ {
            .ctrla_address = 0xA00U, .ctrlb_address = 0xA01U, .ctrlc_address = 0xA02U,
            .ctrld_address = 0xA03U, .ctrleclr_address = 0xA04U, .ctrleset_address = 0xA05U,
            .ctrlfclr_address = 0xA06U, .ctrlfset_address = 0xA07U, .evctrl_address = 0xA09U,
            .intctrl_address = 0xA0AU, .intflags_address = 0xA0BU, .dbgctrl_address = 0xA0EU,
            .temp_address = 0xA0FU, .tcnt_address = 0xA20U, .period_address = 0xA26U,
            .cmp0_address = 0xA28U, .cmp1_address = 0xA2AU, .cmp2_address = 0xA2CU,
            .luf_ovf_vector_index = 0U, .cmp0_vector_index = 0U,
            .cmp1_vector_index = 0U, .cmp2_vector_index = 0U,
            .hunf_vector_index = 0U, .lcmp0_vector_index = 0U,
            .lcmp1_vector_index = 0U, .lcmp2_vector_index = 0U,
            .user_event_address = 0x1B3U
        } }},

    .tcb_count = 4U,
    .timers_tcb = {{ {
            .ctrla_address = 0xA80U, .ctrlb_address = 0xA81U, .evctrl_address = 0xA84U,
            .intctrl_address = 0xA85U, .intflags_address = 0xA86U, .status_address = 0xA87U,
            .dbgctrl_address = 0xA88U, .temp_address = 0xA89U, .cnt_address = 0xA8AU,
            .ccmp_address = 0xA8CU, .vector_index = 0U,
            .user_event_address = 0x1B4U
        },
        {
            .ctrla_address = 0xA90U, .ctrlb_address = 0xA91U, .evctrl_address = 0xA94U,
            .intctrl_address = 0xA95U, .intflags_address = 0xA96U, .status_address = 0xA97U,
            .dbgctrl_address = 0xA98U, .temp_address = 0xA99U, .cnt_address = 0xA9AU,
            .ccmp_address = 0xA9CU, .vector_index = 0U,
            .user_event_address = 0x1B5U
        },
        {
            .ctrla_address = 0xAA0U, .ctrlb_address = 0xAA1U, .evctrl_address = 0xAA4U,
            .intctrl_address = 0xAA5U, .intflags_address = 0xAA6U, .status_address = 0xAA7U,
            .dbgctrl_address = 0xAA8U, .temp_address = 0xAA9U, .cnt_address = 0xAAAU,
            .ccmp_address = 0xAACU, .vector_index = 0U,
            .user_event_address = 0x1B6U
        },
        {
            .ctrla_address = 0xAB0U, .ctrlb_address = 0xAB1U, .evctrl_address = 0xAB4U,
            .intctrl_address = 0xAB5U, .intflags_address = 0xAB6U, .status_address = 0xAB7U,
            .dbgctrl_address = 0xAB8U, .temp_address = 0xAB9U, .cnt_address = 0xABAU,
            .ccmp_address = 0xABCU, .vector_index = 0U,
            .user_event_address = 0x1B7U
        } }},

    .rtc_count = 1U,
    .timers_rtc = {{ {
            .ctrla_address = 0x140U, .status_address = 0x141U,
            .intctrl_address = 0x142U, .intflags_address = 0x143U,
            .temp_address = 0x144U, .dbgctrl_address = 0x145U,
            .clksel_address = 0x147U, .cnt_address = 0x148U,
            .per_address = 0x14AU, .cmp_address = 0x14CU,
            .pitctrla_address = 0x150U, .pitstatus_address = 0x151U,
            .pitintctrl_address = 0x152U, .pitintflags_address = 0x153U,
            .ovf_vector_index = 3U, .pit_vector_index = 4U
        } }},

    .evsys = {
            .strobe_address = 0x180U,
            .channels_address = 0x190U,
            .users_address = 0x1A0U,
            .channel_count = 8U,
            .user_count = 24U
        },

    .ccl = {
            .ctrla_address = 0x1C0U,
            .seqctrl_addresses = { 0x1C1U, 0x1C2U, 0x0U, 0x0U },
            .intctrl_addresses = { 0x1C5U, 0 },
            .intflags_addresses = { 0x1C7U, 0 },
            .lut_count = 4U,
            .luts = { {
                .ctrla_address = 0x1C8U, .ctrlb_address = 0x1C9U,
                .ctrlc_address = 0x1CAU, .truth_address = 0x1CBU
            }, {
                .ctrla_address = 0x1CCU, .ctrlb_address = 0x1CDU,
                .ctrlc_address = 0x1CEU, .truth_address = 0x1CFU
            }, {
                .ctrla_address = 0x1D0U, .ctrlb_address = 0x1D1U,
                .ctrlc_address = 0x1D2U, .truth_address = 0x1D3U
            }, {
                .ctrla_address = 0x1D4U, .ctrlb_address = 0x1D5U,
                .ctrlc_address = 0x1D6U, .truth_address = 0x1D7U
            }, {0}, {0}, {0}, {0} },
            .vector_index = 0U
        },
    
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
            .pr_address = 0, .pr_bit = 255,
            .uart_index = 0U
        },
        {
            .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 0U,
            .udre_vector_index = 0U,
            .tx_vector_index = 0U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255,
            .uart_index = 1U
        },
        {
            .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 0U,
            .udre_vector_index = 0U,
            .tx_vector_index = 0U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255,
            .uart_index = 2U
        },
        {
            .udr_address = 0x0U, .ucsra_address = 0x0U, .ucsrb_address = 0x0U, .ucsrc_address = 0x0U, .ubrrl_address = 0x0U, .ubrrh_address = 0x0U,
            .ucsra_reset = 0x0U, .ucsrb_reset = 0x0U, .ucsrc_reset = 0x0U,
            .rx_vector_index = 0U,
            .udre_vector_index = 0U,
            .tx_vector_index = 0U,
            .u2x_mask = 0x0U, .rxc_mask = 0x0U, .txc_mask = 0x0U, .udre_mask = 0x0U,
            .rxen_mask = 0x0U, .txen_mask = 0x0U, .rxcie_mask = 0x0U, .txcie_mask = 0x0U, .udrie_mask = 0x0U,
            .pr_address = 0, .pr_bit = 255,
            .uart_index = 3U
        } }},

    .nvm_ctrl_count = 1U,
    .nvm_ctrls = {{ {
            .ctrla_address = 0x1000U,
            .ctrlb_address = 0x1001U,
            .status_address = 0x1002U,
            .intctrl_address = 0x1003U,
            .intflags_address = 0x1004U,
            .addr_address = 0x1008U,
            .data_address = 0x1006U,
            .vector_index = 0U
        } }},
    .cpu_int_count = 1U,
    .cpu_ints = {{ {
            .ctrla_address = 0x110U,
            .status_address = 0x111U,
            .lvl0pri_address = 0x112U,
            .lvl1vec_address = 0x113U
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

    .signature = { 0x1EU, 0x96U, 0x51U },
    .fuses = { 0x0U, 0x0U, 0x7EU, 0xFFU, 0xFFU, 0xF6U, 0xFFU, 0x0U, 0x0U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU },

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
