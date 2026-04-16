#include <iostream>
#include <vector>
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328.hpp"

constexpr vioavr::core::u16 encode_ldi(vioavr::core::u8 d, vioavr::core::u8 v) {
    return static_cast<vioavr::core::u16>(0xE000U | ((static_cast<vioavr::core::u16>(v & 0xF0U)) << 4U) | (static_cast<vioavr::core::u16>(d - 16U) << 4U) | (v & 0x0FU));
}
constexpr vioavr::core::u16 encode_out(vioavr::core::u8 io, vioavr::core::u8 s) {
    return static_cast<vioavr::core::u16>(0xB800U | ((static_cast<vioavr::core::u16>(s) & 0x1FU) << 4U) | ((static_cast<vioavr::core::u16>(io) & 0x30U) << 5U) | (io & 0x0FU));
}

int main() {
    using namespace vioavr::core;
    using namespace vioavr::core::devices;
    
    constexpr u16 kSei = 0x9478U;
    constexpr u16 kNop = 0x0000U;
    
    MemoryBus bus{atmega328};
    Timer8 timer0 {"TIMER0", atmega328.timers8[0]};
    bus.attach_peripheral(timer0);
    AvrCpu cpu{bus};
    
    std::vector<u16> program = {
        encode_ldi(16, 0x01),  // 0
        encode_out(0x27, 16),  // 1
        encode_ldi(19, 0x02),  // 2
        0x9200 | (19 << 4), atmega328.timers8[0].timsk_address,  // 3,4 STS
        encode_ldi(20, 0x01),  // 5
        encode_out(0x25, 20),  // 6
        kSei,                  // 7
        kNop,                  // 8
        encode_ldi(18, 0x55),  // 9
        kNop, kNop, kNop, kNop,  // 10-13
        encode_ldi(17, 0x77),  // 14
        0x9518                 // 15 RETI
    };
    
    bus.load_flash(program);
    cpu.reset();
    
    std::cout << "After reset: PC=" << cpu.program_counter() << " loaded=" << bus.loaded_program_words() << std::endl;
    
    for (int i = 0; i < 15; ++i) {
        cpu.step();
        auto snap = cpu.snapshot();
        std::cout << "Step " << i+1 << ": PC=" << snap.program_counter 
                  << " SREG=0x" << std::hex << static_cast<int>(snap.sreg) << std::dec
                  << " I=" << ((snap.sreg >> 7) & 1)
                  << " pending=" << snap.interrupt_pending
                  << " in_isr=" << snap.in_interrupt_handler
                  << " state=" << static_cast<int>(snap.state) << std::endl;
    }
    
    return 0;
}
