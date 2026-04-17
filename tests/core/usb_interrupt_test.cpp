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

TEST_CASE("USB Interrupt and Handshake Fidelity")
{
    Usb usb {"USB", atmega32u4.usbs[0]};
    MemoryBus bus {atmega32u4};
    usb.set_memory_bus(&bus);
    bus.attach_peripheral(usb);

    usb.reset();
    
    // 1. Enable USB peripheral
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0x80); // USBE = 1

    // 2. Select Endpoint 0
    bus.write_data(atmega32u4.usbs[0].uenum_address, 0);

    // 3. Manually simulate an event (e.g., RXOUTI - bit 2)
    // Note: In real SIE, hardware would set this bit. 
    // For now we test firmware interaction.
    // We need a way to set bits internally for testing or just writing to it?
    // Actually, UEINTX is R/W for clearing but hardware sets it.
    // In our implementation, we allow writing everything except clear-on-0.
    
    // Set a flag manually (simulate SIE)
    usb.force_endpoint_interrupt(0, 0xFF);
    CHECK(usb.read(atmega32u4.usbs[0].ueintx_address) == 0xFF);

    // Clear RXOUTI (bit 2) by writing 0 to it
    bus.write_data(atmega32u4.usbs[0].ueintx_address, 0xFB); // 1111 1011
    CHECK((usb.read(atmega32u4.usbs[0].ueintx_address) & 0x04) == 0);
    CHECK((usb.read(atmega32u4.usbs[0].ueintx_address) & 0x01) == 0x01); // TXINI still set

    // 4. Test Interrupt Propagation
    // Enable interrupt for TXINI (bit 0)
    bus.write_data(atmega32u4.usbs[0].ueienx_address, 0x01);
    
    // Verify UEINT summary register
    CHECK((usb.read(atmega32u4.usbs[0].ueint_address) & 0x01) != 0); // Bit 0 (EP0) should be set

    // Check Interface call
    InterruptRequest request;
    CHECK(usb.pending_interrupt_request(request) == true);
    CHECK(request.vector_index == atmega32u4.usbs[0].com_vector_index);

    // 5. Clear flag and verify interrupt clears
    bus.write_data(atmega32u4.usbs[0].ueintx_address, 0xFE); // Clear bit 0
    CHECK(usb.pending_interrupt_request(request) == false);
}

TEST_CASE("USB General Interrupt (VBUS/Reset)")
{
    Usb usb {"USB", atmega32u4.usbs[0]};
    MemoryBus bus {atmega32u4};
    usb.set_memory_bus(&bus);
    bus.attach_peripheral(usb);

    usb.reset();
    bus.write_data(atmega32u4.usbs[0].usbcon_address, 0x80); // USBE = 1

    // Enable End of Reset Interrupt (EORSTE - bit 3)
    bus.write_data(atmega32u4.usbs[0].udien_address, 0x08);

    // Force UDINT flag EORSTI (bit 3)
    usb.force_general_interrupt(0x08);

    InterruptRequest request;
    CHECK(usb.pending_interrupt_request(request) == true);
    CHECK(request.vector_index == atmega32u4.usbs[0].gen_vector_index);
}
