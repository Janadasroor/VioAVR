#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"

TEST_CASE("Analog Comparator Functional Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    constexpr u16 acsr_addr = 0x50U;
    constexpr u8 comparator_vector = 8U;

    MemoryBus bus {atmega328};
    AnalogComparator comparator {"AC", acsr_addr, comparator_vector, 9U, 0.01};
    bus.attach_peripheral(comparator);
    bus.reset();

    SUBCASE("Voltage Comparison and ACSR Status") {
        // IE=1, ACIS=11 (Rising edge)
        bus.write_data(acsr_addr, 0x0BU);
        
        comparator.set_negative_input_voltage(0.80);
        comparator.set_positive_input_voltage(0.20);
        
        // ACO (bit 5) should be 0 (Positive < Negative)
        CHECK((bus.read_data(acsr_addr) & 0x20U) == 0U);

        // Positive 0.79 < Negative 0.80 -> ACO=0
        comparator.set_positive_input_voltage(0.79);
        CHECK((bus.read_data(acsr_addr) & 0x20U) == 0U);
        CHECK((bus.read_data(acsr_addr) & 0x10U) == 0U); // ACI (bit 4) should be 0

        // Positive 0.83 > Negative 0.80 -> ACO=1, ACI=1 (Rising edge)
        comparator.set_positive_input_voltage(0.83);
        CHECK((bus.read_data(acsr_addr) & 0x30U) == 0x30U);
    }

    SUBCASE("Interrupt Generation and Consumption") {
        bus.write_data(acsr_addr, 0x0BU);
        comparator.set_negative_input_voltage(0.80);
        comparator.set_positive_input_voltage(0.83);
        
        InterruptRequest request {};
        CHECK(bus.pending_interrupt_request(request));
        CHECK(request.vector_index == comparator_vector);

        // Consuming interrupt should clear ACI
        CHECK(bus.consume_interrupt_request(request));
        CHECK((bus.read_data(acsr_addr) & 0x10U) == 0U);
    }

    SUBCASE("Comparator Disable (ACD)") {
        // ACD=1
        bus.write_data(acsr_addr, 0x8BU);
        comparator.set_negative_input_voltage(0.90);
        comparator.set_positive_input_voltage(1.00);
        
        InterruptRequest request {};
        CHECK_FALSE(bus.pending_interrupt_request(request));
    }
}
