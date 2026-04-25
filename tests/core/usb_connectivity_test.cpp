#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/usb.hpp"

namespace {

using namespace vioavr::core;

TEST_CASE("USB Connectivity Test") {
    auto machine = Machine::create_for_device("atmega32u4");
    REQUIRE(machine != nullptr);

    auto& bus = machine->bus();
    const auto& device = bus.device();
    
    // Check if USB descriptor is present in the device
    REQUIRE(device.usb_count > 0);
    const auto& usb_desc = device.usbs[0];
    REQUIRE(usb_desc.usbcon_address != 0);

    // EXPECTATION: This will FAIL if it's just memory because 0x80 written will read as 0x80.
    // BUT Usb::write for USBCON has no special masking for USBE yet? 
    // Actually, Usb::write for UDINT has clear-on-write-zero.
    
    const auto udfnum_addr = usb_desc.udfnum_address;
    
    // Enable USB and simulate PLL lock
    bus.write_data(usb_desc.usbcon_address, 0x80); // USBE = 1
    if (usb_desc.pllcsr_address != 0) {
        bus.write_data(usb_desc.pllcsr_address, 0x01); // PLOCK = 1
    }

    u16 frame0 = bus.read_data(udfnum_addr) | (bus.read_data(udfnum_addr + 1) << 8);
    
    // Manually tick peripherals on the bus
    bus.tick_peripherals(16000, 0xFF); // All domains
    
    u16 frame1 = bus.read_data(udfnum_addr) | (bus.read_data(udfnum_addr + 1) << 8);
    
    // If it's the Usb peripheral, frame1 should be frame0 + 1.
    // If it's just memory, frame1 will be frame0 (0).
    CHECK(frame1 == frame0 + 1);
}

}
