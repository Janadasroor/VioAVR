#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny4 {
    .name = "ATtiny4",
    .flash_words = 256U,
    .sram_bytes = 32U,
    .sram_start = 0x0040U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 10U,
    .interrupt_vector_size = 2U,
    .flash_page_size = 0x80U,
    .register_file_range = { 0x0000, 0x001F },
    .io_range = { 0x0020, 0x005F },
    .extended_io_range = { 0x0060, 0x005F },

    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .sigrd_mask = 0x20U,
    .spmen_mask = 0x01U,
    .cpu_frequency_hz = 16'000'000U,

    .ac_count = 1U,
    .acs = {{
        {
            .acsr_address = 0x1FU,
        }
    }},

    .timer16_count = 1U,
    .timers16 = {{
        {
        }
    }},

    .ext_interrupt_count = 1U,
    .ext_interrupts = {{
        {
            .eicra_address = 0x15U,
            .eimsk_address = 0x13U,
            .eifr_address = 0x14U,
            .vector_indices = { 1U, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        }
    }},

    .wdt_count = 1U,
    .wdts = {{
        {
            .wdtcsr_address = 0x31U,
            .vector_index = 8U,
        }
    }},

    .signature = { 0x1E, 0x8F, 0x0A },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 1U,
    .ports = {{
        { "PORTB", 0x20U, 0x21U, 0x22U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
