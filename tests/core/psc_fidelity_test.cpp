#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/psc.hpp"
#include "vioavr/core/devices/at90pwm1.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("PSC Fidelity - Basic PWM Cycle") {
    const auto& desc = at90pwm1.pscs[0];
    Psc psc {"PSC0", desc};
    psc.reset();
    
    // Set OCR0RA = 500 for period
    // Write 500 (0x1F4)
    psc.write(desc.ocrra_address, 0xF4);
    psc.write(desc.ocrra_address + 1, 0x01);
    
    CHECK(psc.read(desc.ocrra_address) == 0xF4);
    CHECK(psc.read(desc.ocrra_address + 1) == 0x01);

    // Enable PSC (PRUN = 1)
    psc.write(desc.pctl_address, 0x80);
    
    // Tick 499 times
    psc.tick(499);
    CHECK((psc.read(desc.pifr_address) & 0x01) == 0); // No PEV yet
    
    // Tick 1 more
    psc.tick(1);
    CHECK((psc.read(desc.pifr_address) & 0x01) != 0); // PEV flag set at 500
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
    
    // Configure Period
    psc.write(desc.ocrra_address, 10);
    psc.write(desc.ocrra_address + 1, 0);
    
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
