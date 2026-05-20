#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/can.hpp"
#include "vioavr/core/device.hpp"

namespace {
using namespace vioavr::core;

CanDescriptor test_can_desc {
    .cangcon_address = 0xD8,
    .cangsta_address = 0xD9,
    .cangit_address = 0xDA,
    .cangie_address = 0xDB,
    .canen1_address = 0xDD,
    .canen2_address = 0xDC,
    .canie1_address = 0xDF,
    .canie2_address = 0xDE,
    .cansit1_address = 0xE1,
    .cansit2_address = 0xE0,
    .canbt1_address = 0xE2,
    .canbt2_address = 0xE3,
    .canbt3_address = 0xE4,
    .cantcon_address = 0xE5,
    .cantim_address = 0xE6,
    .canttc_address = 0xE8,
    .cantec_address = 0xEA,
    .canrec_address = 0xEB,
    .canhpmob_address = 0xEC,
    .canpage_address = 0xED,
    .canstmob_address = 0xEE,
    .cancdmob_address = 0xEF,
    .canidt_address = 0xF0,
    .canidm_address = 0xF4,
    .canstm_address = 0xF8,
    .canmsg_address = 0xFA,
    .canit_vector_index = 30,
    .mob_count = 15
};
}

TEST_CASE("CAN Stability & Error States")
{
    CanBus can {"CAN", test_can_desc};
    can.reset();

    SUBCASE("Delayed Transmission") {
        can.write(0xD8, 0x02); // ENA=1
        
        // 1. Setup MOb 0 for Transmit
        can.write(0xED, 0x00); // CANPAGE = MOb 0
        can.write(0xEF, 0x48); // CONMOB=01 (Tx), DLC=8
        
        // 2. Check if immediate TXOK is NOT set
        CHECK(((can.read(0xEE) & 0x40) == 0)); // TXOK=0
        
        // 3. Tick 500 cycles
        can.tick(500);
        CHECK(((can.read(0xEE) & 0x40) == 0)); // Still 0
        
        // 4. Tick another 501 cycles
        can.tick(501);
        CHECK((can.read(0xEE) & 0x40) != 0); // TXOK=1!
    }

    SUBCASE("Error State Transitions") {
        can.write(0xD8, 0x02); // ENA=1
        
        // Initial: Error Active
        CHECK((can.read(0xD9) & 0x02) != 0); // ERRA=1
        
        // Simulate errors until TEC >= 128
        for (int i = 0; i < 16; ++i) can.simulate_bus_error(); // 8 * 16 = 128
        
        CHECK((can.read(0xD9) & 0x04) != 0); // ERRP=1
        CHECK(((can.read(0xD9) & 0x02) == 0)); // ERRA=0
        
        // Simulate more errors until TEC > 255
        for (int i = 0; i < 17; ++i) can.simulate_bus_error(); // 128 + 8*17 = 264
        
        CHECK((can.read(0xD9) & 0x08) != 0); // BOFF=1
        CHECK(((can.read(0xD8) & 0x02) == 0)); // ENA=0 (disabled on bus off)
    }

    SUBCASE("Priority Arbitration (CANHPMOB)") {
        can.write(0xD8, 0x02); // ENA=1
        
        // 1. Setup MOb 5 with ID 0x100
        can.write(0xED, 0x50);
        can.write(0xF3, 0x20); // IDT1 = 0x20 -> ID = 0x100 (standard)
        can.write(0xEF, 0x40); // Transmit
        
        // 2. Setup MOb 2 with ID 0x050 (Higher priority)
        can.write(0xED, 0x20);
        can.write(0xF3, 0x0A); // IDT1 = 0x0A -> ID = 0x050
        can.write(0xEF, 0x40); // Transmit
        
        // 3. Check CANHPMOB
        // It should point to MOb 2 (index 2)
        u8 hpmob = can.read(0xEC);
        CHECK((hpmob >> 4) == 2);
    }
}
