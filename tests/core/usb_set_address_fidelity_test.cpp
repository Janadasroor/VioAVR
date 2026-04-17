#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/usb.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("USB SET_ADDRESS Handshake Fidelity")
{
    Usb usb {"USB", atmega32u4.usbs[0]};
    MemoryBus bus {atmega32u4};
    usb.set_memory_bus(&bus);
    bus.attach_peripheral(usb);

    usb.reset();
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0x80); // USBE = 1
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0x80); // Clear FRZCLK (assuming bit 5 is 0)

    // 1. Host sends SET_ADDRESS(15)
    Usb::SetupPacket setup { 0x00, 0x05, 15, 0, 0 };
    usb.simulate_setup_packet(setup);

    // Select EP0
    bus.write_data(atmega32u4.usbs[0].uenum_address, 0);
    
    // Verify RXSTPI is set
    u8 ueintx = bus.read_data(atmega32u4.usbs[0].ueintx_address);
    CHECK((ueintx & 0x08) != 0); 

    // 2. Firmware acks the SETUP by clearing RXSTPI
    bus.write_data(atmega32u4.usbs[0].ueintx_address, (u8)~0x08U);
    CHECK((bus.read_data(atmega32u4.usbs[0].ueintx_address) & 0x08) == 0);

    // 3. Firmware prepares Status Stage (ZLP IN)
    // For ZLP, we just clear TXINI without adding data
    bus.write_data(atmega32u4.usbs[0].ueintx_address, (u8)~0x01U); // Clear TXINI (marks as busy)
    CHECK((bus.read_data(atmega32u4.usbs[0].ueintx_address) & 0x01) == 0);

    // 4. Host sends IN token for status stage
    usb.simulate_in_token(0);
    
    // Verify TXINI is set again (ACK received)
    CHECK((bus.read_data(atmega32u4.usbs[0].ueintx_address) & 0x01) != 0);

    // 5. Firmware updates UDADDR
    bus.write_data(atmega32u4.usbs[0].udaddr_address, 15 | 0x80); // ADDEN=1, ADDR=15
    CHECK(bus.read_data(atmega32u4.usbs[0].udaddr_address) == (15 | 0x80));
}
