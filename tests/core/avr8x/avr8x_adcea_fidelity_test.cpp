#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adcea.hpp"
#include "vioavr/core/devices/avr16ea28.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

using namespace vioavr::core;

TEST_CASE("AVR8X AdcEa Fidelity: 8-bit Conversion Timing") {
    MemoryBus bus{devices::avr16ea28};
    AdcEa adc(devices::avr16ea28.adceas[0]);
    bus.attach_peripheral(adc);
    adc.reset();

    AnalogSignalBank signal_bank;
    adc.set_analog_signal_bank(&signal_bank);
    signal_bank.set_voltage(0, 3.3);

    // 8-bit mode, PRESC=DIV1 (0x00), no accumulation
    bus.write_data(0x0609, 0x00); // CTRLF: FREERUN=0, SAMPNUM=0
    bus.write_data(0x0601, 0x00); // CTRLB: PRESC=DIV1 → get_prescaler()=0 → presc_table[0]=2
    bus.write_data(0x0602, 0x00); // CTRLC: REFSEL=VDD
    bus.write_data(0x060C, 0x00); // MUXPOS: AIN0
    bus.write_data(0x060A, 0x00); // COMMAND: MODE=8-bit, DIFF=0
    bus.write_data(0x0600, 0x01); // CTRLA: ENABLE=1

    // Start conversion
    bus.write_data(0x060A, 0x01);

    // 8-bit: (SAMPDUR=2 * SAMPNUM=1 + 8 + 1) * 2 = (2+8+1)*2 = 22 cycles
    bus.tick_peripherals(21);
    u8 intflags = bus.read_data(0x0605);
    CHECK_MESSAGE((intflags & 0x01) == 0, "RESRDY should not be set yet at cycle 21");

    bus.tick_peripherals(1);
    intflags = bus.read_data(0x0605);
    CHECK_MESSAGE((intflags & 0x01) != 0, "RESRDY should be set at cycle 22");

    // Read result
    u16 result = bus.read_data(0x0610) | (bus.read_data(0x0611) << 8);
    // 3.3V / 5.0V * 255 ≈ 168 (8-bit mode)
    u16 expected = static_cast<u16>((3.3 / 5.0) * 255.0 + 0.5);
    CHECK_MESSAGE(result >= expected - 2, "8-bit conversion result should be near 168");
    CHECK_MESSAGE(result <= expected + 2, "8-bit conversion result should be near 168");
}

TEST_CASE("AVR8X AdcEa Fidelity: 12-bit Conversion Result") {
    MemoryBus bus{devices::avr16ea28};
    AdcEa adc(devices::avr16ea28.adceas[0]);
    bus.attach_peripheral(adc);
    adc.reset();

    AnalogSignalBank signal_bank;
    adc.set_analog_signal_bank(&signal_bank);
    signal_bank.set_voltage(0, 5.0);

    bus.write_data(0x0609, 0x00);
    bus.write_data(0x0601, 0x00); // PRESC=DIV1
    bus.write_data(0x0602, 0x00);
    bus.write_data(0x060C, 0x00);
    bus.write_data(0x060A, 0x20); // MODE=12-bit (0x02 << 4)
    bus.write_data(0x0600, 0x01);

    bus.write_data(0x060A, 0x21); // START|MODE

    // 12-bit: (SAMPDUR=2 * SAMPNUM=1 + 12 + 1) * 2 = 30 cycles
    bus.tick_peripherals(29);
    CHECK_MESSAGE((bus.read_data(0x0605) & 0x01) == 0, "RESRDY should not be set yet at cycle 29");

    bus.tick_peripherals(1);
    CHECK_MESSAGE((bus.read_data(0x0605) & 0x01) != 0, "RESRDY should be set at cycle 30");

    u16 result = bus.read_data(0x0610) | (bus.read_data(0x0611) << 8);
    // 5.0V / 5.0V * 4095 = 4095
    CHECK_MESSAGE(result >= 4090, "12-bit full-scale result near 4095");
    CHECK_MESSAGE(result <= 4095, "12-bit result capped at 4095");
}

