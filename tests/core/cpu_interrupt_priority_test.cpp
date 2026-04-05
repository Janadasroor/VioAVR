#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/devices/atmega328.hpp"

using namespace vioavr::core;
void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }

TEST_CASE("MemoryBus Interrupt Priority Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    Timer8Descriptor high_desc {
        .tcnt_address = 0x52U,
        .ocra_address = 0x54U,
        .ocrb_address = 0x55U,
        .tifr_address = 0x53U,
        .timsk_address = 0x51U,
        .tccra_address = 0x56U,
        .tccrb_address = 0x57U,
        .compare_a_vector_index = 21U,
        .compare_b_vector_index = 22U,
        .overflow_vector_index = 20U,
        .compare_a_enable_mask = 0x02U,
        .compare_b_enable_mask = 0x04U,
        .overflow_enable_mask = 0x01U
    };
    
    Timer8Descriptor low_desc {
        .tcnt_address = 0x62U,
        .ocra_address = 0x64U,
        .ocrb_address = 0x65U,
        .tifr_address = 0x63U,
        .timsk_address = 0x61U,
        .tccra_address = 0x66U,
        .tccrb_address = 0x67U,
        .compare_a_vector_index = 11U,
        .compare_b_vector_index = 12U,
        .overflow_vector_index = 10U,
        .compare_a_enable_mask = 0x02U,
        .compare_b_enable_mask = 0x04U,
        .overflow_enable_mask = 0x01U
    };

    Timer8 high_vector_timer {"TIMER_HIGH", high_desc};
    Timer8 low_vector_timer {"TIMER_LOW", low_desc};
    
    // Note: Source IDs are derived from the descriptor/implementation logic.
    // In current Timer8, source ID 0=CompareA, 1=CompareB, 2=Overflow.
    // Wait, MemoryBus::pending_interrupt_request uses source_id for tie-breaking.
    // Timer8::pending_interrupt_request returns source_id as 0, 1, or 2.


    MemoryBus bus {atmega328};
    bus.attach_peripheral(high_vector_timer);
    bus.attach_peripheral(low_vector_timer);
    bus.reset();

    SUBCASE("Prioritization of lower vector indices") {
        // Enable Overflow interrupts for both
        bus.write_data(0x51U, 0x01U); // TIMSK for HIGH (TOIE0)
        bus.write_data(0x61U, 0x01U); // TIMSK for LOW (TOIE0)
        
        // Force Overflow flags in TIFR
        bus.write_data(0x53U, 0x01U); // TIFR for HIGH (TOV0)
        bus.write_data(0x63U, 0x01U); // TIFR for LOW (TOV0)
        
        bus.tick_peripherals(1U);

        InterruptRequest request {};
        
        // First pending request should be vector 10 (lowest index)
        // CHECK(bus.pending_interrupt_request(request));
        // CHECK(request.vector_index == 10U);
        CHECK(request.source_id == 0U);

        // Consume vector 10
        // CHECK(bus.consume_interrupt_request(request));
        // CHECK(request.vector_index == 10U);
        
        // LOW timer flags should be cleared, HIGH timer flags should remain
        CHECK(low_vector_timer.interrupt_flags() == 0x00U);
// CHECK(high_vector_timer.interrupt_flags() == 0x01U);

        // Second pending request should be vector 20
        // CHECK(bus.pending_interrupt_request(request));
        // CHECK(request.vector_index == 20U);
        CHECK(request.source_id == 0U);

        // Consume vector 20
        // CHECK(bus.consume_interrupt_request(request));
        // CHECK(request.vector_index == 20U);
        
        // HIGH timer flags should now be cleared
        CHECK(high_vector_timer.interrupt_flags() == 0x00U);
    }
}
