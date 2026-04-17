#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("External Interrupt Fidelity Test") {
    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};
    PinMux pin_mux {3};

    // 1. Setup Port B and Port D
    GpioPort portb {"PORTB", 0x23U, 0x24U, 0x25U};
    GpioPort portd {"PORTD", 0x29U, 0x2AU, 0x2BU};
    ExtInterrupt exint {"EXINT", atmega328p.ext_interrupts[0], pin_mux, 0};
    
    // Connect PinMux to Bus and Port
    PinMap pin_map;
    pin_map.add_mapping("PORTD", 2, 99, "INT0");
    bus.set_pin_map(&pin_map);
    pin_mux.register_port(portd.port_address(), 0);

    bus.attach_peripheral(portb);
    bus.attach_peripheral(portd);
    bus.attach_peripheral(exint);

    // 2. Load the test firmware
    const auto image = HexImageLoader::load_file("../../tests/test_ext_interrupt.hex", atmega328p);
    bus.load_image(image);
    cpu.reset();

    // 3. Run initial setup (DDRB, EIMSK, sei, etc)
    cpu.run(100);

    // Initial state: PB5 should be LOW
    CHECK((portb.read(0x25U) & (1 << 5)) == 0);
    exint.on_external_pin_change(2, PinLevel::high); 
    cpu.run(10);

    // 4. Trigger Falling Edge on PD2
    exint.on_external_pin_change(2, PinLevel::low);

    // 5. Run simulation
    cpu.run(200);

    // 6. Verify PB5 is now HIGH
    CHECK((portb.read(0x25U) & (1 << 5)) != 0);

    // 7. Trigger another Falling Edge
    exint.on_external_pin_change(2, PinLevel::high);
    cpu.run(100);
    exint.on_external_pin_change(2, PinLevel::low);
    cpu.run(200);

    // 8. Verify PB5 is LOW again
    CHECK((portb.read(0x25U) & (1 << 5)) == 0);
}
