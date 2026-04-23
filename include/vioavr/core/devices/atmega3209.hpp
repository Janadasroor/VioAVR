#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor atmega3209 {
    .name = "ATmega3209",
    .flash_words = 16384U,
    .sram_bytes = 4096U,
    .sram_start = 0x3000U,
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
    .sigrd_mask = 0x0U,
    .blbset_mask = 0x0U,
    .spmen_mask = 0x0U,
    .prr_address = 0x0U,
    .prr0_address = 0x0U,
    .prr1_address = 0x0U,
    .smcr_address = 0x0U,
    .mcusr_address = 0x0U,
    .mcucr_address = 0x0U,
    .ccp_address = 0x34U,
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
    .boot_start_address = 0x0U,
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
            .res_ready_vector_index = 22U, .wcomp_vector_index = 23U,
            .user_event_address = 0x1A8U,
            .resrd_generator_id = 36U
        } }},
    .ac_count = 0U,
    .acs = {{  }},
    .ac8x_count = 1U,
    .acs8x = {{ {
            .ctrla_address = 0x680U, .muxctrla_address = 0x682U, .dacctrla_address = 0x684U,
            .intctrl_address = 0x686U, .status_address = 0x687U,
            .vector_index = 21U,
            .user_event_address = 0x0U,
            .out_generator_id = 32U
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
            .perbuf_address = 0xA36U, .cmp0buf_address = 0xA38U, .cmp1buf_address = 0xA3AU, .cmp2buf_address = 0xA3CU,
            .luf_ovf_vector_index = 7U, .cmp0_vector_index = 9U,
            .cmp1_vector_index = 10U, .cmp2_vector_index = 11U,
            .hunf_vector_index = 8U, .lcmp0_vector_index = 9U,
            .lcmp1_vector_index = 10U, .lcmp2_vector_index = 11U,
            .user_event_address = 0x1B3U,
            .ovf_generator_id = 128U,
            .cmp0_generator_id = 132U, .cmp1_generator_id = 133U, .cmp2_generator_id = 134U
        } }},

    .tcb_count = 4U,
    .timers_tcb = {{ {
            .ctrla_address = 0xA80U, .ctrlb_address = 0xA81U, .evctrl_address = 0xA84U,
            .intctrl_address = 0xA85U, .intflags_address = 0xA86U, .status_address = 0xA87U,
            .dbgctrl_address = 0xA88U, .temp_address = 0xA89U, .cnt_address = 0xA8AU,
            .ccmp_address = 0xA8CU, .vector_index = 12U,
            .user_event_address = 0x1B4U,
            .capt_generator_id = 160U
        },
        {
            .ctrla_address = 0xA90U, .ctrlb_address = 0xA91U, .evctrl_address = 0xA94U,
            .intctrl_address = 0xA95U, .intflags_address = 0xA96U, .status_address = 0xA97U,
            .dbgctrl_address = 0xA98U, .temp_address = 0xA99U, .cnt_address = 0xA9AU,
            .ccmp_address = 0xA9CU, .vector_index = 13U,
            .user_event_address = 0x1B5U,
            .capt_generator_id = 162U
        },
        {
            .ctrla_address = 0xAA0U, .ctrlb_address = 0xAA1U, .evctrl_address = 0xAA4U,
            .intctrl_address = 0xAA5U, .intflags_address = 0xAA6U, .status_address = 0xAA7U,
            .dbgctrl_address = 0xAA8U, .temp_address = 0xAA9U, .cnt_address = 0xAAAU,
            .ccmp_address = 0xAACU, .vector_index = 25U,
            .user_event_address = 0x1B6U,
            .capt_generator_id = 164U
        },
        {
            .ctrla_address = 0xAB0U, .ctrlb_address = 0xAB1U, .evctrl_address = 0xAB4U,
            .intctrl_address = 0xAB5U, .intflags_address = 0xAB6U, .status_address = 0xAB7U,
            .dbgctrl_address = 0xAB8U, .temp_address = 0xAB9U, .cnt_address = 0xABAU,
            .ccmp_address = 0xABCU, .vector_index = 36U,
            .user_event_address = 0x1B7U,
            .capt_generator_id = 166U
        } }},

    .rtc_count = 1U,
    .timers_rtc = {{ {
            .ctrla_address = 0x140U, .status_address = 0x141U, .intctrl_address = 0x142U,
            .intflags_address = 0x143U, .temp_address = 0x144U, .dbgctrl_address = 0x145U,
            .clksel_address = 0x147U, .cnt_address = 0x148U, .per_address = 0x14AU,
            .cmp_address = 0x14CU, .pitctrla_address = 0x150U, .pitstatus_address = 0x151U,
            .pitintctrl_address = 0x152U, .pitintflags_address = 0x153U,
            .ovf_vector_index = 7U, .pit_vector_index = 4U,
            .ovf_generator_id = 6U,
            .cmp_generator_id = 7U
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
                .ctrlc_address = 0x1CAU, .truth_address = 0x1CBU,
                .user_event_a_address = 0x1A0U, .user_event_b_address = 0x1A1U,
                .generator_id = 16U
            }, {
                .ctrla_address = 0x1CCU, .ctrlb_address = 0x1CDU,
                .ctrlc_address = 0x1CEU, .truth_address = 0x1CFU,
                .user_event_a_address = 0x1A2U, .user_event_b_address = 0x1A3U,
                .generator_id = 17U
            }, {
                .ctrla_address = 0x1D0U, .ctrlb_address = 0x1D1U,
                .ctrlc_address = 0x1D2U, .truth_address = 0x1D3U,
                .user_event_a_address = 0x1A4U, .user_event_b_address = 0x1A5U,
                .generator_id = 18U
            }, {
                .ctrla_address = 0x1D4U, .ctrlb_address = 0x1D5U,
                .ctrlc_address = 0x1D6U, .truth_address = 0x1D7U,
                .user_event_a_address = 0x1A6U, .user_event_b_address = 0x1A7U,
                .generator_id = 19U
            }, {0}, {0}, {0}, {0} },
            .vector_index = 0U
        },
    .portmux = {
            .twispiroutea_address = 0x5E3U,
            .usartroutea_address = 0x5E2U,
            .evoutroutea_address = 0x5E0U,
            .tcaroutea_address = 0x5E4U,
            .tcbroutea_address = 0x5E5U,
            .usart = {
                {
                .txd = { { 0x404U, 0x404U, 0x0U, 0x0U }, { 0U, 4U, 0xFFU, 0xFFU } },
                .rxd = { { 0x404U, 0x404U, 0x0U, 0x0U }, { 1U, 5U, 0xFFU, 0xFFU } }
            },
                {
                .txd = { { 0x444U, 0x444U, 0x0U, 0x0U }, { 0U, 4U, 0xFFU, 0xFFU } },
                .rxd = { { 0x444U, 0x444U, 0x0U, 0x0U }, { 1U, 5U, 0xFFU, 0xFFU } }
            },
                {
                .txd = { { 0x4A4U, 0x4A4U, 0x0U, 0x0U }, { 0U, 4U, 0xFFU, 0xFFU } },
                .rxd = { { 0x4A4U, 0x4A4U, 0x0U, 0x0U }, { 1U, 5U, 0xFFU, 0xFFU } }
            },
                {
                .txd = { { 0x424U, 0x424U, 0x0U, 0x0U }, { 0U, 4U, 0xFFU, 0xFFU } },
                .rxd = { { 0x424U, 0x424U, 0x0U, 0x0U }, { 1U, 5U, 0xFFU, 0xFFU } }
            }
            },
            .spi = {
                {
                .mosi = { { 0x404U, 0x444U, 0x484U, 0x0U }, { 4U, 0U, 0U, 0xFFU } },
                .miso = { { 0x404U, 0x444U, 0x484U, 0x0U }, { 5U, 1U, 1U, 0xFFU } },
                .sck = { { 0x404U, 0x444U, 0x484U, 0x0U }, { 6U, 2U, 2U, 0xFFU } },
                .ss = { { 0x404U, 0x444U, 0x484U, 0x0U }, { 7U, 3U, 3U, 0xFFU } }
            },
                {
                .mosi = { { 0x0U, 0x0U, 0x0U, 0x0U }, { 0xFFU, 0xFFU, 0xFFU, 0xFFU } },
                .miso = { { 0x0U, 0x0U, 0x0U, 0x0U }, { 0xFFU, 0xFFU, 0xFFU, 0xFFU } },
                .sck = { { 0x0U, 0x0U, 0x0U, 0x0U }, { 0xFFU, 0xFFU, 0xFFU, 0xFFU } },
                .ss = { { 0x0U, 0x0U, 0x0U, 0x0U }, { 0xFFU, 0xFFU, 0xFFU, 0xFFU } }
            }
            },
            .twi = {
                {
                .sda = { { 0x404U, 0x444U, 0x4A4U, 0x0U }, { 2U, 2U, 2U, 0xFFU } },
                .scl = { { 0x404U, 0x444U, 0x4A4U, 0x0U }, { 3U, 3U, 3U, 0xFFU } }
            },
                {
                .sda = { { 0x0U, 0x0U, 0x0U, 0x0U }, { 0xFFU, 0xFFU, 0xFFU, 0xFFU } },
                .scl = { { 0x0U, 0x0U, 0x0U, 0x0U }, { 0xFFU, 0xFFU, 0xFFU, 0xFFU } }
            }
            }
        },
    
    .vref = {
            .ctrla_address = 0xA0U,
            .ctrlb_address = 0xA1U
        },
    .clkctrl = {
            .ctrla_address = 0x60U,
            .ctrlb_address = 0x61U,
            .mclklock_address = 0x62U,
            .mclkstatus_address = 0x63U,
            .osc20mctrla_address = 0x70U,
            .osc20mcalib_address = 0x71U,
            .osc32kctrla_address = 0x78U,
            .xosc32kctrla_address = 0x7CU
        },
    .slpctrl = {
            .ctrla_address = 0x50U
        },
    .rstctrl = {
            .rstfr_address = 0x40U
        },
    .syscfg = {
            .reves_address = 0xF01U
        },
    .bod = {
            .ctrla_address = 0x80U,
            .ctrlb_address = 0x81U,
            .vlmctrla_address = 0x88U,
            .intctrl_address = 0x89U,
            .intflags_address = 0x8AU,
            .status_address = 0x8BU,
            .vlm_vector_index = 2U
        },
    
    .ext_interrupt_count = 0U,
    .ext_interrupts = {{  }},

    .uart_count = 0U,
    .uarts = {{  }},
    
    .uart8x_count = 4U,
    .uarts8x = {{ {
            .ctrla_address = 0x805U, .ctrlb_address = 0x806U, .ctrlc_address = 0x807U,
            .ctrld_address = 0x80AU, .status_address = 0x804U, .baud_address = 0x808U,
            .rxdata_address = 0x800U, .txdata_address = 0x802U, .dbgctrl_address = 0x80BU,
            .evctrl_address = 0x80CU,
            .rx_vector_index = 17U, .tx_vector_index = 19U, .dre_vector_index = 18U,
            .user_event_address = 0x1AFU,
            .txd_pin_address = 0x404U, .txd_pin_bit = 0U,
            .rxd_pin_address = 0x408U, .rxd_pin_bit = 1U,
            .index = 0U
        },
        {
            .ctrla_address = 0x825U, .ctrlb_address = 0x826U, .ctrlc_address = 0x827U,
            .ctrld_address = 0x82AU, .status_address = 0x824U, .baud_address = 0x828U,
            .rxdata_address = 0x820U, .txdata_address = 0x822U, .dbgctrl_address = 0x82BU,
            .evctrl_address = 0x82CU,
            .rx_vector_index = 26U, .tx_vector_index = 28U, .dre_vector_index = 27U,
            .user_event_address = 0x1B0U,
            .txd_pin_address = 0x444U, .txd_pin_bit = 0U,
            .rxd_pin_address = 0x448U, .rxd_pin_bit = 1U,
            .index = 1U
        },
        {
            .ctrla_address = 0x845U, .ctrlb_address = 0x846U, .ctrlc_address = 0x847U,
            .ctrld_address = 0x84AU, .status_address = 0x844U, .baud_address = 0x848U,
            .rxdata_address = 0x840U, .txdata_address = 0x842U, .dbgctrl_address = 0x84BU,
            .evctrl_address = 0x84CU,
            .rx_vector_index = 31U, .tx_vector_index = 33U, .dre_vector_index = 32U,
            .user_event_address = 0x1B1U,
            .txd_pin_address = 0x4A4U, .txd_pin_bit = 0U,
            .rxd_pin_address = 0x4A8U, .rxd_pin_bit = 1U,
            .index = 2U
        },
        {
            .ctrla_address = 0x865U, .ctrlb_address = 0x866U, .ctrlc_address = 0x867U,
            .ctrld_address = 0x86AU, .status_address = 0x864U, .baud_address = 0x868U,
            .rxdata_address = 0x860U, .txdata_address = 0x862U, .dbgctrl_address = 0x86BU,
            .evctrl_address = 0x86CU,
            .rx_vector_index = 37U, .tx_vector_index = 39U, .dre_vector_index = 38U,
            .user_event_address = 0x1B2U,
            .txd_pin_address = 0x424U, .txd_pin_bit = 0U,
            .rxd_pin_address = 0x428U, .rxd_pin_bit = 1U,
            .index = 3U
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
    
    .spi_count = 0U,
    .spis = {{  }},

    .spi8x_count = 1U,
    .spis8x = {{ {
            .ctrla_address = 0x8C0U, .ctrlb_address = 0x8C1U, .intctrl_address = 0x8C2U, .intflags_address = 0x8C3U, .data_address = 0x8C4U,
            .vector_index = 12U,
            .user_event_address = 0x0U,
            .index = 0U
        } }},
    
    .twi_count = 0U,
    .twis = {{  }},

    .twi8x_count = 1U,
    .twis8x = {{ {
            .mctrla_address = 0x8A3U, .mctrlb_address = 0x8A4U, .mstatus_address = 0x8A5U, .mbaud_address = 0x8A6U, .maddr_address = 0x8A7U, .mdata_address = 0x8A8U,
            .sctrla_address = 0x8A9U, .sctrlb_address = 0x8AAU, .sstatus_address = 0x8ABU, .saddr_address = 0x8ACU, .sdata_address = 0x8ADU, .saddrmask_address = 0x8AEU,
            .dbgctrl_address = 0x8A2U,
            .master_vector_index = 15U, .slave_vector_index = 14U,
            .user_event_address = 0x0U,
            .index = 0U
        } }},
    
    .eeprom_count = 1U,
    .eeproms = {{ {
                .size = 0x100U,
                .mapped_data = { 0x1400U, 0x100U }
            } }},
    
    .wdt_count = 0U,
    .wdts = {{  }},

    .wdt8x_count = 1U,
    .wdts8x = {{ {
            .ctrla_address = 0x100U, 
            .winctrla_address = 0x0U,
            .intctrl_address = 0x0U,
            .status_address = 0x101U,
            .vector_index = 0U
        } }},

    .crc8x_count = 1U,
    .crcs8x = {{ {
            .ctrla_address = 0x120U, .status_address = 0x122U, .data_address = 0x0U, .checksum_address = 0x0U,
            .user_event_address = 0x0U
        } }},

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

    .signature = { 0x1EU, 0x95U, 0x31U },
    .fuses = { 0x0U, 0x0U, 0x7EU, 0xFFU, 0xFFU, 0xF6U, 0xFFU, 0x0U, 0x0U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU },
    .lockbit_reset = 0xC5U,

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 6U,
    .ports = {{
        { "PORTA", 0x408U, 0x400U, 0x404U, 0x401U, 0x402U, 0x403U, 0x405U, 0x406U, 0x407U, 0x410U, 0x409U, 0x0U, 6U },
        { "PORTB", 0x428U, 0x420U, 0x424U, 0x421U, 0x422U, 0x423U, 0x425U, 0x426U, 0x427U, 0x430U, 0x429U, 0x4U, 34U },
        { "PORTC", 0x448U, 0x440U, 0x444U, 0x441U, 0x442U, 0x443U, 0x445U, 0x446U, 0x447U, 0x450U, 0x449U, 0x8U, 24U },
        { "PORTD", 0x468U, 0x460U, 0x464U, 0x461U, 0x462U, 0x463U, 0x465U, 0x466U, 0x467U, 0x470U, 0x469U, 0xCU, 20U },
        { "PORTE", 0x488U, 0x480U, 0x484U, 0x481U, 0x482U, 0x483U, 0x485U, 0x486U, 0x487U, 0x490U, 0x489U, 0x10U, 35U },
        { "PORTF", 0x4A8U, 0x4A0U, 0x4A4U, 0x4A1U, 0x4A2U, 0x4A3U, 0x4A5U, 0x4A6U, 0x4A7U, 0x4B0U, 0x4A9U, 0x14U, 29U }
    }}
};

} // namespace vioavr::core::devices
