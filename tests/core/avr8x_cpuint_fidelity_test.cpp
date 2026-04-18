#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/cpu_int.hpp"
#include "vioavr/core/timer8.hpp" // We can use Timer8 for old chips, or TCA for 4809
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X CPUINT Fidelity Test") {
    auto device = &devices::atmega4809;
    Machine machine(*device);
    auto& bus = machine.bus();

    const u16 CPUINT_BASE = 0x0110;
    const u16 CPUINT_CTRLA = CPUINT_BASE + 0x00;
    const u16 CPUINT_STATUS = CPUINT_BASE + 0x01;
    const u16 CPUINT_LVL0PRI = CPUINT_BASE + 0x02;
    const u16 CPUINT_LVL1VEC = CPUINT_BASE + 0x03;

    // Use two vector indices: 20 (TCA0_OVF) and 27 (ADC0_RESRDY)
    // In legacy mode, 20 wins over 27.
    
    // We need a way to trigger these interrupts.
    // Since we are testing internal priority logic of CPUINT/MemoryBus, 
    // we can use a small DummyPeripheral.

    class MockPeripheral : public IoPeripheral {
    public:
        MockPeripheral(std::string name, u8 vector) : name_(name), vector_(vector) {}
        std::string_view name() const noexcept override { return name_; }
        u8 read(u16) noexcept override { return 0; }
        void write(u16, u8) noexcept override {}
        void tick(u64) noexcept override {}
        void reset() noexcept override { pending_ = false; }
        
        std::span<const AddressRange> mapped_ranges() const noexcept override { return {}; }
        
        bool pending_interrupt_request(InterruptRequest& request) const noexcept override {
            if (pending_) {
                request.vector_index = vector_;
                request.source_id = 0;
                return true;
            }
            return false;
        }
        
        bool consume_interrupt_request(InterruptRequest& request) noexcept override {
            if (pending_ && request.vector_index == vector_) {
                pending_ = false;
                return true;
            }
            return false;
        }

        void trigger() { pending_ = true; }

    private:
        std::string name_;
        u8 vector_;
        bool pending_ = false;
    };

    MockPeripheral low_vec("LOW_VEC", 20);
    MockPeripheral high_vec("HIGH_VEC", 27);
    bus.attach_peripheral(low_vec);
    bus.attach_peripheral(high_vec);

    SUBCASE("Default Priority (Lowest Index Wins)") {
        low_vec.trigger();
        high_vec.trigger();
        
        InterruptRequest req;
        bool found = bus.pending_interrupt_request(req, 0xFF);
        CHECK(found);
        CHECK(req.vector_index == 20); // 20 wins
    }

    SUBCASE("Level 1 Promotion") {
        // Promote 27 to Level 1
        bus.write_data(CPUINT_LVL1VEC, 27);
        
        low_vec.trigger();
        high_vec.trigger();
        
        InterruptRequest req;
        bool found = bus.pending_interrupt_request(req, 0xFF);
        CHECK(found);
        CHECK(req.vector_index == 27); // 27 wins because it is Level 1
    }

    SUBCASE("Level 0 Round-Robin") {
        // Enable Round-Robin
        bus.write_data(CPUINT_CTRLA, 0x01); // LVL0RR=1
        
        // Initial state: Vector 20 wins over 27
        low_vec.trigger();
        high_vec.trigger();
        
        InterruptRequest req;
        bool consumed = bus.consume_interrupt_request(req, 0xFF);
        CHECK(consumed);
        CHECK(req.vector_index == 20);
        
        // ...
        low_vec.trigger();
        high_vec.trigger();
        
        consumed = bus.consume_interrupt_request(req, 0xFF);
        CHECK(consumed);
        CHECK(req.vector_index == 27);
    }

    SUBCASE("Static Priority Offset (LVL0PRI)") {
        // Set priority base to 25
        bus.write_data(CPUINT_LVL0PRI, 25);
        
        // Vectors: 20, 27
        // Relative to 25:
        // 20 -> (20 - 25 + 43) % 43 = 38
        // 27 -> (27 - 25 + 43) % 43 = 2
        // 2 wins over 38. So 27 wins.
        
        low_vec.trigger();
        high_vec.trigger();
        
        InterruptRequest req;
        bool consumed = bus.consume_interrupt_request(req, 0xFF);
        CHECK(consumed);
        CHECK(req.vector_index == 27);
    }
}