TEST_CASE("AVR8X AdcEa Fidelity: Differential Mode with PGA") {
    MemoryBus bus{devices::avr16ea28};
    AdcEa adc(devices::avr16ea28.adceas[0]);
    bus.attach_peripheral(adc);
    adc.reset();

    AnalogSignalBank signal_bank;
    adc.set_analog_signal_bank(&signal_bank);
    signal_bank.set_voltage(0, 1.0);
    signal_bank.set_voltage(1, 0.5);

    bus.write_data(0x0609, 0x00);
    bus.write_data(0x0601, 0x00); // PRESC=DIV1
    bus.write_data(0x0602, 0x00);
    bus.write_data(0x060C, 0x00); // MUXPOS=AIN0
    bus.write_data(0x060D, 0x01); // MUXNEG=AIN1
    bus.write_data(0x060B, 0x01); // PGACTRL: PGAEN=1, GAIN=0 (1x)
    bus.write_data(0x060A, 0xA0); // MODE=12-bit | DIFF=1 (no START yet)
    bus.write_data(0x0600, 0x01); // CTRLA: ENABLE=1
    bus.write_data(0x060A, 0xA1); // START | MODE=12-bit | DIFF=1

    bus.tick_peripherals(30);
    CHECK_MESSAGE((bus.read_data(0x0605) & 0x01) != 0, "RESRDY should be set");

    u16 result = bus.read_data(0x0610) | (bus.read_data(0x0611) << 8);
    // Vdiff = 1.0 - 0.5 = 0.5V
    // Result = (0.5 / 5.0) * 2047 + 2048 = 2253
    CHECK_MESSAGE(result >= 2200, "Differential result should be near 2253");
    CHECK_MESSAGE(result <= 2300, "Differential result should be near 2253");
}

TEST_CASE("AVR8X AdcEa Fidelity: Free Running Mode") {
    MemoryBus bus{devices::avr16ea28};
    AdcEa adc(devices::avr16ea28.adceas[0]);
    bus.attach_peripheral(adc);
    adc.reset();

    AnalogSignalBank signal_bank;
    adc.set_analog_signal_bank(&signal_bank);
    signal_bank.set_voltage(0, 2.5);

    // 10-bit free-running, PRESC=DIV1
    bus.write_data(0x0609, 0x20); // FREERUN=1
    bus.write_data(0x0601, 0x00); // PRESC=DIV1
    bus.write_data(0x0602, 0x00);
    bus.write_data(0x060C, 0x00);
    bus.write_data(0x060A, 0x10); // MODE=10-bit
    bus.write_data(0x0600, 0x01);

    bus.write_data(0x060A, 0x11); // START

    // First conversion: (2+10+1)*2 = 26 cycles
    bus.tick_peripherals(26);
    CHECK_MESSAGE((bus.read_data(0x0605) & 0x01) != 0, "First conversion complete");

    u16 result1 = bus.read_data(0x0610) | (bus.read_data(0x0611) << 8);
    // 5.0V ref, 2.5V input, 10-bit: 2.5/5.0 * 1023 ≈ 512
    CHECK_MESSAGE(result1 >= 490, "10-bit result should be near 512");
    CHECK_MESSAGE(result1 <= 530, "10-bit result should be near 512");

    // Clear flag
    bus.write_data(0x0605, 0x01);
    CHECK((bus.read_data(0x0605) & 0x01) == 0);

    // Next conversion should happen automatically
    bus.tick_peripherals(26);
    CHECK_MESSAGE((bus.read_data(0x0605) & 0x01) != 0, "Free-running second conversion");

    u16 result2 = bus.read_data(0x0610) | (bus.read_data(0x0611) << 8);
    CHECK(result2 == result1);
}
