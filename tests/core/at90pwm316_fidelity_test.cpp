#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/psc.hpp"
#include "vioavr/core/dac.hpp"
#include "vioavr/core/devices/at90pwm316.hpp"

using namespace vioavr::core;

TEST_CASE("AT90PWM316 - PSC Prescaler Timing") {
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    
    // PSC0 addresses for AT90PWM316
    const u16 pctl0 = 0xDB;
    const u16 pconf0 = 0xDA;
    const u16 ocrrb0 = 0xD8; // OCR0RB (TOP)
    
    // Set OCR0RB to 100
    bus.write_data(ocrrb0, 100);
    bus.write_data(ocrrb0 + 1, 0);
    
    // Enable PSC0, One-Ramp, Prescaler /32
    // PPRE = 10 (bits 7:6), PRUN = 1 (bit 0) -> 0x81
    bus.write_data(pctl0, 0x81);
    
    // PCNF0: One-Ramp (PMODE=00, bits 4:3)
    bus.write_data(pconf0, 0x00);
    
    // Run for 100 ticks (3200 cycles)
    for (int i = 0; i < 100; ++i) {
        bus.tick_peripherals(32);
    }
    
    // Check interrupt flags.
    const u16 pifr0 = 0xA0;
    CHECK((bus.read_data(pifr0) & 0x01) != 0);
}

TEST_CASE("AT90PWM316 DAC Fidelity") {
    auto machine = std::make_unique<Machine>(devices::at90pwm316);
    auto& bus = machine->bus();
    
    const u16 dacon = 0xAA;
    const u16 dacl = 0xAB;
    const u16 dach = 0xAC;
    
    // Enable DAC (DAEN=1, DACOE=1)
    bus.write_data(dacon, 0x03);
    
    // Write 512 (mid-range, ~2.5V or 0.5 normalized)
    bus.write_data(dacl, 0x00);
    bus.write_data(dach, 0x02); // 0x200 = 512
    
    // Verify peripheral internal state
    Dac* dac_ptr = nullptr;
    for (auto* p : bus.peripherals()) {
        if (auto* d = dynamic_cast<Dac*>(p)) {
            dac_ptr = d;
            break;
        }
    }
    REQUIRE(dac_ptr != nullptr);
    REQUIRE(dac_ptr->voltage() == doctest::Approx(512.0 / 1023.0));
    
    // Tick DAC to propagate to PinMux
    bus.tick_peripherals(1);
    
    auto state = bus.pin_mux()->get_state_by_address(0x23, 7); // PB7/DACOUT
    CHECK(state.owner == PinOwner::dac);
    CHECK(state.voltage == doctest::Approx(512.0 / 1023.0));
}

TEST_CASE("AT90PWM316 - EUSART Timing") {
    auto machine = std::make_unique<Machine>(devices::at90pwm316);
    auto& bus = machine->bus();
    
    const u16 eucsra = 0xC8;
    const u16 eucsrb = 0xC9;
    const u16 eudr = 0xCE;
    const u16 mubrrl = 0xCC;
    
    // Enable EUSART (EUSART=1)
    bus.write_data(eucsrb, 0x10);
    // MUBRR = 0 (fastest) -> 16 cycles per bit
    bus.write_data(mubrrl, 0);
    
    // Write byte to EUDR
    bus.write_data(eudr, 0x55);
    
    // Check EOT (bit 7) is not set yet
    CHECK((bus.read_data(eucsra) & 0x80) == 0);
    
    // Run for 160 cycles (10 bits * 16 cycles)
    bus.tick_peripherals(160);
    
    // Check EOT is now set
    CHECK((bus.read_data(eucsra) & 0x80) != 0);
}
