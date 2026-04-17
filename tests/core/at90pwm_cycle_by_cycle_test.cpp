#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/psc.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/devices/at90pwm1.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("AT90PWM - Cycle-by-Cycle Overcurrent Protection Fidelity") {
    // We use PSC0 and AC0 for this test
    const auto& psc_desc = at90pwm1.pscs[0];
    const auto& ac_desc = at90pwm1.acs[0];
    
    PinMux mux {3};
    Psc psc {"PSC0", psc_desc};
    AnalogComparator ac {"AC0", ac_desc, mux, 0};
    
    psc.reset();
    ac.reset();
    ac.connect_psc_fault(psc);

    // 1. Setup PSC0: 100 cycle period
    // OCR0RB = 100
    psc.write(psc_desc.ocrrb_address, 100);
    psc.write(psc_desc.ocrrb_address + 1, 0);
    
    // OCR0RA = 50 (50% Duty Cycle)
    psc.write(psc_desc.ocrra_address, 50);
    psc.write(psc_desc.ocrra_address + 1, 0);

    // 2. Configure Fault Protection (Mode 4: Deactivate outputs)
    // PRFM = 4 (bits 3:0), PELEV = 0 (Normal - bit 5)
    psc.write(psc_desc.pfrc0a_address, 0x04); 

    // 3. Configure PSOC0: Output A enabled
    // PSOC0 bit 0 = 1 (POENA)
    psc.write(psc_desc.psoc_address, 0x01);

    // 4. Start PSC0
    psc.write(psc_desc.pctl_address, psc_desc.prun_mask);

    // --- EXECUTION ---

    // Part A: Normal Operation
    psc.tick(10);
    CHECK(psc.read_output_a() == true); // Active high default

    // Part B: Fault Occurs at cycle 20
    psc.tick(10); // Total 20 cycles
    ac.set_positive_input_voltage(0.9); // Fault condition (Pos > Neg)
    ac.set_negative_input_voltage(0.5);
    ac.tick(1); // Triggers fault
    
    // Output A should go LOW immediately (Faulted)
    CHECK(psc.read_output_a() == false);
    
    // Part C: Fault cleared at cycle 40
    psc.tick(19); // Total 39 cycles
    ac.set_positive_input_voltage(0.3); // Fault cleared
    ac.tick(1); // Total 40 cycles
    
    // In Cycle-by-Cycle mode, the output stays low until the END of the current cycle
    CHECK(psc.read_output_a() == false);
    
    // Part D: Next Cycle Starts at cycle 100 (reload)
    psc.tick(61); // Reach the start of next cycle
    
    // New cycle starts, fault is gone, output should be HIGH again
    CHECK(psc.read_output_a() == true);
}

TEST_CASE("AT90PWM - Multi-Comparator Fault Routing") {
    // Verify that PSC can be sensitive to different comparators
    const auto& psc_desc = at90pwm1.pscs[0];
    const auto& ac0_desc = at90pwm1.acs[0];
    const auto& ac1_desc = at90pwm1.acs[1]; // AC2 technically on AT90PWM1
    
    Psc psc {"PSC0", psc_desc};
    PinMux mux {3};
    AnalogComparator ac0 {"AC0", ac0_desc, mux, 0};
    AnalogComparator ac1 {"AC1", ac1_desc, mux, 1}; // Different source ID
    
    psc.reset();
    ac0.connect_psc_fault(psc);
    ac1.connect_psc_fault(psc);
    
    psc.write(psc_desc.ocrrb_address, 100);
    psc.write(psc_desc.ocrrb_address + 1, 0);
    psc.write(psc_desc.pctl_address, psc_desc.prun_mask);
    psc.write(psc_desc.psoc_address, 0x01); // POENA
    
    // Fault on AC0
    ac0.set_positive_input_voltage(0.9);
    ac0.set_negative_input_voltage(0.5);
    ac0.tick(1);
    
    CHECK(psc.read_output_a() == false); // AC0 halted it
}
