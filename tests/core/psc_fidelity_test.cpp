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
    
    // Enable PSC (PRUN = 1)
    psc.write(desc.pctl_address, 0x80);
    
    psc.tick(500);
    CHECK((psc.read(desc.pifr_address) & 0x01) != 0); 
}

TEST_CASE("PSC Fidelity - 12-bit Register Access (TEMP Buffer)") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    psc.write(desc.ocrsa_address, 0xBC);
    CHECK(psc.read(desc.ocrsa_address) == 0x00);
    
    psc.write(desc.ocrsa_address + 1, 0x0A);
    CHECK(psc.read(desc.ocrsa_address) == 0xBC);
    CHECK(psc.read(desc.ocrsa_address + 1) == 0x0A);
}

TEST_CASE("PSC Fidelity - Interrupt Logic") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    psc.write(desc.ocrrb_address, 10);
    psc.write(desc.ocrrb_address + 1, 0);
    psc.write(desc.pctl_address, 0x80);
    psc.write(desc.pim_address, 0x01);
    
    psc.tick(10);
    InterruptRequest req;
    CHECK(psc.pending_interrupt_request(req) == true);
    CHECK(req.vector_index == desc.ec_vector_index);
}

TEST_CASE("PSC Fidelity - Center Aligned Mode") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    psc.write(desc.ocrrb_address, 10);
    psc.write(desc.ocrrb_address + 1, 0);
    psc.write(desc.pctl_address, 0x90); // PMODE=1
    
    psc.tick(10);
    CHECK((psc.read(desc.pifr_address) & 0x01) == 0); 
    psc.tick(10);
    CHECK((psc.read(desc.pifr_address) & 0x01) != 0);
}

TEST_CASE("PSC Fidelity - Analog Comparator Fault Protection") {
    const auto& psc_desc = at90pwm1.pscs[0];
    const auto& ac_desc = at90pwm1.acs[0];
    
    Psc psc {"PSC0", psc_desc};
    psc.reset();
    psc.write(psc_desc.ocrrb_address, 100);
    psc.write(psc_desc.ocrrb_address + 1, 0);
    psc.write(psc_desc.pctl_address, 0x80); 
    
    psc.write(psc_desc.psoc_address, 0x0C); 
    psc.write(psc_desc.pfrc0a_address, 0x30); 
    
    PinMux mux {3};
    AnalogComparator ac {"AC0", ac_desc, mux, 0};
    ac.reset();
    ac.connect_psc_fault(psc);
    
    ac.set_positive_input_voltage(0.8);
    ac.set_negative_input_voltage(0.5);
    ac.tick(1); 
    
    ac.set_positive_input_voltage(0.3);
    ac.tick(1);
    
    CHECK((psc.read(psc_desc.pifr_address) & 0x08) != 0); 
    CHECK(psc.read_output_b() == true);
}

TEST_CASE("PSC Fidelity - Output Polarity") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    psc.write(desc.psoc_address, 0x0D);
    psc.write(desc.ocrra_address, 5);
    psc.write(desc.ocrra_address + 1, 0);
    psc.write(desc.ocrrb_address, 10);
    psc.write(desc.ocrrb_address + 1, 0);
    psc.write(desc.pctl_address, 0x80); 
    
    CHECK(psc.read_output_a() == true);
    CHECK(psc.read_output_b() == false);
}
