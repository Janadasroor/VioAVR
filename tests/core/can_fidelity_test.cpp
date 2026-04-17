#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/can.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/at90can128.hpp"

using namespace vioavr::core;

TEST_CASE("CAN: Basic MOb Page Access and RX Simulation") {
    // We use CanBus class directly to test registers first
    CanBus can {"CAN", devices::at90can128.cans[0]};
    const auto& desc = devices::at90can128.cans[0];
    
    can.reset();
    
    // 1. Select MOb 0
    can.write(desc.canpage_address, 0x00);
    
    // 2. Fill MOB 0 sequence
    // Write 4 bytes to CANMSG
    can.write(desc.canmsg_address, 0x11);
    can.write(desc.canmsg_address, 0x22);
    can.write(desc.canmsg_address, 0x33);
    can.write(desc.canmsg_address, 0x44);
    
    // Verify Z-pointer auto-increment
    CHECK(can.read(desc.canpage_address) == 0x04);
    
    // 3. Set to RX mode
    // CONMOB = 10, DLC = 4
    can.write(desc.cancdmob_address, (0x02 << 6) | 0x04);
    
    // 4. Inject message
    CanBus::CanMessage msg;
    msg.id = 0x123;
    msg.data = {0xDE, 0xAD, 0xBE, 0xEF};
    can.inject_message(msg);
    
    // 5. Verify RXOK and Data
    can.write(desc.canpage_address, 0x00); // Reset index for MOB 0
    CHECK((can.read(desc.canstmob_address) & 0x20) != 0); // RXOK
    
    CHECK(can.read(desc.canmsg_address) == 0xDE);
    CHECK(can.read(desc.canmsg_address) == 0xAD);
}
