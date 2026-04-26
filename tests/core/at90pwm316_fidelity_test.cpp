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
    bus.write_data(ocrrb0 + 1, 0);
    bus.write_data(ocrrb0, 100);
    
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
    bus.write_data(dach, 0x02); // 0x200 = 512
    bus.write_data(dacl, 0x00);
    
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

TEST_CASE("AT90PWM316 - EUSART Manchester Encoding") {
    auto machine = std::make_unique<Machine>(devices::at90pwm316);
    auto& bus = machine->bus();
    
    const u16 eucsra = 0xC8;
    const u16 eucsrb = 0xC9;
    const u16 eudr = 0xCE;
    const u16 mubrrl = 0xCC;
    
    // Enable EUSART, Manchester mode, LSB first
    // EUSART (bit 4) = 1, EMCH (bit 1) = 1 -> 0x12
    bus.write_data(eucsrb, 0x12);
    // MUBRR = 0 -> 16 cycles per bit (8 cycles per half)
    bus.write_data(mubrrl, 0);
    
    // Write 0x01 (LSB is 1)
    bus.write_data(eudr, 0x01);
    
    // Tick 4 cycles
    bus.tick_peripherals(4);
    
    // Check TXD pin (PD1 for AT90PWM316 EUSART)
    // For '1', first half should be LOW (0), second half HIGH (1)
    auto state = bus.pin_mux()->get_state_by_address(0x29, 1); // PIND1
    CHECK(state.drive_level == false);
    
    // Tick another 8 cycles (total 12, now in second half of first bit)
    bus.tick_peripherals(8);
    state = bus.pin_mux()->get_state_by_address(0x29, 1);
    CHECK(state.drive_level == true);
    
    // Write 0x00 (all bits 0)
    bus.write_data(eudr, 0x00);
    bus.tick_peripherals(16); // Finish first byte
    
    // Now start 0x00 (LSB 0)
    // For '0', first half should be HIGH (1), second half LOW (0)
    bus.tick_peripherals(4);
    state = bus.pin_mux()->get_state_by_address(0x29, 1);
    CHECK(state.drive_level == true);
}

TEST_CASE("AT90PWM316 - PSC Complete Cycle (PCCYC)") {
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    
    const u16 pctl0 = 0xDB;
    const u16 ocrrb0 = 0xD8;
    
    bus.write_data(ocrrb0 + 1, 0);
    bus.write_data(ocrrb0, 10); // TOP = 10
    
    // Enable PSC0, One-Ramp, Prescaler /1, PCCYC = 1
    // PCCYC is bit 1. PRUN is bit 0. -> 0x03
    bus.write_data(pctl0, 0x03);
    
    // Check it's running
    CHECK((bus.read_data(pctl0) & 0x01) != 0);
    
    // Run for 12 ticks (just over one cycle)
    bus.tick_peripherals(12);
    
    // Check PRUN is now CLEARED
    CHECK((bus.read_data(pctl0) & 0x01) == 0);
}

TEST_CASE("AT90PWM316 - PSC Fault Protection") {
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    
    // Configure PSC0: One-Ramp, PRUN=1, PSOC0: POEN0A=1
    bus.write_data(0xD0, 0x01); // PSOC0 = 1 (POEN0A=1)
    bus.write_data(0xDB, 0x01); // PCTL0 = 1 (PRUN=1)
    
    // Initially, pin PD1 should be driven by PSC
    // (Wait, we need to tick once to claim)
    bus.tick_peripherals(1);
    auto state = bus.pin_mux()->get_state_by_address(0x29, 1);
    CHECK(state.owner == PinOwner::psc);
    
    // Set non-zero duty cycle so we can see it go LOW on fault
    bus.write_data(0xD8 + 1, 0); bus.write_data(0xD8, 100); // TOP=100
    bus.write_data(0xD4 + 1, 0); bus.write_data(0xD4, 0);   // SA=0
    bus.write_data(0xD6 + 1, 0); bus.write_data(0xD6, 50);  // RA=50
    
    // Set PSC0 Fault Mode: Fault on Level (Mode 6), No Filter
    bus.write_data(0xDC, 0x06); // PFRC0A = 6 (PELEV=0, trigger on LOW)
    
    // Trigger Fault (LOW level)
    for (auto* p : bus.peripherals()) {
        if (auto* psc = dynamic_cast<Psc*>(p)) {
            if (psc->name() == "PSC0" || psc->name() == "PSC") {
                psc->notify_fault(false, 0); // Fault A = false (triggered)
            }
        }
    }
    
    // Tick to process fault
    bus.tick_peripherals(1);
    
    // Check PSC0 output. In mode 6, it should go to Reset Value.
    // psoc0 was 1 (POEN0A=1), bit 1 (Fault level) is 0.
    state = bus.pin_mux()->get_state_by_address(0x2B, 1); // PSCOUT00 (PD1)
    CHECK(state.drive_level == false);
}

