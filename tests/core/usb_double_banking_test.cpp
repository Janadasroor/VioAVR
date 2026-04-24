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

TEST_CASE("USB Double Banking Fidelity")
{
    Usb usb {"USB", atmega32u4.usbs[0]};
    MemoryBus bus {atmega32u4};
    usb.set_memory_bus(&bus);
    bus.attach_peripheral(usb);
    UsbHostSim host {usb};

    usb.reset();
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0x80); // USBE=1

    // 1. Configure EP1 as IN with 2 Banks, 64 bytes
    bus.write_data(atmega32u4.usbs[0].uenum_address, 1);
    bus.write_data(atmega32u4.usbs[0].ueconx_address, 0x01); // EPEN=1
    bus.write_data(atmega32u4.usbs[0].uecfg0x_address, 0x80); // TYPE=IN
    bus.write_data(atmega32u4.usbs[0].uecfg1x_address, 0x36); // EPSIZE=64, EPBK=2 banks, ALLOC=1

    // 2. Initial state: CURRBK=0, NBUSYBK=0
    u8 uesta1x = bus.read_data(atmega32u4.usbs[0].uesta1x_address);
    CHECK((uesta1x & 0x01) == 0); // CURRBK
    CHECK((uesta1x & 0x06) == 0); // NBUSYBK

    // 3. Load Bank 0
    bus.write_data(atmega32u4.usbs[0].uedatx_address, 'B');
    bus.write_data(atmega32u4.usbs[0].uedatx_address, '0');
    
    // Clear FIFOCON to release Bank 0 to SIE
    bus.write_data(atmega32u4.usbs[0].ueintx_address, 0x7F); 
    
    // 4. State check: CURRBK should be 1, NBUSYBK should be 1
    uesta1x = bus.read_data(atmega32u4.usbs[0].uesta1x_address);
    CHECK((uesta1x & 0x01) == 1); // CURRBK switched to Bank 1
    CHECK((uesta1x & 0x06) == 0x02); // NBUSYBK = 1

    // 5. Load Bank 1
    bus.write_data(atmega32u4.usbs[0].uedatx_address, 'B');
    bus.write_data(atmega32u4.usbs[0].uedatx_address, '1');
    
    // Clear FIFOCON to release Bank 1 to SIE
    bus.write_data(atmega32u4.usbs[0].ueintx_address, 0x7F);

    // 6. State check: NBUSYBK should be 2, TXINI should be 0 (all banks full)
    uesta1x = bus.read_data(atmega32u4.usbs[0].uesta1x_address);
    CHECK((uesta1x & 0x06) == 0x04); // NBUSYBK = 2
    CHECK((bus.read_data(atmega32u4.usbs[0].ueintx_address) & 0x01) == 0); // TXINI=0

    // 7. Host pulls first packet (should be Bank 0)
    auto packet1 = host.receive_in_packet(1);
    REQUIRE(packet1.size() == 2);
    CHECK(packet1[0] == 'B');
    CHECK(packet1[1] == '0');

    // 8. State check: NBUSYBK should be 1
    uesta1x = bus.read_data(atmega32u4.usbs[0].uesta1x_address);
    CHECK((uesta1x & 0x06) == 0x02); // NBUSYBK = 1

    // 9. Host pulls second packet (should be Bank 1)
    auto packet2 = host.receive_in_packet(1);
    REQUIRE(packet2.size() == 2);
    CHECK(packet2[0] == 'B');
    CHECK(packet2[1] == '1');

    // 10. State check: NBUSYBK should be 0
    uesta1x = bus.read_data(atmega32u4.usbs[0].uesta1x_address);
    CHECK((uesta1x & 0x06) == 0x00); // NBUSYBK = 0
}
