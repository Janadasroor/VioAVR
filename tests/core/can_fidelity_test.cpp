#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/can.hpp"
#include "vioavr/core/devices/at90can128.hpp"
#include "vioavr/core/logger.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate)
{
    return static_cast<u16>(
        0xE000U |
        ((static_cast<u16>(immediate & 0xF0U)) << 4U) |
        (static_cast<u16>(destination - 16U) << 4U) |
        (immediate & 0x0FU)
    );
}

constexpr u16 encode_sts(const u16 address, const u8 source)
{
    return 0x9200U | ((source & 0x1FU) << 4U); // Simplified for registers
}
}

TEST_CASE("CAN Bus Basic Fidelity")
{
    MemoryBus bus {at90can128};
    CanBus can {"CAN0", at90can128.cans[0]};
    bus.attach_peripheral(can);
    AvrCpu cpu {bus};

    // 1. Enable CAN Controller (CANGCON |= ENASTB)
    bus.write_data(at90can128.cans[0].cangcon_address, 0x02);
    
    // 2. Select MOb 0 (CANPAGE = 0x00)
    bus.write_data(at90can128.cans[0].canpage_address, 0x00);
    
    // 3. Configure MOb 0 as Receiver (CANCDMOB = 0x80 for Rx Enable)
    bus.write_data(at90can128.cans[0].cancdmob_address, 0x80);
    
    CHECK(can.read(at90can128.cans[0].canen2_address) == 0x01); // MOb 0 enabled in CANEN2

    // 4. Inject a message and check RXOK flag
    CanBus::CanMessage msg;
    msg.id = 0x123;
    msg.data = { 0xAA, 0xBB, 0xCC };
    can.inject_message(msg);
    
    bus.write_data(at90can128.cans[0].canpage_address, 0x00);
    u8 status = bus.read_data(at90can128.cans[0].canstmob_address);
    CHECK((status & 0x20) != 0); // RXOK bit should be set
    
    // 5. Check data content
    bus.write_data(at90can128.cans[0].canpage_address, 0x00); // Index 0
    CHECK(bus.read_data(at90can128.cans[0].canmsg_address) == 0xAA);
    // Index should auto-increment if AINC is not set? 
    // Wait, by default AINC is NOT set in our impl?
    // In our impl: if (canpage_ & 0x08) { canpage_ = ... increment ... }
    // AINC is bit 3. 0x00 has bit 3 = 0.
    
    bus.write_data(at90can128.cans[0].canpage_address, 0x08); // Select MOb 0, AINC = 1
    CHECK(bus.read_data(at90can128.cans[0].canmsg_address) == 0xAA);
    CHECK(bus.read_data(at90can128.cans[0].canmsg_address) == 0xBB);
    CHECK(bus.read_data(at90can128.cans[0].canmsg_address) == 0xCC);
}

TEST_CASE("CAN Bus Interrupt Fidelity")
{
    MemoryBus bus {at90can128};
    CanBus can {"CAN0", at90can128.cans[0]};
    bus.attach_peripheral(can);
    AvrCpu cpu {bus};

    // Enable Global Interrupts and CANIT in CAN
    // CANGIE = 0xA0 (ENRX | ENEG)
    bus.write_data(at90can128.cans[0].cangie_address, 0xA0);
    // CANIE2 = 0x01 (Enable MOb 0 interrupt)
    bus.write_data(at90can128.cans[0].canie2_address, 0x01);
    
    // Select MOb 0 as Rx
    bus.write_data(at90can128.cans[0].canpage_address, 0x00);
    bus.write_data(at90can128.cans[0].cancdmob_address, 0x80);
    
    // Enable CANSTB
    bus.write_data(at90can128.cans[0].cangcon_address, 0x02);

    InterruptRequest req;
    CHECK_FALSE(can.pending_interrupt_request(req));

    // Inject message
    CanBus::CanMessage msg;
    msg.id = 0x100;
    msg.data = { 0x55 };
    can.inject_message(msg);
    
    CHECK(can.pending_interrupt_request(req));
    CHECK(req.vector_index == at90can128.cans[0].canit_vector_index);
}
