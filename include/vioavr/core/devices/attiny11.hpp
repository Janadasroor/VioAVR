#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny11 {
    .name = "ATtiny11",
    .flash_words = 512U,
    .sram_bytes = 0U,
    .eeprom_bytes = 0U,
    .interrupt_vector_count = 5U,
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
            .acsr_address = 0x08U,
        }
    }},

    .timer8_count = 1U,
    .timers8 = {{
        {
            .tcnt_address = 0x32U,
            .overflow_vector_index = 3U,
            .timer_index = 0U,
        }
    }},

    .ext_interrupt_count = 1U,
    .ext_interrupts = {{
        {
            .vector_indices = { 1U, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        }
    }},

    .wdt_count = 1U,
    .wdts = {{
        {
            .wdtcsr_address = 0x21U,
        }
    }},

    .signature = { 0x1E, 0x90, 0x04 },

    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 1U,
    .ports = {{
        { "PORTB", 0x36U, 0x37U, 0x38U,
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
          0x00U, 0x00U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
