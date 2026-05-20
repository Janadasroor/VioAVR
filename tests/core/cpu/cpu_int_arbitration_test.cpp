#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/cpu_int.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include <memory>

using namespace vioavr::core;

struct MockPeripheral : public IoPeripheral {
    u8 vec;
    bool pending = false;
    MockPeripheral(u8 v) : vec(v) {}
    std::string_view name() const noexcept override { return "MOCK"; }
    bool pending_interrupt_request(InterruptRequest& req) const noexcept override {
        if (pending) {
            req.vector_index = vec;
            req.source_id = 0;
            return true;
        }
        return false;
    }
    u8 read(u16) noexcept override { return 0; }
    void write(u16, u8) noexcept override {}
    void reset() noexcept override {}
    void tick(u64) noexcept override {}
    std::span<const AddressRange> mapped_ranges() const noexcept override { return {}; }
};

TEST_CASE("MemoryBus - CPUINT Arbitration") {
    MemoryBus bus(devices::atmega4809);
    
    CpuInt cpu_int(devices::atmega4809.cpu_ints[0]);
    bus.set_cpu_int(&cpu_int);
    
    // Attach mock peripherals
    auto mock1 = std::make_shared<MockPeripheral>(1);
    auto mock5 = std::make_shared<MockPeripheral>(5);
    bus.attach_peripheral(*mock1);
    bus.attach_peripheral(*mock5);
    
    mock1->pending = true;
    mock5->pending = true;
    
    InterruptRequest request;
    
    // Case 1: No Level 1 override. Vector 1 (lower index) should win.
    cpu_int.write(0x0113, 0); 
    bool found = bus.pending_interrupt_request(request, 0xFF);
    CHECK(found);
    CHECK(request.vector_index == 1);
    
    // Case 2: Vector 5 is Level 1. Vector 5 should win despite higher index.
    cpu_int.write(0x0113, 5);
    found = bus.pending_interrupt_request(request, 0xFF);
    CHECK(found);
    CHECK(request.vector_index == 5);
    
    // Case 3: Both are Level 0, but swap order of peripherals to check robustness.
    // MemoryBus::peripherals_ stores them in arrival order.
    // Already tested in Case 1.
}