TEST_CASE("AT90PWM316 - EUSART Manchester Decoding") {
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    
    // Enable EUSART RX, Manchester, 16-bit frames
    bus.write_data(0xC9, 0x12); // EUCSRB: EUSART=1, EMCH=1
    bus.write_data(0xC8, 0x0B); // EUCSRA: URxS=11 (16-bit)
    bus.write_data(0xCC, 0);    // MUBRR=0 (16 cycles per bit)
    
    // Claim PD2 as external source (RXD)
    bus.pin_mux()->claim_pin_by_address(0x29, 2, PinOwner::external);
    
    // Initial state: HIGH (Idle)
    bus.pin_mux()->update_pin_by_address(0x29, 2, PinOwner::external, true, true);
    bus.tick_peripherals(1);

    auto send_bit = [&](bool bit) {
        if (bit) {
            bus.pin_mux()->update_pin_by_address(0x29, 2, PinOwner::external, true, false);
            for (int i = 0; i < 8; ++i) bus.tick_peripherals(1);
            bus.pin_mux()->update_pin_by_address(0x29, 2, PinOwner::external, true, true);
            for (int i = 0; i < 8; ++i) bus.tick_peripherals(1);
        } else {
            bus.pin_mux()->update_pin_by_address(0x29, 2, PinOwner::external, true, true);
            for (int i = 0; i < 8; ++i) bus.tick_peripherals(1);
            bus.pin_mux()->update_pin_by_address(0x29, 2, PinOwner::external, true, false);
            for (int i = 0; i < 8; ++i) bus.tick_peripherals(1);
        }
    };
    
    // Send Start Bit (1)
    send_bit(true);

    // Send 0x1234 (LSB first)
    u16 data = 0x1234;
    for (int i = 0; i < 16; ++i) {
        send_bit((data >> i) & 1);
    }
    
    // Final ticks to process the last bit and push to queue
    bus.tick_peripherals(32);
    
    // Read EUDR (16-bit)
    u8 low = bus.read_data(0xCE);
    u8 high = bus.read_data(0xCF);
    u16 received = (static_cast<u16>(high) << 8) | low;
    
    CHECK(received == 0x1234);
}

