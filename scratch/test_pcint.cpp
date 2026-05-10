#include <iostream>
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

int main() {
    PinMux mux(32);
    GpioPort port("PORTB", 0x23, 0x24, 0x25, mux);
    PinChangeInterruptDescriptor desc {
        .pcicr_address = 0x68,
        .pcifr_address = 0x3B,
        .pcmsk_address = 0x6B,
        .pcicr_enable_mask = 0x01,
        .pcifr_flag_mask = 0x01,
        .vector_index = 3
    };
    PinChangeInterrupt pci("PCINT0", desc, port);
    
    MemoryBus bus(atmega328p);
    bus.set_pin_mux(&mux);
    bus.attach_peripheral(port);
    bus.attach_peripheral(pci);
    
    // Enable PCINT0
    bus.write_data(0x6B, 0x01); // PCMSK0.0 = 1
    bus.write_data(0x68, 0x01); // PCICR.0 = 1
    
    std::cout << "PCMSK0 read back: " << (int)bus.read_data(0x6B) << std::endl;
    std::cout << "PCICR read back: " << (int)bus.read_data(0x68) << std::endl;
    
    // Initial state
    port.set_input_levels(0x00);
    bus.tick_peripherals(1);
    
    InterruptRequest req;
    std::cout << "Pending before change: " << bus.pending_interrupt_request(req) << std::endl;
    
    // Change
    port.set_input_levels(0x01);
    bus.tick_peripherals(1);
    
    std::cout << "Pending after change: " << bus.pending_interrupt_request(req) << std::endl;
    if (bus.pending_interrupt_request(req)) {
        std::cout << "Vector index: " << (int)req.vector_index << std::endl;
    }
    
    return 0;
}
