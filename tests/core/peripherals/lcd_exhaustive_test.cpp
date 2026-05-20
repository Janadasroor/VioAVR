#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega3290a.hpp"
#include "vioavr/core/devices/atmega169pa.hpp"
#include <map>

using namespace vioavr::core;

struct ExpectedPin {
    u16 port_addr;
    u8 bit;
};

TEST_CASE("LCD Exhaustive Mapping: ATmega3290A (100-pin)") {
    auto desc = devices::atmega3290a;
    Machine machine(desc);
    auto& bus = machine.bus();
    auto& pin_mux = machine.pin_mux();
    
    // Addresses for 3290A
    const u16 PA = 0x22;
    const u16 PC = 0x28;
    const u16 PD = 0x2B;
    const u16 PG = 0x34;
    const u16 PH = 0xDA;
    const u16 PJ = 0xDD;

    std::map<int, ExpectedPin> expected = {
        {0, {PA, 4}}, {1, {PA, 5}}, {2, {PA, 6}}, {3, {PA, 7}},
        {4, {PG, 2}}, {5, {PC, 7}}, {6, {PC, 6}},
        {7, {PH, 3}}, {8, {PH, 2}}, {9, {PH, 1}}, {10, {PH, 0}},
        {11, {PC, 5}}, {12, {PC, 4}}, {13, {PC, 3}}, {14, {PC, 2}},
        {15, {PC, 1}}, {16, {PC, 0}}, {17, {PG, 1}}, {18, {PG, 0}},
        {19, {PD, 7}}, {20, {PD, 6}}, {21, {PD, 5}}, {22, {PD, 4}},
        {23, {PD, 3}}, {24, {PD, 2}}, {25, {PD, 1}}, {26, {PD, 0}},
        {27, {PJ, 6}}, {28, {PJ, 5}}, {29, {PJ, 4}}, {30, {PJ, 3}},
        {31, {PJ, 2}}, {32, {PG, 4}}, {33, {PG, 3}}, {34, {PJ, 1}},
        {35, {PJ, 0}}, {36, {PH, 7}}, {37, {PH, 6}}, {38, {PH, 5}},
        {39, {PH, 4}}
    };

    bus.write_data(0xE5, 0x0F); // LCDPM = 15 (SEG0-39)
    bus.write_data(0xE4, 0x80); // LCDEN

    for (auto const& [seg, pin] : expected) {
        CAPTURE(seg);
        CHECK(pin_mux.get_state_by_address(pin.port_addr, pin.bit).owner == PinOwner::lcd);
    }
    
    // Check Commons
    CHECK(pin_mux.get_state_by_address(PA, 0).owner == PinOwner::lcd); // COM0
    CHECK(pin_mux.get_state_by_address(PA, 1).owner == PinOwner::lcd); // COM1
    CHECK(pin_mux.get_state_by_address(PA, 2).owner == PinOwner::lcd); // COM2
    CHECK(pin_mux.get_state_by_address(PA, 3).owner == PinOwner::lcd); // COM3
}

TEST_CASE("LCD Exhaustive Mapping: ATmega169PA (64-pin)") {
    auto desc = devices::atmega169pa;
    Machine machine(desc);
    auto& bus = machine.bus();
    auto& pin_mux = machine.pin_mux();
    
    // Addresses for 169PA
    const u16 PA = 0x22;
    const u16 PC = 0x28;
    const u16 PD = 0x2B;
    const u16 PG = 0x34;

    std::map<int, ExpectedPin> expected = {
        {0, {PA, 4}}, {1, {PA, 5}}, {2, {PA, 6}}, {3, {PA, 7}},
        {4, {PG, 2}}, {5, {PC, 7}}, {6, {PC, 6}}, {7, {PC, 5}},
        {8, {PC, 4}}, {9, {PC, 3}}, {10, {PC, 2}}, {11, {PC, 1}},
        {12, {PC, 0}}, {13, {PG, 1}}, {14, {PG, 0}}, {15, {PD, 7}},
        {16, {PD, 6}}, {17, {PD, 5}}, {18, {PD, 4}}, {19, {PD, 3}},
        {20, {PD, 2}}, {21, {PD, 1}}, {22, {PD, 0}}, {23, {PG, 4}},
        {24, {PG, 3}}
    };

    bus.write_data(0xE5, 0x07); // LCDPM = 7 (SEG0-24)
    bus.write_data(0xE4, 0x80); // LCDEN

    for (auto const& [seg, pin] : expected) {
        CAPTURE(seg);
        CHECK(pin_mux.get_state_by_address(pin.port_addr, pin.bit).owner == PinOwner::lcd);
    }
}
