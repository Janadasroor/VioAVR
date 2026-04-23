#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/uart8x.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X: USART0 STARTEI Fidelity Test") {
    Machine machine(devices::atmega4809);
    auto& bus = machine.bus();
    
    // USART0 Base: 0x0800
    // EVSYS Base: 0x0180
    
    // 1. Configure USART0
    // CTRLB: TXEN=1
    bus.write_data(0x0806, 0x40); 
    // EVCTRL: STARTEI=1
    bus.write_data(0x080A, 0x01);
    
    // 2. Configure EVSYS Channel 0 for Software Trigger
    // CHANNEL0 (0x190) = 0x01 (Software Strobe 0 / Manual?)
    // Actually, let's use a software strobe.
    // Manual says STROBE 0 is ID 1.
    bus.write_data(0x0190, 0x01); 
    // User USERUSART0 (0x1AF) -> Channel 0
    bus.write_data(0x01AF, 0x01);
    
    // 3. Write data to TXDATA
    bus.write_data(0x0802, 'A');
    
    // Tick peripherals - should NOT start yet
    bus.tick_peripherals(100);
    
    auto uarts = machine.peripherals_of_type<Uart8x>();
    auto* uart0 = uarts[0];
    u16 tx_data;
    CHECK_FALSE(uart0->consume_transmitted_byte(tx_data)); // Nothing sent yet
    
    // 4. Trigger software event on Channel 0
    // STROBE (0x180) bit 0 = 1
    bus.write_data(0x0180, 0x01);
    
    // Tick peripherals - should start and finish
    bus.tick_peripherals(2000); // Plenty of time for 'A' at default baud
    
    CHECK(uart0->consume_transmitted_byte(tx_data));
    CHECK((tx_data & 0xFF) == 'A');
}
