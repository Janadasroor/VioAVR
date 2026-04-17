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

TEST_CASE("PSC Fidelity - Basic PWM Cycle") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    // Period = 500 (OCR0RB)
    psc.write(desc.ocrrb_address, 0xF4);
    psc.write(desc.ocrrb_address + 1, 0x01);
    
    CHECK(psc.read(desc.ocrrb_address) == 0xF4);
    CHECK(psc.read(desc.ocrrb_address + 1) == 0x01);

    // Enable PSC (PRUN = 1)
    psc.write(desc.pctl_address, 0x80);
    
    // Tick 499 times
    psc.tick(499);
    CHECK((psc.read(desc.pifr_address) & 0x01) == 0); // No PEV yet
    
    // Tick 1 more
    psc.tick(1);
    CHECK((psc.read(desc.pifr_address) & 0x01) != 0); // PEV flag set at wrap
}

TEST_CASE("PSC Fidelity - 12-bit Register Access (TEMP Buffer)") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    // Write L byte of OCR0SA
    psc.write(desc.ocrsa_address, 0xBC);
    // Value in register should still be 0 until H byte is written
    CHECK(psc.read(desc.ocrsa_address) == 0x00);
    
    // Write H byte
    psc.write(desc.ocrsa_address + 1, 0x0A);
    // Now both should be correct (0xABC)
    CHECK(psc.read(desc.ocrsa_address) == 0xBC);
    CHECK(psc.read(desc.ocrsa_address + 1) == 0x0A);
}

TEST_CASE("PSC Fidelity - Interrupt Logic") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    // Configure Period (OCR0RB)
    psc.write(desc.ocrrb_address, 10);
    psc.write(desc.ocrrb_address + 1, 0);
    
    // Enable PSC
    psc.write(desc.pctl_address, 0x80);
    
    // Enable PIE0 (End cycle interrupt)
    psc.write(desc.pim_address, 0x01);
    
    psc.tick(10);
    CHECK((psc.read(desc.pifr_address) & 0x01) != 0);
    
    InterruptRequest req;
    CHECK(psc.pending_interrupt_request(req) == true);
    CHECK(req.vector_index == desc.ec_vector_index);
    
    // Clear flag by writing 1 to PIFR bit 0
    psc.write(desc.pifr_address, 0x01);
    CHECK(psc.pending_interrupt_request(req) == false);
}

TEST_CASE("PSC Fidelity - Center Aligned Mode") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    // Period = 10 (OCR0RB)
    psc.write(desc.ocrrb_address, 10);
    psc.write(desc.ocrrb_address + 1, 0);
    
    // Enable PSC (PRUN=1) and Center-Aligned (PMODE=1) -> 0x90
    psc.write(desc.pctl_address, 0x90);
    
    // Up count to Top (10)
    psc.tick(10);
    CHECK((psc.read(desc.pifr_address) & 0x01) == 0); 
    
    // Down count to Bottom (0)
    psc.tick(10);
    CHECK((psc.read(desc.pifr_address) & 0x01) != 0); // PEV event triggered at bottom
}

TEST_CASE("PSC Fidelity - Analog Comparator Fault Protection") {
    const auto& psc_desc = at90pwm1.pscs[0];
    const auto& ac_desc = at90pwm1.acs[0];
    
    // Setup PSC
    Psc psc {"PSC0", psc_desc};
    psc.reset();
    psc.write(psc_desc.ocrrb_address, 100);
    psc.write(psc_desc.ocrrb_address + 1, 0);
    psc.write(psc_desc.pctl_address, 0x80); // PRUN
    
    // PSOC safe levels: A=Low, B=High
    psc.write(psc_desc.psoc_address, 0x04); 
    
    // Configure Fault Mode: Low Level (mode 3) -> 0x30
    psc.write(psc_desc.pfrc0a_address, 0x30);
    
    // Setup Analog Comparator
    PinMux mux {3};
    AnalogComparator ac {"AC0", ac_desc, mux, 0};
    ac.reset();
    
    // Wire AC to PSC
    ac.connect_psc_fault(psc);
    
    // Enable CAP interrupt
    psc.write(psc_desc.pim_address, 0x08);
    
    // State: AC output 1 (high) -> No fault (mode 3 is low-level)
    ac.set_positive_input_voltage(0.8);
    ac.set_negative_input_voltage(0.5);
    ac.tick(1); 
    
    psc.tick(10);
    CHECK((psc.read(psc_desc.pifr_address) & 0x08) == 0); // No PCAP
    
    // Trigger Fault: AC output 0 (low)
    ac.set_positive_input_voltage(0.3);
    ac.tick(1); // Notify loop happens in evaluate_output
    
    CHECK((psc.read(psc_desc.pifr_address) & 0x08) != 0); // PCAP flag set!
    
    InterruptRequest req;
    CHECK(psc.pending_interrupt_request(req) == true);
    CHECK(req.vector_index == psc_desc.capt_vector_index);
    
    // Verify safe levels
    CHECK(psc.read_output_a() == false);
    CHECK(psc.read_output_b() == true);
}
