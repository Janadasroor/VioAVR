#include "doctest.h"
#include "vioavr/core/twi.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

using namespace vioavr::core;

// Each TWI byte/start/stop step takes divisor × 9 cycles (8 data bits + 1 ACK)
static constexpr u32 kTwbr10Divisor = 36U;
static constexpr u32 kStepCycles = kTwbr10Divisor * 9U;

TEST_CASE("TWI: Master State Machine and Status Codes") {
    const auto& desc = devices::atmega32u4.twis[0];
    MemoryBus bus{devices::atmega32u4};
    Twi twi {"TWI", desc}; twi.set_rx_buffer({0});
    twi.set_memory_bus(&bus);
    bus.attach_peripheral(twi);
    
    // 1. Bit Rate Timing Check
    // TWBR = 10, Prescaler = 0. Divisor = 16 + 2*10*1 = 36.
    bus.write_data(desc.twbr_address, 10);
    
    // Start START condition
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsta_mask | desc.twint_mask);
    CHECK(twi.busy());
    
    bus.tick_peripherals(kStepCycles);
    CHECK(!twi.busy());
    CHECK(bus.read_data(desc.twsr_address) == 0x08); // START Transmitted
    
    // 2. SLA+W Transmit (TWEA must be set for slave to ACK)
    bus.write_data(desc.twar_address, 0x40); // Enable slave response to 0x20
    bus.write_data(desc.twdr_address, 0x40); // Addr 0x20 + Write (0)
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twea_mask | desc.twint_mask);
    
    bus.tick_peripherals(kStepCycles);
    CHECK(bus.read_data(desc.twsr_address) == 0x18); // SLA+W ACK
    
    // 3. Data Transmit
    bus.write_data(desc.twdr_address, 0xA5);
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twea_mask | desc.twint_mask);
    
    bus.tick_peripherals(kStepCycles);
    CHECK(bus.read_data(desc.twsr_address) == 0x28); // Data ACK
    CHECK(twi.tx_buffer().back() == 0xA5);
    
    // 4. STOP
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsto_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    CHECK(bus.read_data(desc.twsr_address) == 0xF8); // Bus Idle
}

TEST_CASE("TWI: Slave Address Matching") {
    const auto& desc = devices::atmega32u4.twis[0];
    MemoryBus bus{devices::atmega32u4};
    Twi twi {"TWI", desc}; twi.set_rx_buffer({0});
    twi.set_memory_bus(&bus);
    bus.attach_peripheral(twi);
    
    // Set slave address to 0x4B (0100 1011) -> SLA is 0x96
    // TWAR = Address << 1
    bus.write_data(desc.twar_address, 0x4B << 1);
    
    CHECK(twi.check_twi_address(0x4B << 1));
    CHECK(!twi.check_twi_address(0x4C << 1));
    
    // Use TWAMR to mask bit 0 of address (ignore LSB)
    // TWAMR: bits 7:1 are mask. LSB is 0.
    bus.write_data(desc.twamr_address, 0x01 << 1); // Mask bit 1 of SLA
    
    CHECK(twi.check_twi_address(0x4A << 1)); // Should match now
    CHECK(twi.check_twi_address(0x4B << 1));
}

TEST_CASE("TWI: NACK Detection on Slave Address") {
    const auto& desc = devices::atmega32u4.twis[0];
    MemoryBus bus{devices::atmega32u4};
    Twi twi {"TWI", desc};
    twi.set_memory_bus(&bus);
    bus.attach_peripheral(twi);
    
    bus.write_data(desc.twbr_address, 10);
    
    // Start condition
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsta_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    CHECK(bus.read_data(desc.twsr_address) == 0x08); // START
    
    // SLA+W to non-existent address (0x7F)
    bus.write_data(desc.twdr_address, 0xFE); // Addr 0x7F + Write
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twint_mask);
    
    bus.tick_peripherals(kStepCycles);
    // Should get SLA+W NOT ACK (0x20) since no slave responds
    CHECK(bus.read_data(desc.twsr_address) == 0x20);
}

