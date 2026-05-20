#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/lin.hpp"
#include "vioavr/core/device.hpp"

namespace {
using namespace vioavr::core;

LinDescriptor test_lin_desc {
    .ctrla_address = 0xC8,
    .vector_index = 20,
    .lincr_lswres_mask = 0x80,
    .lincr_lin13_mask = 0x40,
    .lincr_lconf_mask = 0x30,
    .lincr_lena_mask = 0x08,
    .lincr_lcmd_mask = 0x07,
    .linsir_lidst_mask = 0xE0,
    .linsir_lbusy_mask = 0x10,
    .linsir_lerr_mask = 0x08,
    .linsir_lidok_mask = 0x04,
    .linsir_ltxok_mask = 0x02,
    .linsir_lrxok_mask = 0x01
};
}

TEST_CASE("LIN Protocol Fidelity")
{
    LinUART lin {test_lin_desc};
    lin.reset();

    SUBCASE("Receive Frame") {
        // 1. Enable LIN and set to Receive (LCMD=2, LENA=1)
        lin.write(0xC8, 0x0A); 
        
        // 2. Simulate Break
        lin.simulate_rx_break();
        
        // 3. Simulate Sync Field (0x55)
        lin.simulate_rx_byte(0x55);
        
        // 4. Simulate Identifier (e.g. 0x10, length 2 bytes for LIN 2.x)
        lin.simulate_rx_byte(0x10);
        
        // Check LIDOK
        CHECK((lin.read(0xC9) & 0x04) != 0); // LIDOK=1
        
        // 5. Simulate Data 0
        lin.simulate_rx_byte(0xAA);
        CHECK(lin.read(0xCF) == 0xAA); // LINDAT
        
        // 6. Simulate Data 1
        lin.simulate_rx_byte(0x55);
        CHECK(lin.read(0xCF) == 0x55); // LINDAT
        
        // 7. Simulate Checksum
        // Sum = 0xAA + 0x55 = 0xFF. Checksum = ~0xFF = 0x00
        lin.simulate_rx_byte(0x00);
        
        // Check LRXOK
        CHECK((lin.read(0xC9) & 0x01) != 0); // LRXOK=1
        CHECK(!(lin.read(0xC9) & 0x10)); // LBUSY=0
    }

    SUBCASE("Checksum Error") {
        lin.write(0xC8, 0x0A);
        lin.simulate_rx_break();
        lin.simulate_rx_byte(0x55);
        lin.simulate_rx_byte(0x10); // 2 bytes
        lin.simulate_rx_byte(0xAA);
        lin.simulate_rx_byte(0x55);
        lin.simulate_rx_byte(0xFF); // Wrong checksum
        
        CHECK((lin.read(0xC9) & 0x08) != 0); // LERR=1
        CHECK((lin.read(0xCB) & 0x08) != 0); // LINERR Checksum error bit (bit 3)
    }
}
