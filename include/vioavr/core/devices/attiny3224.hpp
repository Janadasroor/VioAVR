#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny3224 {
    .name = "ATtiny3224",
    .flash_words = 16384U,
    .sram_bytes = 3072U,
    .sram_start = 0x3400U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 30U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 0x80U,
    .register_file_range = { 0x0000, 0x001F },
    .io_range = { 0x0000, 0x003F },
    .extended_io_range = { 0x0040, 0x10FF },
    .mapped_flash = { 0x8000U, 0x8000U },
    .mapped_eeprom = { 0x1400U, 0x0100U },
    .mapped_fuses = { 0x1280U, 0x000AU },
    .mapped_signatures = { 0x1100U, 0x0003U },
    .mapped_user_signatures = { 0x1300U, 0x0020U },

    .spl_address = 0x00U,
    .sph_address = 0x00U,
    .sreg_address = 0x3FU,
    .ccp_address = 0x34U,
    .cpu_frequency_hz = 16'000'000U,

    .adc8x_count = 1U,
    .adcs8x = {{
        {
            .ctrla_address = 0x600U,
            .ctrlb_address = 0x601U,
            .ctrlc_address = 0x602U,
            .ctrld_address = 0x603U,
            .ctrle_address = 0x608U,
            .muxpos_address = 0x60CU,
            .command_address = 0x60AU,
            .intctrl_address = 0x604U,
            .intflags_address = 0x605U,
            .dbgctrl_address = 0x607U,
            .winlt_address = 0x61CU,
            .winht_address = 0x61EU,
            .res_ready_vector_index = 22U,
        }
    }},

    .ac8x_count = 1U,
    .acs8x = {{
        {
            .ctrla_address = 0x680U,
            .muxctrla_address = 0x682U,
            .intctrl_address = 0x686U,
            .status_address = 0x687U,
            .vector_index = 20U,
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
            .luf_ovf_vector_index = 8U,
            .cmp0_vector_index = 10U,
            .cmp1_vector_index = 11U,
            .cmp2_vector_index = 12U,
            .hunf_vector_index = 9U,
        }
    }},

    .tcb_count = 2U,
    .timers_tcb = {{
        {
            .ctrla_address = 0xA80U,
            .ctrlb_address = 0xA81U,
            .evctrl_address = 0xA84U,
            .intctrl_address = 0xA85U,
            .intflags_address = 0xA86U,
            .status_address = 0xA87U,
            .dbgctrl_address = 0xA88U,
            .temp_address = 0xA89U,
            .cnt_address = 0xA8AU,
            .ccmp_address = 0xA8CU,
            .vector_index = 13U,
        },
        {
            .ctrla_address = 0xA90U,
            .ctrlb_address = 0xA91U,
            .evctrl_address = 0xA94U,
            .intctrl_address = 0xA95U,
            .intflags_address = 0xA96U,
            .status_address = 0xA97U,
            .dbgctrl_address = 0xA98U,
            .temp_address = 0xA99U,
            .cnt_address = 0xA9AU,
            .ccmp_address = 0xA9CU,
            .vector_index = 25U,
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
            .ovf_vector_index = 3U,
            .pit_vector_index = 4U,
        }
    }},

    .evsys = {
    },

    .ccl = {
        .ctrla_address = 0x1C0U,
        .seqctrl_addresses = { 0x1C1U, 0x0U, 0x0U, 0x0U },
        .lut_count = 4U,
        .luts = {
        {
            .ctrla_address = 0x1C8U,
            .ctrlb_address = 0x1C9U,
            .ctrlc_address = 0x1CAU,
            .truth_address = 0x1CBU,
            .generator_id = 0U,
            .output_pin_address = 0x404U,
            .output_bit_index = 4U,
        },
        {
            .ctrla_address = 0x1CCU,
            .ctrlb_address = 0x1CDU,
            .ctrlc_address = 0x1CEU,
            .truth_address = 0x1CFU,
            .generator_id = 0U,
            .output_pin_address = 0x404U,
            .output_bit_index = 7U,
        },
        {
            .ctrla_address = 0x1D0U,
            .ctrlb_address = 0x1D1U,
            .ctrlc_address = 0x1D2U,
            .truth_address = 0x1D3U,
            .generator_id = 0U,
            .output_pin_address = 0x424U,
            .output_bit_index = 3U,
        },
        {
            .ctrla_address = 0x1D4U,
            .ctrlb_address = 0x1D5U,
            .ctrlc_address = 0x1D6U,
            .truth_address = 0x1D7U,
            .generator_id = 0U,
            .output_pin_address = 0x404U,
            .output_bit_index = 5U,
        },
        },
    },

    .portmux = {
        .usartroutea_address = 0x5E2U,
        .tca_wo_pin_bit = {3, 2, 1, 0, 5, 4},
    },

    .vref = {
        .ctrla_address = 0xA0U,
        .ctrlb_address = 0xA1U,
    },

    .clkctrl = {
        .ctrla_address = 0x60U,
        .ctrlb_address = 0x61U,
        .mclklock_address = 0x62U,
        .mclkstatus_address = 0x63U,
        .osc20mctrla_address = 0x70U,
        .osc20mcalib_address = 0x71U,
        .osc32kctrla_address = 0x78U,
        .xosc32kctrla_address = 0x7CU,
    },

    .slpctrl = {
        .ctrla_address = 0x50U,
    },

    .rstctrl = {
        .rstfr_address = 0x40U,
        .swrr_address = 0x41U,
    },

    .bod = {
        .ctrla_address = 0x80U,
        .ctrlb_address = 0x81U,
        .vlmctrla_address = 0x88U,
        .intctrl_address = 0x89U,
        .intflags_address = 0x8AU,
        .status_address = 0x8BU,
        .vlm_vector_index = 2U,
    },

    .uart8x_count = 2U,
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
            .rx_vector_index = 17U,
            .tx_vector_index = 19U,
            .dre_vector_index = 18U,
            .txd_pin_address = 0x404U,
            .txd_pin_bit = 1U,
            .rxd_pin_address = 0x404U,
            .rxd_pin_bit = 2U,
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
            .rx_vector_index = 26U,
            .tx_vector_index = 28U,
            .dre_vector_index = 27U,
            .txd_pin_address = 0x404U,
            .txd_pin_bit = 1U,
            .rxd_pin_address = 0x404U,
            .rxd_pin_bit = 2U,
            .index = 1U,
        }
    }},

    .nvm_ctrl_count = 1U,
    .nvm_ctrls = {{
        {
            .ctrla_address = 0x1000U,
            .ctrlb_address = 0x1001U,
            .status_address = 0x1002U,
            .intctrl_address = 0x1003U,
            .intflags_address = 0x1004U,
            .addr_address = 0x1008U,
            .data_address = 0x1006U,
            .vector_index = 29U,
        }
    }},

    .cpu_int_count = 1U,
    .cpu_ints = {{
        {
            .ctrla_address = 0x110U,
            .status_address = 0x111U,
            .lvl0pri_address = 0x112U,
            .lvl1vec_address = 0x113U,
        }
    }},

    .spi8x_count = 1U,
    .spis8x = {{
        {
            .ctrla_address = 0x8C0U,
            .ctrlb_address = 0x8C1U,
            .intctrl_address = 0x8C2U,
            .intflags_address = 0x8C3U,
            .data_address = 0x8C4U,
            .vector_index = 16U,
            .index = 0U,
        }
    }},

    .twi8x_count = 1U,
    .twis8x = {{
        {
            .mctrla_address = 0x8A3U,
            .mctrlb_address = 0x8A4U,
            .mstatus_address = 0x8A5U,
            .mbaud_address = 0x8A6U,
            .maddr_address = 0x8A7U,
            .mdata_address = 0x8A8U,
            .sctrla_address = 0x8A9U,
            .sctrlb_address = 0x8AAU,
            .sstatus_address = 0x8ABU,
            .saddr_address = 0x8ACU,
            .sdata_address = 0x8ADU,
            .saddrmask_address = 0x8AEU,
            .dbgctrl_address = 0x8A2U,
            .master_vector_index = 15U,
            .slave_vector_index = 14U,
            .index = 0U,
        }
    }},

    .wdt8x_count = 1U,
    .wdts8x = {{
        {
            .ctrla_address = 0x100U,
            .status_address = 0x101U,
        }
    }},

    .signature = { 0x1E, 0x95, 0x28 },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 3U,
    .ports = {{
        { "PORTA", 0x408U, 0x400U, 0x404U,
          0x401U, 0x402U, 0x403U, 0x405U, 0x406U, 0x407U,
          0x00U, 0x409U, 0xFFFFU, 255U }
,
        { "PORTB", 0x428U, 0x420U, 0x424U,
          0x421U, 0x422U, 0x423U, 0x425U, 0x426U, 0x427U,
          0x00U, 0x429U, 0xFFFFU, 255U }
,
        { "PORTC", 0x448U, 0x440U, 0x444U,
          0x441U, 0x442U, 0x443U, 0x445U, 0x446U, 0x447U,
          0x00U, 0x449U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