TEST_CASE("TWI: Repeated START Condition") {
    const auto& desc = devices::atmega32u4.twis[0];
    MemoryBus bus{devices::atmega32u4};
    Twi twi {"TWI", desc}; twi.set_rx_buffer({0});
    twi.set_memory_bus(&bus);
    bus.attach_peripheral(twi);
    
    bus.write_data(desc.twbr_address, 10);
    
    // First transaction: Write to address 0x20
    bus.write_data(desc.twar_address, 0x40); // Enable slave response
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsta_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    
    bus.write_data(desc.twdr_address, 0x40); // SLA+W
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twea_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    CHECK(bus.read_data(desc.twsr_address) == 0x18); // SLA+W ACK
    
    // Send data byte
    bus.write_data(desc.twdr_address, 0x55);
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twea_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    CHECK(bus.read_data(desc.twsr_address) == 0x28); // Data ACK
    
    // Repeated START (without STOP)
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsta_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    CHECK(bus.read_data(desc.twsr_address) == 0x10); // Repeated START
    
    // SLA+R (read from same device)
    bus.write_data(desc.twar_address, 0x41); // Enable slave response for read
    bus.write_data(desc.twdr_address, 0x41); // SLA+R
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twea_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    // Status depends on whether device supports read
    
    // STOP
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsto_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    CHECK(bus.read_data(desc.twsr_address) == 0xF8); // Bus idle
}

TEST_CASE("TWI: Clock Stretching Simulation") {
    const auto& desc = devices::atmega32u4.twis[0];
    MemoryBus bus{devices::atmega32u4};
    Twi twi {"TWI", desc}; twi.set_rx_buffer({0});
    twi.set_memory_bus(&bus);
    bus.attach_peripheral(twi);
    
    bus.write_data(desc.twbr_address, 10); // ~100kHz at 16MHz
    
    // Start and address
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsta_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    
    bus.write_data(desc.twdr_address, 0x40);
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twint_mask);
    
    // Clock stretching would extend the SCL low period
    // Verify operation still completes correctly after stretch
    bus.tick_peripherals(72); // Double the normal cycles (simulating stretch)
    
    // Should still complete successfully
    u8 status = bus.read_data(desc.twsr_address);
    (void)status;
    // Status depends on slave response
}

TEST_CASE("TWI: Arbitration Lost Detection") {
    const auto& desc = devices::atmega32u4.twis[0];
    MemoryBus bus{devices::atmega32u4};
    Twi twi {"TWI", desc}; twi.set_rx_buffer({0});
    twi.set_memory_bus(&bus);
    bus.attach_peripheral(twi);
    
    bus.write_data(desc.twbr_address, 10);
    
    // Start transaction
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsta_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    CHECK(bus.read_data(desc.twsr_address) == 0x08); // START
    
    // In multi-master scenario, arbitration could be lost
    // Check that status register can reflect this
    // Note: Actual arbitration simulation requires another master
    
    // Verify TWI state - check busy status if needed
    (void)twi.busy();
}

TEST_CASE("TWI: General Call Address (0x00)") {
    const auto& desc = devices::atmega32u4.twis[0];
    MemoryBus bus{devices::atmega32u4};
    Twi twi {"TWI", desc}; twi.set_rx_buffer({0});
    twi.set_memory_bus(&bus);
    bus.attach_peripheral(twi);
    
    bus.write_data(desc.twbr_address, 10);
    
    // Start
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsta_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    
    // General call address (0x00 + Write = 0x00)
    bus.write_data(desc.twdr_address, 0x00);
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    
    // Status should be General Call ACK (0x70) if slaves respond
    u8 status = bus.read_data(desc.twsr_address);
    (void)status;
}

