#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328.hpp"

TEST_CASE("Timer0 Dual Interrupt (Compare and Overflow) Test")
{
    using vioavr::core::InterruptRequest;
    using vioavr::core::MemoryBus;
    using vioavr::core::Timer8;
    using vioavr::core::devices::atmega328;

    MemoryBus bus {atmega328};
    Timer8 timer0 {"TIMER0", atmega328};
    bus.attach_peripheral(timer0);
    bus.reset();

    SUBCASE("Simultaneous trigger") {
        bus.write_data(atmega328.timer0.ocra_address, 0x00U);
        bus.write_data(atmega328.timer0.tcnt_address, 0xFFU);
        bus.write_data(atmega328.timer0.timsk_address, 0x03U); // Enable both OCIE0A and TOIE0
        
        bus.write_data(atmega328.timer0.tccrb_address, 0x01U); // Start clk/1
        bus.tick_peripherals(1U);

        CHECK(timer0.counter() == 0x00U);
        CHECK(timer0.interrupt_flags() == 0x03U); // Both OCF0A and TOV0 set

        InterruptRequest request {};
        
        // Priority order: Compare Match usually has higher priority (lower vector number) than Overflow
        // For Timer0 on MEGA328: Vector 14 (COMPA), Vector 16 (OVF)
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == atmega328.timer0.compare_a_vector_index);

        // Consume Compare Match
        CHECK(bus.consume_interrupt_request(request));
        CHECK(request.vector_index == atmega328.timer0.compare_a_vector_index);
        CHECK(timer0.interrupt_flags() == 0x01U); // Only TOV0 left

        // Verify Overflow is still pending
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == atmega328.timer0.overflow_vector_index);

        // Consume Overflow
        CHECK(bus.consume_interrupt_request(request));
        CHECK(timer0.interrupt_flags() == 0x00U);
    }
}