TEST_CASE("AT90PWM316 - PSC PAOC Fault Bypass") {
    auto machine = Machine::create_for_device("AT90PWM316");
    REQUIRE(machine != nullptr);
    auto& bus = machine->bus();
    
    const u16 pctl0 = 0xDB;
    const u16 psoc0 = 0xD0;
    const u16 ocrsa0 = 0xD2;
    const u16 ocrra0 = 0xD4;
    const u16 ocrrb0 = 0xD8;
    
    // Set 50% duty cycle (TOP=100, RA=50)
    bus.write_data(ocrrb0 + 1, 0); bus.write_data(ocrrb0, 100);
    bus.write_data(ocrsa0 + 1, 0); bus.write_data(ocrsa0, 0);
    bus.write_data(ocrra0 + 1, 0); bus.write_data(ocrra0, 50);
    
    // Enable Output A, Fault Level = LOW (PSOC bit 1 = 0)
    bus.write_data(psoc0, 0x01); 
    
    // Deactivate fault input (High level is inactive for PELEV=0)
    for (auto* p : bus.peripherals()) {
        if (auto* psc = dynamic_cast<Psc*>(p)) {
            if (psc->name() == "PSC0" || psc->name() == "PSC") {
                psc->notify_fault(true, 0); // Fault A = true (inactive)
            }
        }
    }
    
    // 1. Enable PSC0 with PAOC0A = 0, CLKSEL = 1 (IO Clock)
    bus.write_data(pctl0, 0x03); 
    // Set PSC0 Fault Mode: Fault on Level (Mode 6)
    bus.write_data(0xDC, 0x06); // PFRC0A = 6
    bus.tick_peripherals(1);
    
    // Check output is HIGH (normal operation)
    auto state = bus.pin_mux()->get_state_by_address(0x2B, 1);
    CHECK(state.drive_level == true);
    
    // 2. Inject fault directly into PSC (LOW level triggers if PELEV=0)
    for (auto* p : bus.peripherals()) {
        if (auto* psc = dynamic_cast<Psc*>(p)) {
            if (psc->name() == "PSC0" || psc->name() == "PSC") {
                psc->notify_fault(false, 0); // Fault A = false (triggered)
            }
        }
    }
    bus.tick_peripherals(1);
    
    // Check output is now LOW (Fault active, no bypass)
    state = bus.pin_mux()->get_state_by_address(0x2B, 1);
    CHECK(state.drive_level == false);
    
    // 3. Enable PAOC0A = 1 (Bypass fault)
    bus.write_data(pctl0, 0x0B); 
    bus.tick_peripherals(1);
    
    // Check output is now HIGH again (Fault bypassed)
    state = bus.pin_mux()->get_state_by_address(0x2B, 1);
    CHECK(state.drive_level == true);
}

TEST_CASE("AT90PWM316 - PSC Blanking Window") {
    auto* device_ptr = DeviceCatalog::find("AT90PWM316");
    REQUIRE(device_ptr != nullptr);
    auto psc_desc = device_ptr->pscs[0];
    psc_desc.blanking_duration = 10; // 10 cycles of blanking

    DeviceDescriptor custom_device = *device_ptr;
    custom_device.pscs[0] = psc_desc;

    auto machine = std::make_unique<Machine>(custom_device);
    REQUIRE(machine != nullptr);

    auto& bus = machine->bus();
    bus.pin_mux()->register_port(0x23, 0); // Port B
    bus.pin_mux()->register_port(0x29, 2); // Port D

    u16 pctl0 = 0xDB;
    u16 ocrsa0 = 0xD2;
    u16 ocrra0 = 0xD4;
    u16 psoc0 = 0xD0;

    // Configure PWM: SA=0, RA=50
    bus.write_data(ocrsa0 + 1, 0); bus.write_data(ocrsa0, 0);
    bus.write_data(ocrra0 + 1, 0); bus.write_data(ocrra0, 50);
    bus.write_data(psoc0, 0x01); // Enable OutA
    
    // Set Mode: Fault on Level (Mode 6)
    bus.write_data(0xDC, 0x06); // PFRC0A = 6
    
    // 1. Enable PSC and tick to start
    bus.write_data(pctl0, 0x03); // PRUN=1, CLKSEL=1
    bus.tick_peripherals(1);
    
    // Output should be HIGH
    auto state = bus.pin_mux()->get_state_by_address(0x2B, 1);
    CHECK(state.drive_level == true);

    // 2. Inject fault during blanking (Switching event just happened at start)
    for (auto* p : bus.peripherals()) {
        if (auto* psc = dynamic_cast<Psc*>(p)) {
            if (psc->name() == "PSC0" || psc->name() == "PSC") {
                psc->notify_fault(false, 0); // Trigger fault
            }
        }
    }
    
    // Tick 5 cycles (Inside blanking window)
    bus.tick_peripherals(5);
    state = bus.pin_mux()->get_state_by_address(0x2B, 1);
    // Should STILL be HIGH because of blanking
    CHECK(state.drive_level == true);

    // 3. Wait for blanking to expire
    bus.tick_peripherals(6); // Total 11 cycles since switching
    state = bus.pin_mux()->get_state_by_address(0x2B, 1);
    // Should now be LOW because fault is accepted
    CHECK(state.drive_level == false);
}