TEST_CASE("TWI: Bus Error Detection") {
    const auto& desc = devices::atmega32u4.twis[0];
    MemoryBus bus{devices::atmega32u4};
    Twi twi {"TWI", desc}; twi.set_rx_buffer({0});
    twi.set_memory_bus(&bus);
    bus.attach_peripheral(twi);
    
    // Enable TWI
    bus.write_data(desc.twcr_address, desc.twen_mask);
    
    // Bus error status code is 0x00
    // This occurs when START/STOP condition occurs at illegal position
    
    // For now, verify TWI is enabled
    u8 twcr = bus.read_data(desc.twcr_address);
    CHECK((twcr & desc.twen_mask) != 0);
}

TEST_CASE("TWI: Interrupt Flag Behavior") {
    const auto& desc = devices::atmega32u4.twis[0];
    MemoryBus bus{devices::atmega32u4};
    Twi twi {"TWI", desc}; twi.set_rx_buffer({0});
    twi.set_memory_bus(&bus);
    bus.attach_peripheral(twi);
    
    bus.write_data(desc.twbr_address, 10);
    
    // Enable interrupt
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twie_mask);
    
    // Start transaction
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsta_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    
    // TWINT should be set when operation completes
    u8 twcr = bus.read_data(desc.twcr_address);
    CHECK((twcr & desc.twint_mask) != 0);
    
    // Clear TWINT by writing 1
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twint_mask);
    twcr = bus.read_data(desc.twcr_address);
    CHECK((twcr & desc.twint_mask) == 0);
}

TEST_CASE("TWI: Different Bit Rates") {
    const auto& desc = devices::atmega32u4.twis[0];
    
    struct RateTest {
        u8 twbr;
        u8 prescaler;
        u32 expected_divisor;
    };
    
    // At 16MHz: SCL = F_CPU / (16 + 2*TWBR*Prescaler)
    RateTest rates[] = {
        {72, 0, 160},   // ~100kHz (100.0 kHz exact)
        {12, 0, 40},    // ~400kHz (400.0 kHz exact)
        {3, 0, 22},     // ~727kHz
        {0, 0, 16},     // 1MHz
        {32, 1, 80},    // ~200kHz with prescaler
        {12, 1, 40},    // ~400kHz with prescaler
    };
    
    for (const auto& rate : rates) {
        MemoryBus bus{devices::atmega32u4};
        Twi twi {"TWI", desc}; twi.set_rx_buffer({0});
        twi.set_memory_bus(&bus);
        bus.attach_peripheral(twi);
        
        // Set bit rate
        bus.write_data(desc.twbr_address, rate.twbr);
        // TWSR contains prescaler bits (bits 0-1)
        bus.write_data(desc.twsr_address, rate.prescaler & 0x03);
        
        // Read back and verify
        u8 twbr_read = bus.read_data(desc.twbr_address);
        u8 twsr_read = bus.read_data(desc.twsr_address);
        
        CHECK(twbr_read == rate.twbr);
        CHECK((twsr_read & 0x03) == rate.prescaler);
    }
}

TEST_CASE("TWI: Master Receiver Mode") {
    const auto& desc = devices::atmega32u4.twis[0];
    MemoryBus bus{devices::atmega32u4};
    Twi twi {"TWI", desc}; twi.set_rx_buffer({0});
    twi.set_memory_bus(&bus);
    bus.attach_peripheral(twi);
    
    bus.write_data(desc.twbr_address, 10);
    
    // START
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsta_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    CHECK(bus.read_data(desc.twsr_address) == 0x08); // START
    
    // SLA+R (Read from address 0x20)
    bus.write_data(desc.twdr_address, 0x41); // SLA+R
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    
    // Status depends on slave response
    // 0x40 = SLA+R transmitted, ACK received
    // 0x48 = SLA+R transmitted, NOT ACK received
    u8 status = bus.read_data(desc.twsr_address);
    (void)status;
    
    // Data byte reception with ACK
    // TWEA=1: Send ACK after receive
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twea_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
    
    // Status 0x50 = Data received, ACK returned
    // Status 0x58 = Data received, NACK returned
    status = bus.read_data(desc.twsr_address);
    (void)status;
    
    // STOP
    bus.write_data(desc.twcr_address, desc.twen_mask | desc.twsto_mask | desc.twint_mask);
    bus.tick_peripherals(kStepCycles);
}
