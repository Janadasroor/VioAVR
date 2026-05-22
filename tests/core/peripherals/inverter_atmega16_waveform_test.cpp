#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega16.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/gpio_port.hpp"

using namespace vioavr::core;

static constexpr int TOP_VAL    = 799;
static constexpr int DEAD_TIME  = 8;
static constexpr int HALF_TOP   = 400;
static constexpr int OUTA_END   = HALF_TOP - DEAD_TIME;  // 392
static constexpr int OUTB_START = HALF_TOP + DEAD_TIME;  // 408

struct CycleShape { int start_a, end_a, start_b, end_b; };

static CycleShape walk_cycle(Timer16& timer, GpioPort& port)
{
    // PORTD read at 0x30 (PIND), OC1A=PD5, OC1B=PD4
    int last_a = (port.read(0x30) >> 5) & 1;
    int last_b = (port.read(0x30) >> 4) & 1;
    CycleShape s{-1, -1, -1, -1};
    if (last_a == 1) s.start_a = 0;
    if (last_b == 1) s.start_b = 0;

    for (int c = 0; c < TOP_VAL + 1; ++c) {
        int a = (port.read(0x30) >> 5) & 1;
        int b = (port.read(0x30) >> 4) & 1;

        if (last_a == 0 && a == 1) s.start_a = c;
        if (last_a == 1 && a == 0) s.end_a   = c;
        if (last_b == 0 && b == 1) s.start_b = c;
        if (last_b == 1 && b == 0) s.end_b   = c;

        last_a = a; last_b = b;
        timer.tick(1);
    }

    if (s.end_b < 0 && last_b == 1) s.end_b = TOP_VAL + 1;
    if (s.end_a < 0 && last_a == 1) s.end_a = TOP_VAL + 1;
    return s;
}

static void write_16bit(MemoryBus& bus, u16 base, int val) {
    bus.write_data(base + 1, (u8)(val >> 8));
    bus.write_data(base,     (u8)(val));
}

TEST_CASE("ATmega16 Timer1 Fast PWM — initial waveform and dead time")
{
    auto& dev = devices::atmega16;
    PinMux pm{10};
    // PORTD: PIN=0x30, DDR=0x31, PORT=0x32
    GpioPort port{"PORTD", 0x30, 0x31, 0x32, pm};
    Timer16 timer{"TIMER1", dev.timers16[0], &pm};
    MemoryBus bus{dev};
    timer.set_bus(bus);
    bus.attach_peripheral(timer);
    bus.attach_peripheral(port);
    // OC1A=PD5, OC1B=PD4
    timer.connect_compare_output_a(port, 5);
    timer.connect_compare_output_b(port, 4);
    bus.reset();
    port.write(0x31, 0x30);  // DDRD PD5,PD4 as output

    // Fast PWM Mode 14 (ICR1=TOP):
    //   COM1A=2 (non-inverting), COM1B=3 (inverting)
    bus.write_data(dev.timers16[0].tccra_address, 0xB2U);
    write_16bit(bus, dev.timers16[0].icr_address,     TOP_VAL);
    write_16bit(bus, dev.timers16[0].ocra_address,    OUTA_END);
    write_16bit(bus, dev.timers16[0].ocrb_address,    OUTB_START);
    bus.write_data(dev.timers16[0].tccrb_address, 0x19U);

    // Pre-warm one full period
    timer.tick(TOP_VAL + 1);
    CHECK(timer.counter() == 0);

    CycleShape s = walk_cycle(timer, port);

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
    CHECK_MESSAGE(on_a == OUTA_END,
                  "OutA width: " << on_a << " (expected " << OUTA_END << ")");
    CHECK_MESSAGE(on_b == (TOP_VAL + 1) - OUTB_START,
                  "OutB width: " << on_b << " (expected " << ((TOP_VAL + 1) - OUTB_START) << ")");
}

TEST_CASE("ATmega16 Timer1 Fast PWM — duty cycle modulation")
{
    auto& dev = devices::atmega16;
    PinMux pm{10};
    GpioPort port{"PORTD", 0x30, 0x31, 0x32, pm};
    Timer16 timer{"TIMER1", dev.timers16[0], &pm};
    MemoryBus bus{dev};
    timer.set_bus(bus);
    bus.attach_peripheral(timer);
    bus.attach_peripheral(port);
    timer.connect_compare_output_a(port, 5);
    timer.connect_compare_output_b(port, 4);
    bus.reset();
    port.write(0x31, 0x30);

    bus.write_data(dev.timers16[0].tccra_address, 0xB2U);
    write_16bit(bus, dev.timers16[0].icr_address,  TOP_VAL);
    write_16bit(bus, dev.timers16[0].ocrb_address, OUTB_START);

    auto write_ocra = [&](int val) {
        write_16bit(bus, dev.timers16[0].ocra_address, val);
    };

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
        CHECK_MESSAGE(pw == c.exp_pw,
                      c.label << " pulse width = " << pw
                      << " (expected " << c.exp_pw << ")");

        timer.tick(TOP_VAL + 1);
        CHECK_MESSAGE(timer.counter() == 0,
                      c.label << " counter=0 after drain");

        CycleShape s = walk_cycle(timer, port);

        CHECK_MESSAGE(s.start_a == 0,
                      c.label << " OutA rise at " << s.start_a << " (expected 0)");
        CHECK_MESSAGE(s.end_a == pw,
                      c.label << " OutA fall at " << s.end_a << " (expected " << pw << ")");
        CHECK_MESSAGE(s.start_b == OUTB_START,
                      c.label << " OutB rise at " << s.start_b << " (expected " << OUTB_START << ")");
        CHECK_MESSAGE(s.end_b == TOP_VAL + 1,
                      c.label << " OutB fall at " << s.end_b << " (expected " << (TOP_VAL + 1) << ")");

        int on_a = s.end_a - s.start_a;
        int on_b = s.end_b - s.start_b;
        CHECK_MESSAGE(on_a == pw,
                      c.label << " OutA width " << on_a << " (expected " << pw << ")");
        CHECK_MESSAGE(on_b == (TOP_VAL + 1) - OUTB_START,
                      c.label << " OutB width " << on_b
                      << " (expected " << ((TOP_VAL + 1) - OUTB_START) << " fixed)");
    }
}
