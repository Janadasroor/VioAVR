#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/adc8x.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include <vector>

using namespace vioavr::core;

TEST_CASE("AVR8X ADC - Conversion Timing Fidelity") {
    auto device = DeviceCatalog::find("ATmega4809");
    REQUIRE(device != nullptr);

    Machine machine(*device);
    auto& bus = machine.bus();
    machine.reset();

    // Load NOPs
    std::vector<u16> program(1000, 0x0000);
    bus.load_flash(program);
    machine.cpu().reset();

    // 1. Setup Signal Bank
    AnalogSignalBank signal_bank;
    auto adcs = machine.peripherals_of_type<Adc8x>();
    REQUIRE(!adcs.empty());
    auto adc = adcs[0];
    adc->set_analog_signal_bank(&signal_bank);
    signal_bank.set_voltage(0, 5.0); // 5V on AIN0

    // 2. Configure ADC: DIV2, 10-bit, AIN0, SampleNum=1
    bus.write_data(0x0602, 0x00); // CTRLC: VREF=VDD (3.3V), PRESC=DIV2
    bus.write_data(0x0606, 0x00); // MUXPOS: AIN0
    bus.write_data(0x0600, 0x01); // CTRLA: ENABLE=1

    // 3. Start Conversion
    bus.write_data(0x0608, 0x01); // COMMAND: STCONV

    // 4. Trace Timing
    // ATmega4809 ADC Timing:
    // Startup: 2 CLK_ADC (if enabled just now? Usually 2)
    // Sample: 2 + SAMPLEN CLK_ADC
    // Conversion: 13 CLK_ADC
    // Prescaler: DIV2 means 1 CLK_ADC = 2 CLK_PER
    // Total for SAMPLEN=0: (2 + 2 + 13) * 2 = 34 cycles
    
    machine.run(33);
    CHECK((bus.read_data(0x060B) & 0x01) == 0); // Not ready yet

    machine.run(1);
    CHECK((bus.read_data(0x060B) & 0x01) != 0); // Ready!
}
