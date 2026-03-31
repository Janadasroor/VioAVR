#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/spi.hpp"
#include "vioavr/core/devices/atmega328.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_out(const u8 io_offset, const u8 source) {
    return static_cast<u16>(0xB800U | ((static_cast<u16>(source) & 0x1FU) << 4U) | ((static_cast<u16>(io_offset) & 0x30U) << 5U) | (io_offset & 0x0FU));
}

constexpr u16 encode_in(const u8 destination, const u8 io_offset) {
    return static_cast<u16>(0xB000U | ((static_cast<u16>(destination) & 0x1FU) << 4U) | ((static_cast<u16>(io_offset) & 0x30U) << 5U) | (io_offset & 0x0FU));
}

} // namespace

TEST_CASE("SPI Peripheral Functional Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    Spi spi {"SPI", atmega328};
    bus.attach_peripheral(spi);
    AvrCpu cpu {bus};

    SUBCASE("SPI Master Transmission and Flag Handling") {
        bus.load_image(HexImage {
            .flash_words = {
                encode_ldi(16, 0x50), // SPE=1, MSTR=1
                encode_out(0x2C, 16), // SPCR
                encode_ldi(17, 0xAA),
                encode_out(0x2E, 17), // SPDR -> Start transfer
                encode_in(18, 0x2D),  // SPSR (check SPIF)
                encode_in(19, 0x2E),  // SPDR (read result and clear SPIF)
                0x0000U
            },
            .entry_word = 0U
        });
        cpu.reset();
        spi.inject_miso_byte(0x55);

        cpu.step(); // LDI
        cpu.step(); // OUT SPCR
        cpu.step(); // LDI
        cpu.step(); // OUT SPDR -> starts transfer
        
        CHECK(spi.busy());
        CHECK(spi.last_transmitted_byte() == 0xAA);

        // Advance 32 cycles (8 bits * 4 cycles/bit)
        for (int i = 0; i < 32; ++i) {
            bus.tick_peripherals(1);
        }
        
        CHECK_FALSE(spi.busy());

        cpu.step(); // IN SPSR
        CHECK((cpu.registers()[18] & 0x80U) != 0U); // SPIF should be set

        cpu.step(); // IN SPDR
        CHECK(cpu.registers()[19] == 0x55U);
        
        // Reading SPSR then SPDR should clear SPIF
        CHECK((spi.read(atmega328.spi.spsr_address) & 0x80U) == 0U);
    }

    SUBCASE("SPI Data Order (DORD)") {
        bus.load_image(HexImage {
            .flash_words = {
                encode_ldi(16, 0x70), // SPE=1, DORD=1, MSTR=1
                encode_out(0x2C, 16), // SPCR
                encode_ldi(17, 0x01), // LSB is 1
                encode_out(0x2E, 17), // SPDR -> Start transfer (should reverse bits if DORD=1)
                0x0000U
            },
            .entry_word = 0U
        });
        cpu.reset();

        cpu.step(); // LDI
        cpu.step(); // OUT SPCR
        cpu.step(); // LDI
        cpu.step(); // OUT SPDR
        
        // If DORD=1, 0x01 becomes 0x80 (MSB is 1)
        CHECK(spi.last_transmitted_byte() == 0x80);
    }

    SUBCASE("SPI Slave Mode") {
        bus.load_image(HexImage {
            .flash_words = {
                encode_ldi(16, 0x40), // SPE=1, MSTR=0 (Slave)
                encode_out(0x2C, 16), // SPCR
                encode_ldi(17, 0xAA),
                encode_out(0x2E, 17), // SPDR -> just buffers, doesn't start
                0x0000U
            },
            .entry_word = 0U
        });
        cpu.reset();

        cpu.step(); // LDI
        cpu.step(); // OUT SPCR
        cpu.step(); // LDI
        cpu.step(); // OUT SPDR
        
        CHECK_FALSE(spi.busy()); // Should not be busy in slave mode just from SPDR write
        
        spi.inject_miso_byte(0x55);
        spi.trigger_slave_transfer(); // Host triggers the clock
        
        CHECK(spi.busy());
        
        // Advance 1 cycle (as implemented in trigger_slave_transfer)
        bus.tick_peripherals(1);
        
        CHECK_FALSE(spi.busy());
        CHECK((spi.read(atmega328.spi.spsr_address) & 0x80U) != 0U); // SPIF should be set
        CHECK(spi.read(atmega328.spi.spdr_address) == 0x55);
    }
}
