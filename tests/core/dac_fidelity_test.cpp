#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/dac.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/at90pwm1.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("DAC Fidelity - Basic Voltage Output") {
    const auto& desc = at90pwm1.dacs[0];
    Dac dac {"DAC0", desc};
    
    // Set up dummy bus and pin mux
    PinMux mux {3};
    mux.register_port(0x23, 0); // PINB
    mux.register_port(0x24, 0); // DDRB
    mux.register_port(0x25, 0); // PORTB
    MemoryBus bus {at90pwm1};
    bus.set_pin_mux(&mux);
    dac.set_memory_bus(&bus);
    
    dac.reset();
    
    // Enable DAC and Output Enable (DAEN=1, DAOE=1)
    dac.write(desc.dacon_address, desc.daen_mask | desc.dacoe_mask);
    
    // Write 10-bit value: 512 (approx 0.5V if Vref=1V)
    // 512 = 0x200. DACH = 0x02, DACL = 0x00 (Right adjust)
    dac.write(desc.dacl_address, 0x00);
    dac.write(desc.dach_address, 0x02); // Triggers update
    
    dac.tick(1);
    
    CHECK(dac.voltage() == doctest::Approx(512.0 / 1023.0));
    
    // Verify pin update
    auto state = mux.get_state_by_address(desc.dac_pin_address, desc.dac_pin_bit);
    CHECK(state.owner == PinOwner::dac);
    CHECK(state.voltage == doctest::Approx(512.0 / 1023.0));
}

TEST_CASE("DAC Fidelity - Left Adjust") {
    const auto& desc = at90pwm1.dacs[0];
    Dac dac {"DAC0", desc};
    
    PinMux mux {3};
    MemoryBus bus {at90pwm1};
    bus.set_pin_mux(&mux);
    dac.set_memory_bus(&bus);
    
    dac.reset();
    
    // Enable DAC, OE and Left Adjust (DAEN=1, DAOE=1, DALA=1)
    dac.write(desc.dacon_address, desc.daen_mask | desc.dacoe_mask | desc.dala_mask);
    
    // 10-bit value 512 in Left Adjust:
    // DACH = 0x80 (bits 9:2 = 10000000)
    // DACL = 0x00 (bits 1:0 = 00 in bits 7:6)
    dac.write(desc.dacl_address, 0x00);
    dac.write(desc.dach_address, 0x80); 
    
    dac.tick(1);
    CHECK(dac.voltage() == doctest::Approx(512.0 / 1023.0));
}
