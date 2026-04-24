#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega328p.hpp"

using namespace vioavr::core;

TEST_CASE("GPIO Fidelity: PinMux Arbitration & PUD") {
    auto machine = Machine::create_for_device("ATmega328P");
    REQUIRE(machine != nullptr);
    
    auto& cpu = machine->cpu();
    auto& bus = machine->bus();
    auto& mux = machine->pin_mux();
    auto* portb = machine->get_port("PORTB");
    REQUIRE(portb != nullptr);
    
    const auto& d = bus.device();
    
    // Test Case 1: GPIO Drive
    // Configure PB0 as output, set HIGH
    // For ATmega328P: ports[0] is PORTB. GpioPort maps it to index 1 (B=1)
    bus.write_data(d.ports[0].ddr_address, 0x01); // DDRB = 0x01
    bus.write_data(d.ports[0].port_address, 0x01); // PORTB = 0x01
    
    auto state = mux.get_state(1, 0); // Port 1 (B), Pin 0
    CHECK(state.owner == PinOwner::gpio);
    CHECK(state.is_output == true);
    CHECK(state.drive_level == true);
    
    // Test Case 2: Peripheral Priority Override
    // Claim PB0 for SPI (higher priority than GPIO)
    mux.claim_pin(1, 0, PinOwner::spi);
    
    // Initially SPI wants to drive LOW (default)
    mux.update_pin(1, 0, PinOwner::spi, true, false);
    
    state = mux.get_state(1, 0);
    CHECK(state.owner == PinOwner::spi);
    CHECK(state.drive_level == false); // SPI (LOW) overrides GPIO (HIGH)
    
    // Update SPI to drive HIGH
    mux.update_pin(1, 0, PinOwner::spi, true, true);
    state = mux.get_state(1, 0);
    CHECK(state.drive_level == true);
    
    // Release SPI
    mux.release_pin(1, 0, PinOwner::spi);
    state = mux.get_state(1, 0);
    CHECK(state.owner == PinOwner::gpio);
    CHECK(state.drive_level == true); // Returns to GPIO level
}

TEST_CASE("GPIO Fidelity: Pull-up Suppression (PUD)") {
    auto machine = Machine::create_for_device("ATmega328P");
    auto& bus = machine->bus();
    auto& mux = machine->pin_mux();
    const auto& d = bus.device();
    
    // Enable pull-up on PB1 (DDR=0, PORT=1)
    bus.write_data(d.ports[0].ddr_address, 0x00);
    bus.write_data(d.ports[0].port_address, 0x02);
    
    auto state = mux.get_state(1, 1);
    CHECK(state.pullup_enabled == true);
    
    // Set PUD bit in MCUCR (bit 4)
    bus.write_data(d.mcucr_address, 0x10);
    
    state = mux.get_state(1, 1);
    CHECK(state.pullup_enabled == false); // Suppressed by PUD
    
    // Clear PUD
    bus.write_data(d.mcucr_address, 0x00);
    state = mux.get_state(1, 1);
    CHECK(state.pullup_enabled == true);
}

TEST_CASE("GPIO Fidelity: PIN Register Toggle") {
    auto machine = Machine::create_for_device("ATmega328P");
    auto& bus = machine->bus();
    const auto& d = bus.device();
    
    // Initial PORTB = 0
    bus.write_data(d.ports[0].port_address, 0x00);
    CHECK(bus.read_data(d.ports[0].port_address) == 0x00);
    
    // Write 1 to PINB.0 -> should toggle PORTB.0
    bus.write_data(d.ports[0].pin_address, 0x01);
    CHECK(bus.read_data(d.ports[0].port_address) == 0x01);
    
    // Write 1 again -> toggle back to 0
    bus.write_data(d.ports[0].pin_address, 0x01);
    CHECK(bus.read_data(d.ports[0].port_address) == 0x00);
    
    // Multiple bits
    bus.write_data(d.ports[0].pin_address, 0x03);
    CHECK(bus.read_data(d.ports[0].port_address) == 0x03);
}
