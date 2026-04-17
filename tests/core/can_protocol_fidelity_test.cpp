#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/can.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/at90can128.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("CAN Protocol Fidelity - Standard ID Filtering")
{
    CanBus can {"CAN", at90can128.cans[0]};
    const auto& desc = at90can128.cans[0];
    can.reset();
    
    // Enable CAN
    can.write(desc.cangcon_address, 0x02); // ENA=1
    
    // Configure MOb 0 for Standard RX
    can.write(desc.canpage_address, 0x00);
    // ID = 0x123. Part1(10:3)=0x24, Part2(2:0)=0x03
    can.write(desc.canidt_address + 3, 0x24); // CANIDT1: ID 10:3
    can.write(desc.canidt_address + 2, 0x03 << 5); // CANIDT2: ID 2:0
    // Mask: require exact match
    can.write(desc.canidm_address + 3, 0xFF);
    can.write(desc.canidm_address + 2, 0xFF);
    
    can.write(desc.cancdmob_address, (0x02 << 6) | 0x04); // RX, DLC=4, IDE=0

    // Inject matching message
    CanBus::CanMessage msg;
    msg.id = 0x123; // 11-bit ID
    msg.ide = false;
    msg.data = {0x11, 0x22, 0x33, 0x44};
    can.inject_message(msg);
    
    // Check RXOK
    CHECK((can.read(desc.canstmob_address) & 0x20) != 0);
    // Verify data
    can.write(desc.canpage_address, 0x00);
    CHECK(can.read(desc.canmsg_address) == 0x11);
    
    // Verify CONMOB is cleared after reception
    CHECK((can.read(desc.cancdmob_address) >> 6) == 0);
}

TEST_CASE("CAN Protocol Fidelity - Extended ID Filtering with Mask")
{
    CanBus can {"CAN", at90can128.cans[0]};
    const auto& desc = at90can128.cans[0];
    can.reset();
    can.write(desc.cangcon_address, 0x02);
    
    // Configure MOb 1 for Extended RX
    can.write(desc.canpage_address, 0x10);
    // Ext ID: 0x12345678
    // Tags: IDT1(28:21), IDT2(20:13), IDT3(12:5), IDT4(4:0)
    // Ext ID = 0x12345678. 
    // ID28:21 = 0x91. ID20:13 = 0xA2. ID12:5 = 0xB3. ID4:0 = 0x18.
    u32 ext_id = 0x12345678;
    can.write(desc.canidt_address + 3, (ext_id >> 21) & 0xFF);
    can.write(desc.canidt_address + 2, (ext_id >> 13) & 0xFF);
    can.write(desc.canidt_address + 1, (ext_id >> 5) & 0xFF);
    can.write(desc.canidt_address + 0, (ext_id << 3) & 0xFF);
    
    // Mask: Ignore low byte (ID7:0)
    can.write(desc.canidm_address + 3, 0xFF);
    can.write(desc.canidm_address + 2, 0xFF);
    can.write(desc.canidm_address + 1, 0x00);
    can.write(desc.canidm_address + 0, 0x00);
    
    can.write(desc.cancdmob_address, (0x02 << 6) | 0x10 | 0x08); // RX, IDE=1, DLC=8

    // Inject partially matching message
    CanBus::CanMessage msg;
    msg.id = 0x12345600; // Matches high bits
    msg.ide = true;
    msg.data = {0xAA, 0xBB};
    can.inject_message(msg);
    
    CHECK((can.read(desc.canstmob_address) & 0x20) != 0); // RXOK
}

TEST_CASE("CAN Protocol Fidelity - Interrupt Logic & Status Clearing")
{
    CanBus can {"CAN", at90can128.cans[0]};
    const auto& desc = at90can128.cans[0];
    can.reset();
    can.write(desc.cangcon_address, 0x02);
    can.write(desc.cangie_address, 0x01); // ENIT=1
    can.write(desc.canie2_address, 0x01); // MOb 0 interrupt enable
    
    // Trigger TXOK
    can.write(desc.canpage_address, 0x00);
    can.write(desc.cancdmob_address, (0x01 << 6)); // TX
    
    // Check interrupt
    InterruptRequest req;
    CHECK(can.pending_interrupt_request(req) == true);
    CHECK(req.vector_index == desc.canit_vector_index);
    
    // Clear TXOK by writing 0 to bit 6 (standard CAN) or bit-masking in AVR?
    // In AT90CAN, you clear flags by writing 0 to that specific bit.
    can.write(desc.canstmob_address, 0xBF); // Clear bit 6
    
    CHECK(can.pending_interrupt_request(req) == false);
}

TEST_CASE("CAN Protocol Fidelity - Timer Timing")
{
    CanBus can {"CAN", at90can128.cans[0]};
    const auto& desc = at90can128.cans[0];
    can.reset();
    can.write(desc.cangcon_address, 0x02);
    
    // Set BRP = 0 -> TQ = 1 cycle
    can.write(desc.canbt1_address, 0x00);
    
    // Quick tick reach
    for(int i=0; i<65535; ++i) { can.tick(1); }
    CHECK(can.read(desc.cantim_address) == (0xFFFF & 0xFF));
    
    can.tick(1); // Overflow!
    CHECK(can.read(desc.cantim_address) == 0); // Overflow
    CHECK((can.read(desc.cangit_address) & 0x01) != 0); // OVRIT
}
