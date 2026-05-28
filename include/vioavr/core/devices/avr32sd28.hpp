#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor avr32sd28 {
    .name = "AVR32SD28",
    .flash_words = 16384U,
    .sram_bytes = 4096U,
    .sram_start = 0x7000U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 54U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 0x200U,
    .register_file_range = { 0x0000, 0x001F },
    .io_range = { 0x0000, 0x003F },
    .extended_io_range = { 0x0040, 0x103E },
    .mapped_flash = { 0x8000U, 0x8000U },
    .mapped_eeprom = { 0x1400U, 0x0100U },
    .mapped_fuses = { 0x1050U, 0x0010U },
    .mapped_signatures = { 0x1080U, 0x0003U },
    .mapped_user_signatures = { 0x1200U, 0x0200U },

    .spl_address = 0x3DU,
    .sph_address = 0x3DU,
    .sreg_address = 0x3FU,
    .ccp_address = 0x34U,
    .cpu_frequency_hz = 16'000'000U,

    .adc10b_count = 2U,
    .adcs10b = {{
        {
            .ctrla_address = 0x600U,
            .ctrlb_address = 0x601U,
            .ctrlc_address = 0x602U,
            .ctrld_address = 0x603U,
            .ctrle_address = 0x604U,
            .ctrlf_address = 0x605U,
            .intctrl_address = 0x606U,
            .intflags_address = 0x607U,
            .status_address = 0x608U,
            .dbgctrl_address = 0x609U,
            .command_address = 0x60AU,
            .muxpos_address = 0x60BU,
            .result_address = 0x60CU,
            .sample_address = 0x60EU,
            .winlt_address = 0x610U,
            .winht_address = 0x612U,
            .temp_address = 0x614U,
            .resrdy_vector_index = 36U,
            .samprdy_vector_index = 37U,
        },
        {
            .ctrla_address = 0x640U,
            .ctrlb_address = 0x641U,
            .ctrlc_address = 0x642U,
            .ctrld_address = 0x643U,
            .ctrle_address = 0x644U,
            .ctrlf_address = 0x645U,
            .intctrl_address = 0x646U,
            .intflags_address = 0x647U,
            .status_address = 0x648U,
            .dbgctrl_address = 0x649U,
            .command_address = 0x64AU,
            .muxpos_address = 0x64BU,
            .result_address = 0x64CU,
            .sample_address = 0x64EU,
            .winlt_address = 0x650U,
            .winht_address = 0x652U,
            .temp_address = 0x654U,
            .resrdy_vector_index = 39U,
            .samprdy_vector_index = 40U,
        }
    }},

    .ac8x_count = 3U,
    .acs8x = {{
        {
            .ctrla_address = 0x680U,
            .intctrl_address = 0x686U,
            .status_address = 0x687U,
            .vector_index = 32U,
        },
        {
            .ctrla_address = 0x688U,
            .intctrl_address = 0x68EU,
            .status_address = 0x68FU,
            .vector_index = 33U,
        },
        {
            .ctrla_address = 0x690U,
            .intctrl_address = 0x696U,
            .status_address = 0x697U,
            .vector_index = 34U,
        }
    }},

    .tca_count = 1U,
    .timers_tca = {{
        {
            .ctrla_address = 0xA00U,
            .ctrlb_address = 0xA01U,
            .ctrlc_address = 0xA02U,
            .ctrld_address = 0xA03U,
            .ctrleclr_address = 0xA04U,
            .ctrleset_address = 0xA05U,
            .ctrlfclr_address = 0xA06U,
            .ctrlfset_address = 0xA07U,
            .evctrl_address = 0xA09U,
            .intctrl_address = 0xA0AU,
            .intflags_address = 0xA0BU,
            .dbgctrl_address = 0xA0EU,
            .temp_address = 0xA0FU,
            .tcnt_address = 0xA20U,
            .period_address = 0xA26U,
            .cmp0_address = 0xA28U,
            .cmp1_address = 0xA2AU,
            .cmp2_address = 0xA2CU,
            .perbuf_address = 0xA36U,
            .cmp0buf_address = 0xA38U,
            .cmp1buf_address = 0xA3AU,
            .cmp2buf_address = 0xA3CU,
            .luf_ovf_vector_index = 15U,
            .cmp0_vector_index = 17U,
            .cmp1_vector_index = 18U,
            .cmp2_vector_index = 19U,
            .hunf_vector_index = 16U,
        }
    }},

    .tcb_count = 4U,
    .timers_tcb = {{
        {
            .ctrla_address = 0xB00U,
            .ctrlb_address = 0xB01U,
            .evctrl_address = 0xB04U,
            .intctrl_address = 0xB05U,
            .intflags_address = 0xB06U,
            .status_address = 0xB07U,
            .dbgctrl_address = 0xB08U,
            .temp_address = 0xB09U,
            .cnt_address = 0xB0AU,
            .ccmp_address = 0xB0CU,
            .vector_index = 20U,
        },
        {
            .ctrla_address = 0xB10U,
            .ctrlb_address = 0xB11U,
            .evctrl_address = 0xB14U,
            .intctrl_address = 0xB15U,
            .intflags_address = 0xB16U,
            .status_address = 0xB17U,
            .dbgctrl_address = 0xB18U,
            .temp_address = 0xB19U,
            .cnt_address = 0xB1AU,
            .ccmp_address = 0xB1CU,
            .vector_index = 21U,
        },
        {
            .ctrla_address = 0xB20U,
            .ctrlb_address = 0xB21U,
            .evctrl_address = 0xB24U,
            .intctrl_address = 0xB25U,
            .intflags_address = 0xB26U,
            .status_address = 0xB27U,
            .dbgctrl_address = 0xB28U,
            .temp_address = 0xB29U,
            .cnt_address = 0xB2AU,
            .ccmp_address = 0xB2CU,
            .vector_index = 48U,
        },
        {
            .ctrla_address = 0xB30U,
            .ctrlb_address = 0xB31U,
            .evctrl_address = 0xB34U,
            .intctrl_address = 0xB35U,
            .intflags_address = 0xB36U,
            .status_address = 0xB37U,
            .dbgctrl_address = 0xB38U,
            .temp_address = 0xB39U,
            .cnt_address = 0xB3AU,
            .ccmp_address = 0xB3CU,
            .vector_index = 49U,
        }
    }},

    .tcd_count = 1U,
    .timers_tcd = {{
        {
            .base_address = 0x0B80U,
            .ovf_vector_index = 22U,
            .trig_vector_index = 23U,
        }
    }},

    .rtc_count = 1U,
    .timers_rtc = {{
        {
            .ctrla_address = 0x140U,
            .status_address = 0x141U,
            .intctrl_address = 0x142U,
            .intflags_address = 0x143U,
            .temp_address = 0x144U,
            .dbgctrl_address = 0x145U,
            .clksel_address = 0x147U,
            .cnt_address = 0x148U,
            .per_address = 0x14AU,
            .cmp_address = 0x14CU,
            .pitctrla_address = 0x150U,
            .pitstatus_address = 0x151U,
            .pitintctrl_address = 0x152U,
            .pitintflags_address = 0x153U,
            .ovf_vector_index = 11U,
            .pit_vector_index = 12U,
        }
    }},

    .evsys = {
    },

    .ccl = {
        .ctrla_address = 0x1C0U,
        .seqctrl_addresses = { 0x1C1U, 0x0U, 0x0U, 0x0U },
        .lut_count = 6U,
        .luts = {
        {
            .ctrla_address = 0x1C8U,
            .ctrlb_address = 0x1C9U,
            .ctrlc_address = 0x1CAU,
            .truth_address = 0x1CBU,
            .generator_id = 0U,
            .output_pin_address = 0x404U,
            .output_bit_index = 3U,
        },
        {
            .ctrla_address = 0x1CCU,
            .ctrlb_address = 0x1CDU,
            .ctrlc_address = 0x1CEU,
            .truth_address = 0x1CFU,
            .generator_id = 0U,
            .output_pin_address = 0x444U,
            .output_bit_index = 3U,
        },
        {
            .ctrla_address = 0x1D0U,
            .ctrlb_address = 0x1D1U,
            .ctrlc_address = 0x1D2U,
            .truth_address = 0x1D3U,
            .generator_id = 0U,
            .output_pin_address = 0x464U,
            .output_bit_index = 3U,
        },
        {
            .ctrla_address = 0x1D4U,
            .ctrlb_address = 0x1D5U,
            .ctrlc_address = 0x1D6U,
            .truth_address = 0x1D7U,
            .generator_id = 0U,
        },
        {
            .ctrla_address = 0x1D8U,
            .ctrlb_address = 0x1D9U,
            .ctrlc_address = 0x1DAU,
            .truth_address = 0x1DBU,
            .generator_id = 0U,
        },
        {
            .ctrla_address = 0x1DCU,
            .ctrlb_address = 0x1DDU,
            .ctrlc_address = 0x1DEU,
            .truth_address = 0x1DFU,
            .generator_id = 0U,
        },
        },
    },

    .portmux = {
        .twispiroutea_address = 0U,
        .usartroutea_address = 0x5E2U,
        .evoutroutea_address = 0x5E0U,
        .tcaroutea_address = 0x5E7U,
        .tcbroutea_address = 0x5E8U,
        .usart = {},
        .spi = {},
        .twi = {},
    },

    .vref = {
    },

    .clkctrl = {
        .ctrla_address = 0x80U,
        .ctrlb_address = 0x81U,
        .mclkstatus_address = 0x85U,
        .osc20mctrla_address = 0xC0U,
        .osc20mcalib_address = 0xC1U,
        .osc32kctrla_address = 0xD0U,
        .xosc32kctrla_address = 0xD4U,
        .xoschfctrla_address = 0xD8U,
        .mclkctrlc_address = 0x82U,
        .mclkintctrl_address = 0x83U,
        .mclkintflags_address = 0x84U,
        .mclktimebase_address = 0x86U,
        .pllctrla_address = 0xC8U,
    },

    .slpctrl = {
        .ctrla_address = 0x50U,
    },

    .rstctrl = {
        .rstfr_address = 0x40U,
        .swrr_address = 0x41U,
    },

    .bod = {
        .ctrla_address = 0x60U,
        .ctrlb_address = 0x61U,
        .vlmctrla_address = 0x68U,
        .intctrl_address = 0x69U,
        .intflags_address = 0x6AU,
        .status_address = 0x6BU,
        .vlm_vector_index = 2U,
    },

    .mvio = {
        .intctrl_address = 0x100U,
        .intflags_address = 0x101U,
        .status_address = 0x102U,
        .vector_index = 10U,
    },

    .uart8x_count = 3U,
    .uarts8x = {{
        {
            .ctrla_address = 0x805U,
            .ctrlb_address = 0x806U,
            .ctrlc_address = 0x807U,
            .ctrld_address = 0x80AU,
            .status_address = 0x804U,
            .baud_address = 0x808U,
            .rxdata_address = 0x800U,
            .txdata_address = 0x802U,
            .dbgctrl_address = 0x80BU,
            .evctrl_address = 0x80CU,
            .rx_vector_index = 28U,
            .tx_vector_index = 30U,
            .dre_vector_index = 29U,
            .txd_pin_address = 0x404U,
            .txd_pin_bit = 0U,
            .rxd_pin_address = 0x404U,
            .rxd_pin_bit = 1U,
            .index = 0U,
        },
        {
            .ctrla_address = 0x825U,
            .ctrlb_address = 0x826U,
            .ctrlc_address = 0x827U,
            .ctrld_address = 0x82AU,
            .status_address = 0x824U,
            .baud_address = 0x828U,
            .rxdata_address = 0x820U,
            .txdata_address = 0x822U,
            .dbgctrl_address = 0x82BU,
            .evctrl_address = 0x82CU,
            .rx_vector_index = 43U,
            .tx_vector_index = 45U,
            .dre_vector_index = 44U,
            .txd_pin_address = 0x444U,
            .txd_pin_bit = 0U,
            .rxd_pin_address = 0x444U,
            .rxd_pin_bit = 1U,
            .index = 1U,
        },
        {
            .ctrla_address = 0x845U,
            .ctrlb_address = 0x846U,
            .ctrlc_address = 0x847U,
            .ctrld_address = 0x84AU,
            .status_address = 0x844U,
            .baud_address = 0x848U,
            .rxdata_address = 0x840U,
            .txdata_address = 0x842U,
            .dbgctrl_address = 0x84BU,
            .evctrl_address = 0x84CU,
            .rx_vector_index = 51U,
            .tx_vector_index = 53U,
            .dre_vector_index = 52U,
            .txd_pin_address = 0x4A4U,
            .txd_pin_bit = 0U,
            .rxd_pin_address = 0x4A4U,
            .rxd_pin_bit = 1U,
            .index = 2U,
        }
    }},

    .nvm_ctrl_count = 1U,
    .nvm_ctrls = {{
        {
            .ctrla_address = 0x1000U,
            .ctrlb_address = 0x1001U,
            .status_address = 0x1007U,
            .intctrl_address = 0x1004U,
            .intflags_address = 0x1005U,
            .addr_address = 0x100CU,
            .data_address = 0x1008U,
        }
    }},

    .cpu_int_count = 1U,
    .cpu_ints = {{
        {
            .ctrla_address = 0x120U,
            .status_address = 0x121U,
            .lvl0pri_address = 0x122U,
            .lvl1vec_address = 0x123U,
        }
    }},

    .spi8x_count = 2U,
    .spis8x = {{
        {
            .ctrla_address = 0x940U,
            .ctrlb_address = 0x941U,
            .intctrl_address = 0x942U,
            .intflags_address = 0x943U,
            .data_address = 0x944U,
            .vector_index = 26U,
            .index = 0U,
        },
        {
            .ctrla_address = 0x960U,
            .ctrlb_address = 0x961U,
            .intctrl_address = 0x962U,
            .intflags_address = 0x963U,
            .data_address = 0x964U,
            .vector_index = 27U,
            .index = 1U,
        }
    }},

    .twi8x_count = 1U,
    .twis8x = {{
        {
            .mctrla_address = 0x903U,
            .mctrlb_address = 0x904U,
            .mstatus_address = 0x905U,
            .mbaud_address = 0x906U,
            .maddr_address = 0x907U,
            .mdata_address = 0x908U,
            .sctrla_address = 0x909U,
            .sctrlb_address = 0x90AU,
            .sstatus_address = 0x90BU,
            .saddr_address = 0x90CU,
            .sdata_address = 0x90DU,
            .saddrmask_address = 0x90EU,
            .dbgctrl_address = 0x902U,
            .master_vector_index = 25U,
            .slave_vector_index = 24U,
            .index = 0U,
        }
    }},

    .wdt8x_count = 1U,
    .wdts8x = {{
        {
            .ctrla_address = 0x110U,
            .status_address = 0x112U,
        }
    }},

    .dac8x_count = 1U,
    .dacs8x = {{
        {
            .ctrla_address = 0x6A0U,
            .data_address = 0x6A2U,
        }
    }},

    .zcd_count = 2U,
    .zcds = {{
        {
            .ctrla_address = 0x6C0U,
            .intctrl_address = 0x6C2U,
            .status_address = 0x6C3U,
            .vector_index = 50U,
        },
        {
            .ctrla_address = 0x6D8U,
            .intctrl_address = 0x6DAU,
            .status_address = 0x6DBU,
            .vector_index = 41U,
        }
    }},

    .signature = { 0x1E, 0x95, 0x53 },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 4U,
    .ports = {{
        { "PORTA", 0x408U, 0x400U, 0x404U,
          0x401U, 0x402U, 0x403U, 0x405U, 0x406U, 0x407U,
          0x00U, 0x409U, 0x0000U, 255U }
,
        { "PORTC", 0x448U, 0x440U, 0x444U,
          0x441U, 0x442U, 0x443U, 0x445U, 0x446U, 0x447U,
          0x00U, 0x449U, 0x0008U, 255U }
,
        { "PORTD", 0x468U, 0x460U, 0x464U,
          0x461U, 0x462U, 0x463U, 0x465U, 0x466U, 0x467U,
          0x00U, 0x469U, 0x000CU, 255U }
,
        { "PORTF", 0x4A8U, 0x4A0U, 0x4A4U,
          0x4A1U, 0x4A2U, 0x4A3U, 0x4A5U, 0x4A6U, 0x4A7U,
          0x00U, 0x4A9U, 0x0014U, 255U }
    }}
};

} // namespace vioavr::core::devices
