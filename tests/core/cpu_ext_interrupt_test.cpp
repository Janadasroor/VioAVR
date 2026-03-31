#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"

TEST_CASE("External Interrupt Logic Test")
{
    using vioavr::core::ExtInterrupt;
    using vioavr::core::InterruptRequest;
    using vioavr::core::MemoryBus;
    using vioavr::core::devices::atmega328;

    constexpr auto int0_vector = static_cast<vioavr::core::u8>(2U);

    MemoryBus bus {atmega328};
    ExtInterrupt exti {"EXTINT", atmega328, 4U};
    bus.attach_peripheral(exti);
    bus.reset();

    InterruptRequest request {};

    SUBCASE("Falling Edge Trigger") {
        bus.write_data(atmega328.ext_interrupt.eimsk_address, 0x01U); // Enable INT0
        bus.write_data(atmega328.ext_interrupt.eicra_address, 0x02U); // Falling edge (ISC0[1:0] = 10)
        
        exti.set_int0_level(true);
        exti.set_int0_level(false);
        
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == int0_vector);
        CHECK(bus.read_data(atmega328.ext_interrupt.eifr_address) == 0x01U);

        CHECK(bus.consume_interrupt_request(request));
        CHECK(bus.read_data(atmega328.ext_interrupt.eifr_address) == 0x00U);
    }

    SUBCASE("Rising Edge Trigger") {
        bus.write_data(atmega328.ext_interrupt.eimsk_address, 0x01U);
        bus.write_data(atmega328.ext_interrupt.eicra_address, 0x03U); // Rising edge (ISC0[1:0] = 11)
        
        exti.set_int0_level(false);
        exti.set_int0_level(true);
        
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == int0_vector);
    }

    SUBCASE("Clear Flag by Writing 1") {
        bus.write_data(atmega328.ext_interrupt.eicra_address, 0x03U);
        exti.set_int0_level(false);
        exti.set_int0_level(true);
        CHECK((bus.read_data(atmega328.ext_interrupt.eifr_address) & 0x01U) != 0U);

        bus.write_data(atmega328.ext_interrupt.eifr_address, 0x01U); // Write 1 to clear
        CHECK(bus.read_data(atmega328.ext_interrupt.eifr_address) == 0x00U);
        CHECK_FALSE(bus.pending_interrupt_request(request));
    }

    SUBCASE("Low Level Trigger") {
        bus.write_data(atmega328.ext_interrupt.eimsk_address, 0x01U);
        bus.write_data(atmega328.ext_interrupt.eicra_address, 0x00U); // Low level (ISC0[1:0] = 00)
        
        exti.set_int0_level(false);
        bus.tick_peripherals(1U);
        
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == int0_vector);
    }
}
