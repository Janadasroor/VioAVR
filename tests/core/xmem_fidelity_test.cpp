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
