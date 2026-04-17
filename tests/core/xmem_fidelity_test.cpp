#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/xmem.hpp"
#include "vioavr/core/devices/atmega2560.hpp"

using namespace vioavr::core;

TEST_CASE("XMEM: Wait State and Sector Partitioning") {
    MemoryBus bus {devices::atmega2560};
    AvrCpu cpu {bus};
    Xmem xmem {devices::atmega2560, cpu.control()};
    
    // Setup and attach
    bus.attach_peripheral(xmem);
    bus.set_xmem(&xmem);
    
    const auto& desc = devices::atmega2560;
    
    // Enable XMEM
    // SRE = 0x80
    // SRL = 2 (bits 6:4 = 0x20). Boundary = 0x2000 * 2 - 1 = 0x3FFF.
    // SRW0 = 1 (1 wait state for LS, bits 1:0 = 0x01)
    // SRW1 = 2 (2 wait states for US, bits 3:2 = 0x08)
    u8 xmcra_val = 0x80 | 0x20 | 0x01 | 0x08;
    bus.write_data(desc.xmem.xmcra_address, xmcra_val);
    
    // Internal SRAM ends at 0x21FF.
    // Ext SRAM starts at 0x2200.
    
    // LS: 0x2200 - 0x3FFF. Try 0x3000.
    CHECK(bus.get_wait_states(0x3000) == 1);
    
    // US: 0x4000 - 0xFFFF. Try 0x5000.
    CHECK(bus.get_wait_states(0x5000) == 2);
    
    // Internal SRAM. Try 0x2000.
    CHECK(bus.get_wait_states(0x2000) == 0);
    
    // 3. Verify R/W to external memory
    bus.write_data(0x5000, 0xA5);
    CHECK(bus.read_data(0x5000) == 0xA5);
    
    // Disable XMEM
    bus.write_data(desc.xmem.xmcra_address, 0x00);
    CHECK(bus.get_wait_states(0x3000) == 0);
    // Should return 0 (disconnected)
    CHECK(bus.read_data(0x5000) == 0);
}

TEST_CASE("XMEM: Address Masking (XMM)") {
    MemoryBus bus {devices::atmega2560};
    AvrCpu cpu {bus};
    Xmem xmem {devices::atmega2560, cpu.control()};
    bus.attach_peripheral(xmem);
    bus.set_xmem(&xmem);
    
    // Enable XMEM with SRE=0x80
    bus.write_data(devices::atmega2560.xmem.xmcra_address, 0x80);
    
    // Set XMM = 1 (release PC7 -> A15 masked)
    // XMCRB = 0x01
    bus.write_data(devices::atmega2560.xmem.xmcrb_address, 0x01);
    
    // Write to a location with A15=0 and A15=1
    // Both should hit masked_address (original & 0x7FFF)
    bus.write_data(0x3000, 0x55);
    CHECK(bus.read_data(0x3000) == 0x55);
    
    // Reading 0xB000 (0x3000 | 0x8000) should return the same value if masked
    CHECK(bus.read_data(0xB000) == 0x55);
    
    // Change XMM to 7 (release PC7..1 -> top 7 bits masked)
    // 0xFFFF >> 7 = 0x01FF.
    bus.write_data(devices::atmega2560.xmem.xmcrb_address, 0x07);
    
    bus.write_data(0x2345, 0xAA);
    // Masked to 0x2345 & 0x01FF = 0x0145.
    // Wait! 0x2345 & 0x01FF = 0x0145.
    // So writing to 0x2345 should be same as writing to 0x2145?
    // Internal SRAM ends at 0x21FF on 2560.
    // So 0x2345 is external.
    CHECK(bus.read_data(0x2345) == 0xAA);
    CHECK(bus.read_data(0x2545) == 0xAA); // Masked to same
}

TEST_CASE("XMEM: Banking (XMBK)") {
    MemoryBus bus {devices::atmega2560};
    AvrCpu cpu {bus};
    Xmem xmem {devices::atmega2560, cpu.control()};
    bus.attach_peripheral(xmem);
    bus.set_xmem(&xmem);
    
    // Enable XMEM
    bus.write_data(devices::atmega2560.xmem.xmcra_address, 0x80);
    
    // Bank 0 (XMBK=0)
    bus.write_data(0x4000, 0x11);
    CHECK(bus.read_data(0x4000) == 0x11);
    
    // Switch to Bank 1 (XMBK=0x80)
    bus.write_data(devices::atmega2560.xmem.xmcrb_address, 0x80);
    
    // Should be different memory
    CHECK(bus.read_data(0x4000) == 0x00);
    bus.write_data(0x4000, 0x22);
    CHECK(bus.read_data(0x4000) == 0x22);
    
    // Switch back to Bank 0
    bus.write_data(devices::atmega2560.xmem.xmcrb_address, 0x00);
    CHECK(bus.read_data(0x4000) == 0x11);
}
