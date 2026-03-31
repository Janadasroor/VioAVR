#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/twi.hpp"
#include "vioavr/core/devices/atmega328.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_sts(const u8 source) {
    return static_cast<u16>(0x9200U | (static_cast<u16>(source) << 4U));
}

} // namespace

TEST_CASE("TWI (I2C) Peripheral Master Mode Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    MemoryBus bus {atmega328};
    Twi twi {"TWI", atmega328};
    bus.attach_peripheral(twi);
    AvrCpu cpu {bus};

    // TWI addresses on ATmega328:
    // TWCR = 0xBC (extended IO)
    // TWSR = 0xB9 (extended IO)
    // TWDR = 0xBB (extended IO)

    bus.load_image(HexImage {
        .flash_words = {
            // Send START
            encode_ldi(16, 0xA4), // TWINT=1, TWSTA=1, TWEN=1
            encode_sts(16), atmega328.twi.twcr_address,
            
            // Send SLA+W (address 0x50)
            encode_ldi(17, 0xA0), // 0x50 << 1
            encode_sts(17), atmega328.twi.twdr_address,
            encode_ldi(18, 0x84), // TWINT=1, TWEN=1
            encode_sts(18), atmega328.twi.twcr_address,

            // Send DATA 0xDE
            encode_ldi(19, 0xDE),
            encode_sts(19), atmega328.twi.twdr_address,
            encode_sts(18), atmega328.twi.twcr_address,

            // Send STOP
            encode_ldi(20, 0x94), // TWINT=1, TWSTO=1, TWEN=1
            encode_sts(20), atmega328.twi.twcr_address,

            0x0000U
        },
        .entry_word = 0U
    });
    cpu.reset();

    SUBCASE("Master Transmission Sequence") {
        // 1. Send START
        cpu.step(); // LDI
        cpu.step(); // STS TWCR
        CHECK(twi.busy());
        bus.tick_peripherals(200); // Increased from 100
        CHECK_FALSE(twi.busy());
        CHECK((twi.read(atmega328.twi.twsr_address) & 0xF8U) == 0x08U); // START status

        // 2. Send SLA+W
        cpu.step(); // LDI
        cpu.step(); // STS TWDR
        cpu.step(); // LDI
        cpu.step(); // STS TWCR
        bus.tick_peripherals(200);
        CHECK((twi.read(atmega328.twi.twsr_address) & 0xF8U) == 0x18U); // SLA+W ACK status

        // 3. Send DATA
        cpu.step(); // LDI
        cpu.step(); // STS TWDR
        cpu.step(); // STS TWCR
        bus.tick_peripherals(200);
        CHECK((twi.read(atmega328.twi.twsr_address) & 0xF8U) == 0x28U); // Data ACK status
        REQUIRE(twi.tx_buffer().size() == 1);
        CHECK(twi.tx_buffer()[0] == 0xDE);

        // 4. Send STOP
        cpu.step(); // LDI
        cpu.step(); // STS TWCR
        bus.tick_peripherals(200);
        // Status 0xF8 is the 'No relevant state information' which essentially means idle/STOP.
        CHECK((twi.read(atmega328.twi.twsr_address) & 0xF8U) == 0xF8U);
    }
}
