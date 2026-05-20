#pragma once
#include "vioavr/core/device.hpp"

namespace vioavr::core::devices {

inline constexpr DeviceDescriptor attiny85 {
    .name = "ATtiny85",
    .flash_words = 4096U, // 8 KB
    .sram_bytes = 512U,   // 512 Bytes
    .sram_start = 0x60U,
    .eeprom_bytes = 512U,
    .interrupt_vector_count = 15U,
    .interrupt_vector_size = 2U,
    .flash_page_size = 64U,
    .register_file_range = { 0x0000, 0x001F },
    .io_range = { 0x0020, 0x005F },
    .extended_io_range = { 0x60U, 0x5FU },

    .spl_address = 0x5DU,
    .sph_address = 0x5EU,
    .sreg_address = 0x5FU,
    .spmcsr_address = 0x57U,
    .sigrd_mask = 0x20U,
    .spmen_mask = 0x01U,

    .signature = { 0x1EU, 0x93U, 0x0BU }, // ATtiny85 signature
    .operating_voltage_v = 5.0,
    .vil_factor = 0.3,
    .vih_factor = 0.6,

    .port_count = 1U,
    .ports = {{
        { "PORTB", 0x36U, 0x37U, 0x38U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0x0U, 0xFFFFU, 255U }
    }}
};

} // namespace vioavr::core::devices
