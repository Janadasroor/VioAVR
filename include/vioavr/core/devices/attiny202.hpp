#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny202 {
    .name = "ATtiny202",
    .flash_words = 1024U,
    .sram_bytes = 128U,
    .sram_start = 0x3F80U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 26U,
    .interrupt_vector_size = 4U,
    .flash_page_size = 0x40U,
    .register_file_range = { 0x0000, 0x001F },
    .io_range = { 0x0000, 0x003F },
    .extended_io_range = { 0x0040, 0x10FF },
    .mapped_flash = { 0x8000U, 0x0800U },
    .mapped_eeprom = { 0x1400U, 0x0040U },
    .mapped_fuses = { 0x1280U, 0x000AU },
    .mapped_signatures = { 0x1100U, 0x0003U },
    .mapped_user_signatures = { 0x1300U, 0x0020U },

    .spl_address = 0x3DU,
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
            .ctrle_address = 0x604U,
            .sampctrl_address = 0x605U,
            .muxpos_address = 0x606U,
            .command_address = 0x608U,
            .evctrl_address = 0x609U,
            .intctrl_address = 0x60AU,
            .intflags_address = 0x60BU,
            .dbgctrl_address = 0x60CU,
            .calib_address = 0x616U,
            .temp_address = 0x60DU,
            .res_address = 0x610U,
            .winlt_address = 0x612U,
            .winht_address = 0x614U,
            .res_ready_vector_index = 17U,
            .wcomp_vector_index = 18U,
        }
    }},

    .ac8x_count = 1U,
    .acs8x = {{
        {
            .ctrla_address = 0x670U,
            .muxctrla_address = 0x672U,
            .intctrl_address = 0x676U,
            .status_address = 0x677U,
            .vector_index = 16U,
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

    .tcb_count = 1U,
    .timers_tcb = {{
        {
            .ctrla_address = 0xA40U,
            .ctrlb_address = 0xA41U,
            .evctrl_address = 0xA44U,
            .intctrl_address = 0xA45U,
            .intflags_address = 0xA46U,
            .status_address = 0xA47U,
            .dbgctrl_address = 0xA48U,
            .temp_address = 0xA49U,
            .cnt_address = 0xA4AU,
            .ccmp_address = 0xA4CU,
            .vector_index = 13U,
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
            .ovf_vector_index = 6U,
            .pit_vector_index = 7U,
        }
    }},

    .evsys = {
        .strobe_address = 0x180U,
        .channels_address = 0x182U,
        .users_address = 0x192U,
        .channel_count = 4U,
        .user_count = 12U,
    },

    .ccl = {
        .ctrla_address = 0x1C0U,
        .seqctrl_addresses = { 0x1C1U, 0x0U, 0x0U, 0x0U },
        .lut_count = 2U,
        .luts = {
        {
            .ctrla_address = 0x1C5U,
            .ctrlb_address = 0x1C6U,
            .ctrlc_address = 0x1C7U,
            .truth_address = 0x1C8U,
            .generator_id = 0U,
            .output_pin_address = 0x404U,
            .output_bit_index = 6U,
        },
        {
            .ctrla_address = 0x1C9U,
            .ctrlb_address = 0x1CAU,
            .ctrlc_address = 0x1CBU,
            .truth_address = 0x1CCU,
            .generator_id = 0U,
            .output_pin_address = 0x404U,
            .output_bit_index = 7U,
        },
        },
    },

    .portmux = {
        .twispiroutea_address = 0x200U,
        .usartroutea_address = 0x201U,
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

    .uart8x_count = 1U,
    .uarts8x = {{
        {
            .ctrla_address = 0x805U,
            .ctrlb_address = 0x806U,
            .ctrlc_address = 0x807U,
            .status_address = 0x804U,
            .baud_address = 0x808U,
            .rxdata_address = 0x800U,
            .txdata_address = 0x802U,
            .dbgctrl_address = 0x80BU,
            .evctrl_address = 0x80CU,
            .rx_vector_index = 22U,
            .tx_vector_index = 24U,
            .dre_vector_index = 23U,
            .txd_pin_address = 0x404U,
            .txd_pin_bit = 1U,
            .rxd_pin_address = 0x404U,
            .rxd_pin_bit = 2U,
            .index = 0U,
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
            .vector_index = 25U,
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
            .ctrla_address = 0x820U,
            .ctrlb_address = 0x821U,
            .intctrl_address = 0x822U,
            .intflags_address = 0x823U,
            .data_address = 0x824U,
            .vector_index = 21U,
            .index = 0U,
        }
    }},

    .twi8x_count = 1U,
    .twis8x = {{
        {
            .mctrla_address = 0x813U,
            .mctrlb_address = 0x814U,
            .mstatus_address = 0x815U,
            .mbaud_address = 0x816U,
            .maddr_address = 0x817U,
            .mdata_address = 0x818U,
            .sctrla_address = 0x819U,
            .sctrlb_address = 0x81AU,
            .sstatus_address = 0x81BU,
            .saddr_address = 0x81CU,
            .sdata_address = 0x81DU,
            .saddrmask_address = 0x81EU,
            .dbgctrl_address = 0x812U,
            .master_vector_index = 20U,
            .slave_vector_index = 19U,
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

    .signature = { 0x1E, 0x91, 0x23 },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 1U,
    .ports = {{
        { "PORTA", 0x408U, 0x400U, 0x404U,
          0x401U, 0x402U, 0x403U, 0x405U, 0x406U, 0x407U,
          0x00U, 0x409U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
