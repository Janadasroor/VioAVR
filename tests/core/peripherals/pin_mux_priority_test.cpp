#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/pin_mux.hpp"
#include <iostream>

using namespace vioavr::core;

TEST_CASE("PinMux: Priority Hierarchy Validation") {
    PinMux pm(1); // One port
    
    // Default state
    auto state = pm.get_state(0, 0);
    CHECK(state.owner == PinOwner::gpio);
    
    // 1. Lower priority claim: ADC
    pm.claim_pin(0, 0, PinOwner::adc);
    state = pm.get_state(0, 0);
    CHECK(state.owner == PinOwner::adc);
    
    // 2. Higher priority claim: UART
    pm.claim_pin(0, 0, PinOwner::uart);
    state = pm.get_state(0, 0);
    CHECK(state.owner == PinOwner::uart);
    
    // 3. Even higher priority claim: SPI
    pm.claim_pin(0, 0, PinOwner::spi);
    state = pm.get_state(0, 0);
    CHECK(state.owner == PinOwner::spi);
    
    // 4. Reset claim (Highest priority)
    pm.claim_pin(0, 0, PinOwner::reset);
    state = pm.get_state(0, 0);
    CHECK(state.owner == PinOwner::reset);
    
    // 5. Release Reset -> returns to SPI
    pm.release_pin(0, 0, PinOwner::reset);
    state = pm.get_state(0, 0);
    CHECK(state.owner == PinOwner::spi);
    
    // 6. Release SPI -> returns to UART
    pm.release_pin(0, 0, PinOwner::spi);
    state = pm.get_state(0, 0);
    CHECK(state.owner == PinOwner::uart);
    
    // 7. Release ADC (while UART still owns it)
    pm.release_pin(0, 0, PinOwner::adc);
    state = pm.get_state(0, 0);
    CHECK(state.owner == PinOwner::uart); // still UART
    
    // 8. Release UART -> returns to GPIO
    pm.release_pin(0, 0, PinOwner::uart);
    state = pm.get_state(0, 0);
    CHECK(state.owner == PinOwner::gpio);
}

TEST_CASE("PinMux: Simultaneous Claims") {
    PinMux pm(1);
    
    // Claim multiple at once
    pm.claim_pin(0, 5, PinOwner::adc);
    pm.claim_pin(0, 5, PinOwner::uart);
    pm.claim_pin(0, 5, PinOwner::timer);
    
    // Highest should be UART (4) > Timer (3) > ADC (1)
    CHECK(pm.get_state(0, 5).owner == PinOwner::uart);
    
    pm.release_pin(0, 5, PinOwner::uart);
    CHECK(pm.get_state(0, 5).owner == PinOwner::timer);
    
    pm.release_pin(0, 5, PinOwner::timer);
    CHECK(pm.get_state(0, 5).owner == PinOwner::adc);
}
