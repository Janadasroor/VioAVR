#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega328p.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "signal_tracer.hpp"

using namespace vioavr::core;

static constexpr int TOP_VAL    = 799;
static constexpr int DEAD_TIME  = 8;
static constexpr int HALF_TOP   = 400;
static constexpr int OUTA_END   = HALF_TOP - DEAD_TIME;  // 392
static constexpr int OUTB_START = HALF_TOP + DEAD_TIME;  // 408

struct CycleShape { int start_a, end_a, start_b, end_b; };

// Walk one period after a pre-warm that left counter=0.
// Timer16 uses ocra_/ocrb_ (loaded from buffer at overflow) for PWM.
static CycleShape walk_cycle(Timer16& timer, GpioPort& port,
                              vcd::SignalTracer* tracer = nullptr,
                              uint64_t* out_time = nullptr)
{
    int last_a = (port.read(0x23) >> 1) & 1;
    int last_b = (port.read(0x23) >> 2) & 1;
    CycleShape s{-1, -1, -1, -1};
    if (last_a == 1) s.start_a = 0;
    if (last_b == 1) s.start_b = 0;

    for (int c = 0; c < TOP_VAL + 1; ++c) {
        int a = (port.read(0x23) >> 1) & 1;
        int b = (port.read(0x23) >> 2) & 1;

        if (last_a == 0 && a == 1) s.start_a = c;
        if (last_a == 1 && a == 0) s.end_a   = c;
        if (last_b == 0 && b == 1) s.start_b = c;
        if (last_b == 1 && b == 0) s.end_b   = c;

        if (tracer) {
            tracer->record("OutA", *out_time, (bool)a);
            tracer->record("OutB", *out_time, (bool)b);
            (*out_time)++;
        }

        last_a = a; last_b = b;
        timer.tick(1);
    }

    if (s.end_b < 0 && last_b == 1) s.end_b = TOP_VAL + 1;
    if (s.end_a < 0 && last_a == 1) s.end_a = TOP_VAL + 1;
    return s;
}

// Timer16 write expects HIGH byte first, then LOW byte commits 16-bit value.
static void write_16bit(MemoryBus& bus, u16 base, int val) {
    bus.write_data(base + 1, (u8)(val >> 8));
    bus.write_data(base,     (u8)(val));
}

TEST_CASE("ATmega328P Timer1 Fast PWM — initial waveform and dead time")
{
    auto& dev = devices::atmega328p;
    PinMux pm{10};
    GpioPort port{"PORTB", 0x23, 0x24, 0x25, pm};
    Timer16 timer{"TIMER1", dev.timers16[0], &pm};
    MemoryBus bus{dev};
    timer.set_bus(bus);
    bus.attach_peripheral(timer);
    bus.attach_peripheral(port);
    timer.connect_compare_output_a(port, 1);
    timer.connect_compare_output_b(port, 2);
    bus.reset();
    port.write(0x24, 0x06);  // DDRB PB1,PB2 as output

    // Fast PWM Mode 14 (ICR1=TOP):
    //   COM1A=2 (non-inverting)  — OutA HIGH from BOTTOM to OCR1A
    //   COM1B=3 (inverting)      — OutB HIGH from OCR1B to TOP
    bus.write_data(dev.timers16[0].tccra_address, 0xB2U); // COM1A=2, COM1B=3, WGM11=1
    write_16bit(bus, dev.timers16[0].icr_address,     TOP_VAL);
    write_16bit(bus, dev.timers16[0].ocra_address,    OUTA_END);
    write_16bit(bus, dev.timers16[0].ocrb_address,    OUTB_START);
    bus.write_data(dev.timers16[0].tccrb_address, 0x19U); // WGM13=1, WGM12=1, CS10=1

    // Pre-warm one full period: loads ocra_/ocrb_ from buffer at overflow.
    timer.tick(TOP_VAL + 1);

    CHECK_MESSAGE(timer.counter() == 0, "Counter synchronized to 0 after pre-warm");

    uint64_t t = 0;
    vcd::SignalTracer tracer;
    tracer.add_signal("OutA");
    tracer.add_signal("OutB");

    CycleShape s = walk_cycle(timer, port, &tracer, &t);
    tracer.dump("atmega328p_pwm.vcd");

    CHECK_MESSAGE(s.start_a == 0,
                  "OutA rise at " << s.start_a << " (expected 0)");
    CHECK_MESSAGE(s.end_a == OUTA_END,
                  "OutA fall at " << s.end_a << " (expected " << OUTA_END << ")");
    CHECK_MESSAGE(s.start_b == OUTB_START,
                  "OutB rise at " << s.start_b << " (expected " << OUTB_START << ")");
    CHECK_MESSAGE(s.end_b == TOP_VAL + 1,
                  "OutB fall at " << s.end_b << " (expected " << (TOP_VAL + 1) << ")");

    int dead = s.start_b - s.end_a;
    CHECK_MESSAGE(dead == 2 * DEAD_TIME,
                  "Dead time: " << dead << " cycles (expected " << (2 * DEAD_TIME) << ")");

    int on_a = s.end_a - s.start_a;
    int on_b = s.end_b - s.start_b;
    CHECK_MESSAGE(on_a == OUTA_END, "OutA width: " << on_a << " (expected " << OUTA_END << ")");
    CHECK_MESSAGE(on_b == (TOP_VAL + 1) - OUTB_START,
                  "OutB width: " << on_b << " (expected " << ((TOP_VAL + 1) - OUTB_START) << ")");
}

