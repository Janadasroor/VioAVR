#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny26 {
    .name = "ATtiny26",
    .flash_words = 1024U,
    .sram_bytes = 128U,
    .sram_start = 0x0060U,
    .eeprom_bytes = 128U,
    .interrupt_vector_count = 12U,
    .interrupt_vector_size = 2U,
    .flash_page_size = 0x20U,
    .register_file_range = { 0x0000, 0x001F },
    .io_range = { 0x0020, 0x005F },
    .extended_io_range = { 0x0060, 0x005F },

    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .sigrd_mask = 0x20U,
    .spmen_mask = 0x01U,
    .cpu_frequency_hz = 16'000'000U,

    .adc_count = 1U,
    .adcs = {{
        {
            .admux_address = 0x27U,
            .vector_index = 11U,
        }
    }},

    .ac_count = 1U,
    .acs = {{
        {
            .acsr_address = 0x28U,
        }
    }},

    .timer8_count = 1U,
    .timers8 = {{
        {
            .tcnt_address = 0x52U,
            .timer_index = 0U,
        }
    }},

    .timer16_count = 1U,
    .timers16 = {{
        {
            .tcnt_address = 0x4EU,
            .ocra_address = 0x4DU,
            .ocrb_address = 0x4CU,
            .ocrc_address = 0x4BU,
            .tifr_address = 0x58U,
            .timsk_address = 0x59U,
            .tccra_address = 0x50U,
            .tccrb_address = 0x4FU,
        }
    }},

    .ext_interrupt_count = 1U,
    .ext_interrupts = {{
        {
            .vector_indices = { 1U, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        }
    }},

    .usi_count = 1U,
    .usis = {{
        {
            .usicr_address = 0x2DU,
            .usisr_address = 0x2EU,
            .usidr_address = 0x2FU,
            .overflow_vector_index = 8U,
        }
    }},

    .eeprom_count = 1U,
    .eeproms = {{
        {
            .eecr_address = 0x3CU,
            .eedr_address = 0x3DU,
            .eearl_address = 0x3EU,
            .size = 0x80U,
        }
    }},

    .wdt_count = 1U,
    .wdts = {{
        {
            .wdtcsr_address = 0x41U,
        }
    }},

    .signature = { 0x1E, 0x91, 0x09 },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 2U,
    .ports = {{
        { "PORTA", 0x39U, 0x3AU, 0x3BU,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
,
        { "PORTB", 0x36U, 0x37U, 0x38U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
