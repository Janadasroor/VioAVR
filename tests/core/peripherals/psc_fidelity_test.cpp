#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/psc.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/at90pwm1.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("PSC Fidelity - Basic PWM Cycle") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    // Period = 500 (OCR0RB)
    psc.write(desc.ocrrb_address + 1, 0x01);
    psc.write(desc.ocrrb_address, 0xF4);
    
    // Enable PSC (PRUN = 1)
    psc.write(desc.pctl_address, desc.prun_mask);
    
    psc.tick(500);
    CHECK((psc.read(desc.pifr_address) & desc.ec_flag_mask) != 0); 
}

TEST_CASE("PSC Fidelity - 12-bit Register Access (TEMP Buffer)") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    // Little-Endian: Write High first
    psc.write(desc.ocrsa_address + 1, 0x0A);
    CHECK(psc.read(desc.ocrsa_address + 1) == 0x00);
    
    psc.write(desc.ocrsa_address, 0xBC);
    CHECK(psc.read(desc.ocrsa_address + 1) == 0x0A);
    CHECK(psc.read(desc.ocrsa_address) == 0xBC);
}

TEST_CASE("PSC Fidelity - Interrupt Logic") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    // Period = 10
    psc.write(desc.ocrrb_address + 1, 0);
    psc.write(desc.ocrrb_address, 10);
    psc.write(desc.pctl_address, desc.prun_mask);
    psc.write(desc.pim_address, desc.ec_flag_mask);
    
    psc.tick(10);
    InterruptRequest req;
    CHECK(psc.pending_interrupt_request(req) == true);
    CHECK(req.vector_index == desc.ec_vector_index);
}

TEST_CASE("PSC Fidelity - Center Aligned Mode") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    // Period = 10
    psc.write(desc.ocrrb_address + 1, 0);
    psc.write(desc.ocrrb_address, 10);
    // Mode = 3 for Center Aligned (bits [4:3] = 11)
    psc.write(desc.pconf_address, 0x18); 
    psc.write(desc.pctl_address, desc.prun_mask); 
    
    psc.tick(10); // Up to 10
    CHECK((psc.read(desc.pifr_address) & desc.ec_flag_mask) == 0); 
    psc.tick(11); // Down to 0 + 1 cycle to set flag
    CHECK((psc.read(desc.pifr_address) & desc.ec_flag_mask) != 0);
}

TEST_CASE("PSC Fidelity - Analog Comparator Fault Protection") {
    const auto& psc_desc = at90pwm1.pscs[0];
    const auto& ac_desc = at90pwm1.acs[0];
    
    Psc psc {"PSC0", psc_desc};
    psc.reset();
    // Period = 100
    psc.write(psc_desc.ocrrb_address + 1, 0);
    psc.write(psc_desc.ocrrb_address, 100);
    psc.write(psc_desc.pctl_address, psc_desc.prun_mask); 
    
    psc.write(psc_desc.psoc_address, 0x05); // POEN0A, POEN0B
    psc.write(psc_desc.pfrc0a_address, 0x36); // PELEV=1 (Low), PFLTE=1 (Filter), PRFM=6 (Fault Level Auto)
    
    PinMux mux {3};
    MemoryBus bus {at90pwm1};
    AnalogComparator ac {"AC0", ac_desc, mux, 0};
    ac.reset();
    bus.attach_peripheral(ac);
    ac.connect_psc_fault(psc);
    bus.attach_peripheral(psc);
    
    ac.set_positive_input_voltage(0.8);
    ac.set_negative_input_voltage(0.5);
    bus.tick_peripherals(10); 
    
    ac.set_positive_input_voltage(0.3); // Diff = -0.2 -> Raw Output = 0 (FAULT if PELEV=1)
    bus.tick_peripherals(10); // AC propagates 0 to PSC
    
    bus.tick_peripherals(10); // PSC processes the fault through its filter
    
    CHECK((bus.read_data(psc_desc.pifr_address) & psc_desc.capt_flag_mask) != 0); 
}

TEST_CASE("PSC Fidelity - Output Polarity") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    psc.write(desc.psoc_address, 0x05); 
    psc.write(desc.ocrra_address + 1, 0);
    psc.write(desc.ocrra_address, 10);
    psc.write(desc.ocrrb_address + 1, 0);
    psc.write(desc.ocrrb_address, 10);
    psc.write(desc.pctl_address, desc.prun_mask); 
    
    CHECK(psc.read_output_a() == true);
    CHECK(psc.read_output_b() == true);
 
    psc.write(desc.pconf_address, 0x04); // Set POP0
    psc.tick(1);
    CHECK(psc.read_output_a() == false);
    CHECK(psc.read_output_b() == false);
}

TEST_CASE("PSC Fidelity - Burst Flank Modulation (PBFM)") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    // Period = 100
    psc.write(desc.ocrrb_address + 1, 0);
    psc.write(desc.ocrrb_address, 100);
    
    // First pulse: 10 to 30
    psc.write(desc.ocrsa_address + 1, 0);
    psc.write(desc.ocrsa_address, 10);
    psc.write(desc.ocrra_address + 1, 0);
    psc.write(desc.ocrra_address, 30);
    
    // Second pulse: 60 to 80
    psc.write(desc.ocrsb_address + 1, 0);
    psc.write(desc.ocrsb_address, 60);
    
    // Enable Output A in PSOC
    psc.write(desc.psoc_address, 0x01);
    
    // Enable PBFM (bit 1 of PCTL) and PRUN
    psc.write(desc.pctl_address, desc.prun_mask | desc.pbfm_mask);
    
    // Check initial state (counter=0, before first pulse)
    CHECK(psc.read_output_a() == false);
    
    // Tick to 15 (inside first pulse)
    psc.tick(15);
    CHECK(psc.read_output_a() == true);
    
    // Tick to 40 (between pulses)
    psc.tick(25);
    CHECK(psc.read_output_a() == false);
    
    // Tick to 70 (inside second pulse: 60 to 100)
    psc.tick(30);
    CHECK(psc.read_output_a() == true);
    
    // Tick to 99 (still inside second pulse)
    psc.tick(29);
    CHECK(psc.read_output_a() == true);

    // Tick to 100 -> wraps to 0 (after second pulse)
    psc.tick(1);
    CHECK(psc.read_output_a() == false);
}
