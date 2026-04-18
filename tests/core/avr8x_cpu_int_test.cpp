#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/cpu_int.hpp"
#include <memory>
#include <vector>

using namespace vioavr::core;

struct MockPeripheral : public IoPeripheral {
    u8 vec;
    bool pending = false;
    MockPeripheral(u8 v) : vec(v) {}
    std::string_view name() const noexcept override { return "MOCK"; }
    bool pending_interrupt_request(InterruptRequest& req) const noexcept override {
        if (pending) { req.vector_index = vec; return true; }
        return false;
    }
    bool consume_interrupt_request(InterruptRequest& req) noexcept override {
        if (pending && req.vector_index == vec) {
            pending = false;
            return true;
        }
        return false;
    }
    u8 read(u16) noexcept override { return 0; }
    void write(u16, u8) noexcept override {}
    void reset() noexcept override { pending = false; }
    void tick(u64) noexcept override {}
    std::span<const AddressRange> mapped_ranges() const noexcept override { return {}; }
};

TEST_CASE("Avr8xCpuIntTest - Level 1 Priority Override") {
    auto machine = std::make_unique<Machine>(devices::atmega4809);
    auto& bus = machine->bus();
    auto& cpu = machine->cpu();

    std::vector<u16> prog(512, 0);
    auto write_rjmp = [&](u32 vector_idx, u32 target) {
        prog[vector_idx * (devices::atmega4809.interrupt_vector_size / 2)] = 0x940C;
        prog[vector_idx * (devices::atmega4809.interrupt_vector_size / 2) + 1] = target & 0xFFFFU;
    };
    write_rjmp(0, 0x100); 
    write_rjmp(1, 0x110); 
    write_rjmp(5, 0x150); 
    prog[0x100] = 0x9478; 
    prog[0x101] = 0xCFFF; 
    prog[0x110] = 0x9518; 
    prog[0x150] = 0x9518; 

    bus.load_flash(prog);
    cpu.reset();

    // 1. Set Level 1 High Priority Vector to Index 5 AFTER cpu.reset()
    bus.write_data(0x0113, 5); 

    // Attach mocks
    auto mock1 = std::make_shared<MockPeripheral>(1);
    auto mock5 = std::make_shared<MockPeripheral>(5);
    bus.attach_peripheral(*mock1);
    bus.attach_peripheral(*mock5);

    mock1->pending = true;
    mock5->pending = true;
    cpu.write_sreg(0x80);

    for (int i = 0; i < 200; ++i) {
        machine->step();
        // Vector 5 is word addr 10. PC will be 10 when executing the table entry.
        if (cpu.program_counter() == 10 || cpu.program_counter() == 0x150) break;
        if (cpu.program_counter() == 2 || cpu.program_counter() == 0x110) break; // Should not happen
    }

    if (cpu.program_counter() == 10) machine->step();

    CHECK(cpu.program_counter() == 0x150);
    u8 status = bus.read_data(0x0111);
    CHECK((status & 0x02U) != 0); 
}

TEST_CASE("Avr8xCpuIntTest - Level 0 Status Bits") {
    auto machine = std::make_unique<Machine>(devices::atmega4809);
    auto& bus = machine->bus();
    auto& cpu = machine->cpu();

    machine->reset();
    bus.write_data(0x0113, 0);

    std::vector<u16> prog(512, 0);
    auto write_rjmp = [&](u32 vector_idx, u32 target) {
        prog[vector_idx * 2] = 0x940C;
        prog[vector_idx * 2 + 1] = target & 0xFFFFU;
    };
    write_rjmp(0, 0x100); 
    write_rjmp(1, 0x110); 
    prog[0x100] = 0x9478; 
    prog[0x101] = 0xCFFF; 
    prog[0x110] = 0x9518; 

    bus.load_flash(prog);
    cpu.reset();

    auto mock = std::make_shared<MockPeripheral>(1);
    bus.attach_peripheral(*mock);
    mock->pending = true;
    cpu.write_sreg(0x80);

    for (int i = 0; i < 200; ++i) {
        machine->step();
        if (cpu.program_counter() == 2 || cpu.program_counter() == 0x110) break;
    }

    if (cpu.program_counter() == 2) machine->step();

    CHECK(cpu.program_counter() == 0x110);
    u8 status = bus.read_data(0x0111);
    CHECK((status & 0x01U) != 0); 

    machine->step(); // RETI
    status = bus.read_data(0x0111);
    CHECK((status & 0x01U) == 0);
}
