#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/usb.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"
#include "usb_host_sim.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
using namespace vioavr::core::testing;
}

TEST_CASE("USB Enumeration Fidelity - SETUP Handling")
{
    Usb usb {"USB", atmega32u4.usbs[0]};
    MemoryBus bus {atmega32u4};
    usb.set_memory_bus(&bus);
    bus.attach_peripheral(usb);
    UsbHostSim host {usb};

    usb.reset();
    host.set_vbus(true);
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0x80); // USBE = 1

    // 1. Host sends SETUP GET_DESCRIPTOR (Device)
    host.send_setup(0x80, 0x06, 0x0100, 0, 64);

    // 2. Select EP0
    bus.write_data(atmega32u4.usbs[0].uenum_address, 0);

    // 3. Verify Firmware sees SETUP flag
    u8 ueintx = bus.read_data(atmega32u4.usbs[0].ueintx_address);
    CHECK((ueintx & 0x08) != 0); // RXSTPI set

    // 4. Verify Firmware reads the correct 8 bytes from FIFO
    CHECK(bus.read_data(atmega32u4.usbs[0].uedatx_address) == 0x80);
    CHECK(bus.read_data(atmega32u4.usbs[0].uedatx_address) == 0x06);
    CHECK(bus.read_data(atmega32u4.usbs[0].uedatx_address) == 0x00);
    CHECK(bus.read_data(atmega32u4.usbs[0].uedatx_address) == 0x01);
    CHECK(bus.read_data(atmega32u4.usbs[0].uedatx_address) == 0x00);
    CHECK(bus.read_data(atmega32u4.usbs[0].uedatx_address) == 0x00);
    CHECK(bus.read_data(atmega32u4.usbs[0].uedatx_address) == 0x40);
    CHECK(bus.read_data(atmega32u4.usbs[0].uedatx_address) == 0x00);

    // 5. Firmware clears RXSTPI
    bus.write_data(atmega32u4.usbs[0].ueintx_address, ~0x08); 
    CHECK((bus.read_data(atmega32u4.usbs[0].ueintx_address) & 0x08) == 0);
}

TEST_CASE("USB Enumeration Fidelity - SET_ADDRESS Flow")
{
    Usb usb {"USB", atmega32u4.usbs[0]};
    MemoryBus bus {atmega32u4};
    usb.set_memory_bus(&bus);
    bus.attach_peripheral(usb);
    UsbHostSim host {usb};

    usb.reset();
    host.set_vbus(true);
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0x80); // USBE = 1

    // 1. Host sends SET_ADDRESS(7)
    host.send_setup(0x00, 0x05, 7, 0, 0);

    // 2. Select EP0 and read
    bus.write_data(atmega32u4.usbs[0].uenum_address, 0);
    
    // Check request
    CHECK(bus.read_data(atmega32u4.usbs[0].uedatx_address) == 0x00);
    CHECK(bus.read_data(atmega32u4.usbs[0].uedatx_address) == 0x05);
    
    // 3. Firmware updates address register
    bus.write_data(atmega32u4.usbs[0].udaddr_address, 7 | 0x80); // ADDEN=1, ADDR=7
    
    CHECK(bus.read_data(atmega32u4.usbs[0].udaddr_address) == (7 | 0x80));
}
