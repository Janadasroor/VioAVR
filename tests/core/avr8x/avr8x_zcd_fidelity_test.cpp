#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/zcd.hpp"
#include "vioavr/core/memory_bus.hpp"

using namespace vioavr::core;

TEST_CASE("ZCD Peripheral Fidelity") {
    DeviceDescriptor desc{};
    desc.zcd_count = 1;
    desc.zcds[0] = ZcdDescriptor{
        .ctrla_address = 0x0080,
        .vector_index  = 5,
        .pin_address   = 0x1234,
        .pin_bit       = 0,
    };
    
    Machine machine(desc);
    MemoryBus& bus = machine.bus();
    
    // 1. Enable ZCD
    bus.write_data(0x0080, 0x01);
    
    // 2. Initial state
    CHECK((bus.read_data(0x0080) & 0x80) == 0); // STATE should be low initially
    
    // 3. Simulate pin change to High
    bus.propagate_external_pin_change(0x1234, 0, PinLevel::high);
    
    // 4. Verify STATE bit
    CHECK((bus.read_data(0x0080) & 0x80) != 0); // STATE should be high
    
    // 5. Check interrupt pending
    InterruptRequest req{};
    CHECK(machine.bus().pending_interrupt_request(req, 0xFF));
    CHECK(req.vector_index == 5);
    
    // 6. Consume interrupt
    CHECK(machine.bus().consume_interrupt_request(req));
    CHECK(!machine.bus().pending_interrupt_request(req, 0xFF));
    
    // 7. Simulate pin change to Low
    bus.propagate_external_pin_change(0x1234, 0, PinLevel::low);
    CHECK((bus.read_data(0x0080) & 0x80) == 0);
    
    // 8. Verify second crossing triggers interrupt
    CHECK(machine.bus().pending_interrupt_request(req, 0xFF));
}
