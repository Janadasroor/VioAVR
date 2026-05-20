#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/usb.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("USB Peripheral Basic Register and FIFO Fidelity")
{
    MemoryBus bus {atmega32u4};
    Usb usb {"USB", atmega32u4.usbs[0]};
    bus.attach_peripheral(usb);
    bus.reset();

    const auto& desc = atmega32u4.usbs[0];

    SUBCASE("Register Access") {
        bus.write_data(desc.usbcon_address, 0x80); // USBE = 1
        CHECK(bus.read_data(desc.usbcon_address) == 0x80);
        
        bus.write_data(desc.udcon_address, 0x01); // DETACH = 1
        CHECK(bus.read_data(desc.udcon_address) == 0x01);
    }

    SUBCASE("Endpoint Selection and FIFO Access") {
        // Select Endpoint 0
        bus.write_data(desc.uenum_address, 0);
        bus.write_data(desc.uedatx_address, 0xAA);
        bus.write_data(desc.uedatx_address, 0xBB);
        
        // Select Endpoint 1
        bus.write_data(desc.uenum_address, 1);
        bus.write_data(desc.uedatx_address, 0xCC);
        
        // Go back to Endpoint 0 and read
        bus.write_data(desc.uenum_address, 0);
        CHECK(bus.read_data(desc.uedatx_address) == 0xAA);
        CHECK(bus.read_data(desc.uedatx_address) == 0xBB);
        
        // Go back to Endpoint 1 and read
        bus.write_data(desc.uenum_address, 1);
        CHECK(bus.read_data(desc.uedatx_address) == 0xCC);
    }

    SUBCASE("Byte Count and FIFO Reset simulation") {
        bus.write_data(desc.uenum_address, 2);
        bus.write_data(desc.uedatx_address, 0x01);
        bus.write_data(desc.uedatx_address, 0x02);
        bus.write_data(desc.uedatx_address, 0x03);
        
        CHECK(bus.read_data(desc.uebclx_address) == 3);
        
        // Simulated Reset (in real hardware this would be UERST)
        usb.reset();
        bus.write_data(desc.uenum_address, 2);
        CHECK(bus.read_data(desc.uebclx_address) == 0);
    }
}
