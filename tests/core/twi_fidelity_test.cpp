#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/twi.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

using namespace vioavr::core;

TEST_CASE("TWI: Master State Machine and Status Codes") {
    const auto& desc = devices::atmega32u4.twis[0];
    Twi twi {"TWI", desc};
    
    // 1. Bit Rate Timing Check
    // TWBR = 10, Prescaler = 0. Divisor = 16 + 2*10*1 = 36.
    twi.write(desc.twbr_address, 10);
    
    // Start START condition
    twi.write(desc.twcr_address, desc.twen_mask | desc.twsta_mask | desc.twint_mask);
    CHECK(twi.busy());
    
    twi.tick(36);
    CHECK(!twi.busy());
    CHECK(twi.read(desc.twsr_address) == 0x08); // START Transmitted
    
    // 2. SLA+W Transmit
    twi.write(desc.twdr_address, 0x40); // Addr 0x20 + Write (0)
    twi.write(desc.twcr_address, desc.twen_mask | desc.twint_mask);
    
    twi.tick(36);
    CHECK(twi.read(desc.twsr_address) == 0x18); // SLA+W ACK
    
    // 3. Data Transmit
    twi.write(desc.twdr_address, 0xA5);
    twi.write(desc.twcr_address, desc.twen_mask | desc.twint_mask);
    
    twi.tick(36);
    CHECK(twi.read(desc.twsr_address) == 0x28); // Data ACK
    CHECK(twi.tx_buffer().back() == 0xA5);
    
    // 4. STOP
    twi.write(desc.twcr_address, desc.twen_mask | desc.twsto_mask | desc.twint_mask);
    twi.tick(36);
    CHECK(twi.read(desc.twsr_address) == 0xF8); // Bus Idle
}

TEST_CASE("TWI: Slave Address Matching") {
    const auto& desc = devices::atmega32u4.twis[0];
    Twi twi {"TWI", desc};
    
    // Set slave address to 0x4B (0100 1011) -> SLA is 0x96
    // TWAR = Address << 1
    twi.write(desc.twar_address, 0x4B << 1);
    
    CHECK(twi.check_slave_address(0x4B << 1));
    CHECK(!twi.check_slave_address(0x4C << 1));
    
    // Use TWAMR to mask bit 0 of address (ignore LSB)
    // TWAMR: bits 7:1 are mask. LSB is 0.
    twi.write(desc.twamr_address, 0x01 << 1); // Mask bit 1 of SLA
    
    CHECK(twi.check_slave_address(0x4A << 1)); // Should match now
    CHECK(twi.check_slave_address(0x4B << 1));
}
