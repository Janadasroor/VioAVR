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

    // Select and Enable EP0
    bus.write_data(atmega32u4.usbs[0].uenum_address, 0);
    bus.write_data(atmega32u4.usbs[0].ueconx_address, 0x01); // EPEN=1
    // Configure EP0 as Control 8-byte
    bus.write_data(atmega32u4.usbs[0].uecfg0x_address, 0x00); // Control
    bus.write_data(atmega32u4.usbs[0].uecfg1x_address, 0x02); // 8-byte, 1-bank, ALLOC=1
    
    // Verify TXINI is set after enable
    CHECK((bus.read_data(atmega32u4.usbs[0].ueintx_address) & 0x01) != 0);

    // 1. Host sends SET_ADDRESS(15)
    Usb::SetupPacket setup { 0x00, 0x05, 15, 0, 0 };
    usb.simulate_setup_packet(setup);
    
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

TEST_CASE("USB Clock Dependency: PLL Lock")
{
    Usb usb {"USB", atmega32u4.usbs[0]};
    MemoryBus bus {atmega32u4};
    usb.set_memory_bus(&bus);
    bus.attach_peripheral(usb);
    
    // 1. Enable USB but NO PLL
    usb.reset();
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0x80); // USBE=1, FRZCLK=0
    
    // Tick for a long time (1s)
    usb.tick(16000000); 
    
    // Verify SOFI is NOT set (no clock)
    CHECK((bus.read_data(atmega32u4.usbs[0].udint_address) & 0x04) == 0);
    
    // 2. Attach a RegisterFile to simulate the PLLCSR at 0x49
    // (Or just use a custom peripheral for 0x49)
    struct PllMock : IoPeripheral {
        u8 val {0};
        std::string_view name() const noexcept override { return "PLL"; }
        std::span<const AddressRange> mapped_ranges() const noexcept override {
            static const std::array<AddressRange, 1> r {{{0x49, 0x49}}};
            return r;
        }
        void reset() noexcept override { val = 0; }
        void tick(u64) noexcept override {}
        u8 read(u16) noexcept override { return val; }
        void write(u16, u8 v) noexcept override { val = v; }
    };
    
    PllMock pll;
    bus.attach_peripheral(pll);
    
    // Set PLOCK=1
    pll.val = 0x01; 
    
    // Tick again
    usb.tick(16000); 
    
    // Verify SOFI is now set
    CHECK((bus.read_data(atmega32u4.usbs[0].udint_address) & 0x04) != 0);
}
