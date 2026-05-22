#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/device.hpp"
#include "vioavr/core/devices/atmega328p.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/gpio_port.hpp"

using namespace vioavr::core;

TEST_CASE("Debug Timer1 Fast PWM") {
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
    port.write(0x24, 0x06);  // DDRB PB1,PB2 output
    bus.reset();

    bus.write_data(dev.timers16[0].tccra_address, 0xB2U);
    bus.write_data(dev.timers16[0].icr_address,     (u8)799);
    bus.write_data(dev.timers16[0].icr_address + 1, (u8)(799 >> 8));
    bus.write_data(dev.timers16[0].ocra_address,    (u8)392);
    bus.write_data(dev.timers16[0].ocra_address + 1, (u8)(392 >> 8));
    bus.write_data(dev.timers16[0].ocrb_address,    (u8)408);
    bus.write_data(dev.timers16[0].ocrb_address + 1, (u8)(408 >> 8));
    bus.write_data(dev.timers16[0].tccrb_address, 0x19U);

    auto pin_state = [&](int bit) -> int {
        return pm.get_state_by_address(0x25, bit).drive_level ? 1 : 0;
    };

    MESSAGE("counter=" << timer.counter() << " pinA=" << pin_state(1) << " pinB=" << pin_state(2)
            << " port=" << (int)port.read(0x23));

    timer.tick(1);
    MESSAGE("after 1 tick: counter=" << timer.counter() << " pinA=" << pin_state(1) << " pinB=" << pin_state(2)
            << " port=" << (int)port.read(0x23));

    timer.tick(390);
    MESSAGE("after 391 ticks: counter=" << timer.counter() << " pinA=" << pin_state(1) << " pinB=" << pin_state(2)
            << " port=" << (int)port.read(0x23));

    timer.tick(1);
    MESSAGE("after 392 ticks: counter=" << timer.counter() << " pinA=" << pin_state(1) << " pinB=" << pin_state(2)
            << " port=" << (int)port.read(0x23));

    timer.tick(15);
    MESSAGE("after 407 ticks: counter=" << timer.counter() << " pinA=" << pin_state(1) << " pinB=" << pin_state(2)
            << " port=" << (int)port.read(0x23));

    timer.tick(1);
    MESSAGE("after 408 ticks: counter=" << timer.counter() << " pinA=" << pin_state(1) << " pinB=" << pin_state(2)
            << " port=" << (int)port.read(0x23));

    timer.tick(799-408);
    MESSAGE("after 799 ticks: counter=" << timer.counter() << " pinA=" << pin_state(1) << " pinB=" << pin_state(2)
            << " port=" << (int)port.read(0x23));

    timer.tick(1);
    MESSAGE("after 800 ticks (wrap): counter=" << timer.counter() << " pinA=" << pin_state(1) << " pinB=" << pin_state(2)
            << " port=" << (int)port.read(0x23));
}