TEST_CASE("ATmega328P Timer1 Fast PWM — duty cycle modulation")
{
    auto& dev = devices::atmega328p;
    PinMux pm{10};
    GpioPort port{"PORTB", 0x23, 0x24, 0x25, pm};
    Timer16 timer{"TIMER1", dev.timers16[0], &pm};
    MemoryBus bus{dev};
    timer.set_bus(bus);
    bus.attach_peripheral(timer);
    bus.attach_peripheral(port);
    timer.connect_compare_output_a(port, 1);
    timer.connect_compare_output_b(port, 2);
    bus.reset();
    port.write(0x24, 0x06);

    bus.write_data(dev.timers16[0].tccra_address, 0xB2U);
    write_16bit(bus, dev.timers16[0].icr_address,  TOP_VAL);
    write_16bit(bus, dev.timers16[0].ocrb_address, OUTB_START);

    auto write_ocra = [&](int val) {
        write_16bit(bus, dev.timers16[0].ocra_address, val);
    };

    // Start timer (OCR1A=0 from reset, 0% duty)
    bus.write_data(dev.timers16[0].tccrb_address, 0x19U);

    auto set_duty = [&](int pct) -> int {
        int pw = pct * OUTA_END / 100;
        write_ocra(pw);
        return pw;
    };

    struct Case { const char* label; int pct; int exp_pw; };
    Case cases[] = {
        {"50%",  50, 196},
        {"25%",  25,  98},
        {"12.5%", 12, 47},
    };

    for (auto& c : cases) {
        int pw = set_duty(c.pct);
        CHECK_MESSAGE(pw == c.exp_pw, c.label << " pulse width = " << pw << " (expected " << c.exp_pw << ")");

        // Drain one period to load new ocra_ from buffer at overflow
        timer.tick(TOP_VAL + 1);
        CHECK_MESSAGE(timer.counter() == 0, c.label << " counter=0 after drain");

        CycleShape s = walk_cycle(timer, port);

        CHECK_MESSAGE(s.start_a == 0, c.label << " OutA rise at " << s.start_a << " (expected 0)");
        CHECK_MESSAGE(s.end_a == pw, c.label << " OutA fall at " << s.end_a << " (expected " << pw << ")");
        CHECK_MESSAGE(s.start_b == OUTB_START, c.label << " OutB rise at " << s.start_b << " (expected " << OUTB_START << ")");
        CHECK_MESSAGE(s.end_b == TOP_VAL + 1, c.label << " OutB fall at " << s.end_b << " (expected " << (TOP_VAL + 1) << ")");

        int on_a = s.end_a - s.start_a;
        int on_b = s.end_b - s.start_b;
        CHECK_MESSAGE(on_a == pw, c.label << " OutA width " << on_a << " (expected " << pw << ")");
        CHECK_MESSAGE(on_b == (TOP_VAL + 1) - OUTB_START, c.label << " OutB width " << on_b << " (expected " << ((TOP_VAL + 1) - OUTB_START) << " fixed)");
    }
}
