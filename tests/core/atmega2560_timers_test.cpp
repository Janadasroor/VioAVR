#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/devices/atmega2560.hpp"

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

TEST_CASE("ATmega2560 Multi-Timer Stress Test")
{
    using namespace vioavr::core::devices;
    
    // ATmega2560 has 4 identical 16-bit timers (TC1, TC3, TC4, TC5)
    // We'll create specialized tests for 16-bit timers 3, 4, 5 as they are often
    // the ones where generator bugs or address mapping issues appear.

    MemoryBus bus {atmega2560};
    
    // Setup 4 16-bit timers
    Timer16 t1 {"TIMER1", atmega2560.timers16[0]};
    Timer16 t3 {"TIMER3", atmega2560.timers16[1]};
    Timer16 t4 {"TIMER4", atmega2560.timers16[2]};
    Timer16 t5 {"TIMER5", atmega2560.timers16[3]};

    t1.set_bus(bus); bus.attach_peripheral(t1);
    t3.set_bus(bus); bus.attach_peripheral(t3);
    t4.set_bus(bus); bus.attach_peripheral(t4);
    t5.set_bus(bus); bus.attach_peripheral(t5);

    AvrCpu cpu {bus};

    SUBCASE("Simultaneous CTC Operation on all 16-bit timers") {
        // Set different TOP values for each timer
        // OCR1A = 10, OCR3A = 15, OCR4A = 20, OCR5A = 25
        std::vector<u16> code = {
            // Timer 1 (OCR1A = 10)
            encode_ldi(16, 0), 0x9300, static_cast<u16>(atmega2560.timers16[0].ocra_address + 1),
            encode_ldi(16, 10), 0x9300, static_cast<u16>(atmega2560.timers16[0].ocra_address),
            
            // Timer 3 (OCR3A = 15)
            encode_ldi(16, 0), 0x9300, static_cast<u16>(atmega2560.timers16[1].ocra_address + 1),
            encode_ldi(16, 15), 0x9300, static_cast<u16>(atmega2560.timers16[1].ocra_address),

            // Timer 4 (OCR4A = 20)
            encode_ldi(16, 0), 0x9300, static_cast<u16>(atmega2560.timers16[2].ocra_address + 1),
            encode_ldi(16, 20), 0x9300, static_cast<u16>(atmega2560.timers16[2].ocra_address),

            // Timer 5 (OCR5A = 25)
            encode_ldi(16, 0), 0x9300, static_cast<u16>(atmega2560.timers16[3].ocra_address + 1),
            encode_ldi(16, 25), 0x9300, static_cast<u16>(atmega2560.timers16[3].ocra_address),

            // Enable all timers in CTC mode (WGM=4, CS=1)
            encode_ldi(16, 0x09),
            0x9300, atmega2560.timers16[0].tccrb_address,
            0x9300, atmega2560.timers16[1].tccrb_address,
            0x9300, atmega2560.timers16[2].tccrb_address,
            0x9300, atmega2560.timers16[3].tccrb_address,

            0x0000, 0x0000, 0x0000 // NOPs
        };

        bus.load_flash(code);
        cpu.reset();

        // Run until all timers are enabled
        for (int i = 0; i < 21; ++i) cpu.step();

        // At this point, T1 has ticked 8 cycles, T3 has ticked 6 cycles, T4=4, T5=2
        // because of the sequential enabling instructions.
        
        // Let's just track them for a while
        for (int i = 0; i < 50; ++i) {
            cpu.step();
            // Verify they are within bounds [0, TOP]
            CHECK(t1.counter() <= 10);
            CHECK(t3.counter() <= 15);
            CHECK(t4.counter() <= 20);
            CHECK(t5.counter() <= 25);
        }
        
        // Final sanity check: they should have wrapped multiple times
        // If they didn't wrap, they would be around 50+.
    }
}
