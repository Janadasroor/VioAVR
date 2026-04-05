#include <iostream>
#include <vector>
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"

constexpr vioavr::core::u16 encode_ldi(vioavr::core::u8 d, vioavr::core::u8 v) {
    return static_cast<vioavr::core::u16>(
        0xE000U |
        ((static_cast<vioavr::core::u16>(v & 0xF0U)) << 4U) |
        (static_cast<vioavr::core::u16>(d - 16U) << 4U) |
        (v & 0x0FU)
    );
}

int main() {
    using namespace vioavr::core;
    using namespace vioavr::core::devices;
    
    constexpr u16 kSei = 0x9478U;
    constexpr u16 kReti = 0x9518U;
    constexpr u16 kNop = 0x0000U;
    
    MemoryBus bus{atmega328};
    Timer8 timer0{"TIMER0", atmega328};
    bus.attach_peripheral(timer0);
    AvrCpu cpu{bus};
    
    std::vector<u16> program = {
        encode_ldi(16, 0x01),  // 0
        0x9200 | (16 << 4), atmega328.timer0.timsk_address,  // 1,2 STS TIMSK
        encode_ldi(17, 0x01),  // 3
        0x9200 | (17 << 4), atmega328.timer0.tcnt_address,  // 4,5 STS TCNT0
        kSei,                  // 6
        kNop,                  // 7
        encode_ldi(18, 0x55),  // 8
        kNop, kNop, kNop,      // 9-11
        encode_ldi(19, 0x77),  // 12
        kReti                  // 13
    };
    
    bus.load_flash(program);
    cpu.reset();
    
    std::cout << "After reset: PC=" << cpu.program_counter() << std::endl;
    
    for (int i = 0; i < 12; ++i) {
        cpu.step();
        auto snap = cpu.snapshot();
        std::cout << "Step " << i+1 << ": PC=" << snap.program_counter 
                  << " SREG=0x" << std::hex << static_cast<int>(snap.sreg) << std::dec
                  << " I=" << ((snap.sreg >> 7) & 1)
                  << " pending=" << snap.interrupt_pending
                  << " in_isr=" << snap.in_interrupt_handler << std::endl;
    }
    
    return 0;
}
