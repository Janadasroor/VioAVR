#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("GPIO Input Pin Fidelity Test") {
    MemoryBus bus {atmega328p};
    AvrCpu cpu {bus};

    // 1. Setup Port B and Port D
    GpioPort portb {"PORTB", 0x23U, 0x24U, 0x25U};
    GpioPort portd {"PORTD", 0x29U, 0x2AU, 0x2BU};
    bus.attach_peripheral(portb);
    bus.attach_peripheral(portd);

    // 2. Load the input_test firmware
    const auto image = HexImageLoader::load_file("../tests/input_test.hex", atmega328p);
    bus.load_image(image);
    cpu.reset();

    // 3. Run initial setup (DDRB, DDRD, PORTD pullup)
    // About 20 cycles is enough for main() setup
    cpu.run(20);

    // 4. Force PD2 to LOW externally (bit 2 = 0)
    portd.set_input_levels(0x00U);
    
    // 5. Run simulation for a few cycles to let firmware read PIND and update PORTB
    cpu.run(100); // 10 was too small for GCC delay and loop
    
    // 6. Verify PB5 is LOW
    CHECK((portb.read(0x25U) & (1 << 5)) == 0);

    // 7. Force PD2 to HIGH externally (bit 2 = 1)
    portd.set_input_levels(1 << 2);
    
    // 8. Run simulation
    cpu.run(100);

    // 9. Verify PB5 is HIGH
    CHECK((portb.read(0x25U) & (1 << 5)) != 0);
}
