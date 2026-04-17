#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "usb_host_sim.hpp"
#include "vioavr/core/devices/atmega32u4.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
using namespace vioavr::core::testing;
}

TEST_CASE("USB Protocol Fidelity - IN Transaction Flow")
{
    Usb usb {"USB", atmega32u4.usbs[0]};
    MemoryBus bus {atmega32u4};
    usb.set_memory_bus(&bus);
    bus.attach_peripheral(usb);
    UsbHostSim host {usb};

    usb.reset();
    host.set_vbus(true);
    bus.write_data(0x49U, 0x01U); // Lock PLL
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0x80); // USBE=1

    // 1. Configure EP1 as IN
    bus.write_data(atmega32u4.usbs[0].uenum_address, 1);
    bus.write_data(atmega32u4.usbs[0].ueconx_address, 0x01); // EPEN=1
    bus.write_data(atmega32u4.usbs[0].uecfg0x_address, 0x80); // TYPE=IN
    bus.write_data(atmega32u4.usbs[0].uecfg1x_address, 0x32); // EPSIZE=64, ALLOC=1

    // 2. Verify TXINI is set initially (Ready to load)
    CHECK((bus.read_data(atmega32u4.usbs[0].ueintx_address) & 0x01) != 0);

    // 3. Firmware writes data to EP1
    bus.write_data(atmega32u4.usbs[0].uedatx_address, 'H');
    bus.write_data(atmega32u4.usbs[0].uedatx_address, 'i');

    // 4. Firmware clears TXINI to signal "Ready to send"
    bus.write_data(atmega32u4.usbs[0].ueintx_address, (u8)~0x01U);
    CHECK((bus.read_data(atmega32u4.usbs[0].ueintx_address) & 0x01) == 0);

    // 5. Host sends IN token and receives data
    auto packet = host.receive_in_packet(1);
    
    REQUIRE(packet.size() == 2);
    CHECK(packet[0] == 'H');
    CHECK(packet[1] == 'i');

    // 6. Verify TXINI is set again (ACK received, ready for more)
    CHECK((bus.read_data(atmega32u4.usbs[0].ueintx_address) & 0x01) != 0);
}

TEST_CASE("USB Protocol Fidelity - SOF Timing")
{
    Usb usb {"USB", atmega32u4.usbs[0]};
    MemoryBus bus {atmega32u4};
    usb.set_memory_bus(&bus);
    bus.attach_peripheral(usb);
    UsbHostSim host {usb};

    usb.reset();
    bus.write_data(0x49U, 0x01U); // Lock PLL
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0xA0); // USBE=1, FRZCLK=1
    // FRZCLK is 1, SOF should NOT trigger
    
    bus.tick_peripherals(20000); // 1.25ms
    CHECK((bus.read_data(atmega32u4.usbs[0].udint_address) & 0x04) == 0); // SOFI=0

    // Unfreeze clock
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0x80); // USBE=1, FRZCLK=0
    
    bus.tick_peripherals(16000); // 1.0ms
    CHECK((bus.read_data(atmega32u4.usbs[0].udint_address) & 0x04) != 0); // SOFI=1
    
    // Verify Frame Number
    u16 fnum = bus.read_data(atmega32u4.usbs[0].udfnum_address);
    fnum |= (static_cast<u16>(bus.read_data(atmega32u4.usbs[0].udfnum_address + 1)) << 8U);
    CHECK(fnum == 1);
}
