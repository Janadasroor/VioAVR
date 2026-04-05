#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/devices/atmega328.hpp"

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate)
{
    return static_cast<u16>(
        0xE000U |
        ((static_cast<u16>(immediate & 0xF0U)) << 4U) |
        (static_cast<u16>(destination - 16U) << 4U) |
        (immediate & 0x0FU)
    );
}

}  // namespace

TEST_CASE("Timer16 Input Capture Noise Canceler Fidelity")
{
    using namespace vioavr::core::devices;
    
    MemoryBus bus {atmega328};
    Timer16 timer1 {"TIMER1", atmega328};
    bus.attach_peripheral(timer1);
    AvrCpu cpu {bus};
    GpioPort port {"PORTB", 0x23, 0x24, 0x25};
    timer1.connect_input_capture(port, 0);

    // TCCR1B = 0xC1 (ICNC1 = 1, ICES1 = 1 (Rising), CS10 = 1)
    std::vector<u16> code = {
        encode_ldi(16, 0xC1),
        0x9300, atmega328.timer1.tccrb_address,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    };
    bus.load_flash(code);
    cpu.reset();

    cpu.step(); // LDI
    cpu.step(); // STS TCCR1B (Timer enabled, ticks 2 cycles, current TCNT=2)
    
    CHECK(timer1.counter() == 2);
    
    // Simulate noise (single cycle pulse) - should NOT trigger capture
    port.set_input_levels(0x01); // PINB0 = 1
    cpu.step(); // Step 1: sample 1
    port.set_input_levels(0x00); // PINB0 = 0
    cpu.step(); // Step 2: sample 0 (reset canceler)
    
    CHECK(timer1.input_capture() == 0); 

    // Simulate valid pulse (4+ cycles)
    port.set_input_levels(0x01); // PINB0 = 1
    cpu.step(); // Sample 1: TCNT=5
    cpu.step(); // Sample 2: TCNT=6
    cpu.step(); // Sample 3: TCNT=7
    cpu.step(); // Sample 4: TCNT=8 -> Should trigger capture OF THE TCNT VALUE AT THE 4th SAMPLE
    
    // In actual HW, there's a small latency between sensing and capturing.
    // Our implementation samples at the start of tick(). 
    // At sample 4, current_raw_state == noise_canceler_register_ (1), counter reaches 4, event_triggered = true.
    // handle_input_capture() sets icr_ = tcnt_.
    
    CHECK(timer1.input_capture() == 8);
    CHECK((timer1.read(atmega328.timer1.tifr_address) & 0x20U)); // ICF1 flag should be set
}

TEST_CASE("Timer8 Mode 7 (Fast PWM OCR0A=TOP) Double Buffering Fidelity")
{
    using namespace vioavr::core::devices;
    
    MemoryBus bus {atmega328};
    Timer8 timer0 {"TIMER0", atmega328};
    bus.attach_peripheral(timer0);
    AvrCpu cpu {bus};

    // Fast PWM Mode 7: WGM0[2:0] = 111, TOP = OCR0A
    // TCCR0A = 0x03 (WGM01=1, WGM00=1)
    // TCCR0B = 0x09 (WGM02=1, CS00=1)
    // OCR0A = 10 initial
    std::vector<u16> code = {
        encode_ldi(16, 10),
        0x9300, atmega328.timer0.ocra_address,
        encode_ldi(16, 0x03),
        0x9300, atmega328.timer0.tccra_address,
        encode_ldi(16, 0x09),
        0x9300, atmega328.timer0.tccrb_address, // Timer starts here, ticks 2
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    };
    bus.load_flash(code);
    cpu.reset();

    cpu.step(); cpu.step(); // OCR0A = 10
    cpu.step(); cpu.step(); // TCCR0A
    cpu.step(); cpu.step(); // TCCR0B - TCNT starts at 2
    
    CHECK(timer0.counter() == 2);
    
    // Change OCR0A mid-cycle to 5. 
    // Since we are in PWM mode, it should be BUFFERS and only applied at TOP (10).
    bus.write_data(atmega328.timer0.ocra_address, 5);
    
    // Step until we reach the old TOP (10)
    for (int i = 0; i < 8; ++i) cpu.step(); 
    
    // At TCNT=10, OCR0A should still be 10 (the old top)
    CHECK(timer0.counter() == 10);
    CHECK(timer0.compare_a() == 10);
    
    cpu.step(); // TCNT hits TOP (10), wraps to 0, UPDATES OCR0A from buffer
    CHECK(timer0.counter() == 0);
    CHECK(timer0.compare_a() == 5); // NEW TOP applied
    
    // Next cycle should wrap at 5
    for (int i = 0; i < 5; ++i) cpu.step();
    CHECK(timer0.counter() == 5);
    cpu.step(); // Reach 5, wrap to 0
    CHECK(timer0.counter() == 0);
}
