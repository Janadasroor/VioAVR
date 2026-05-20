#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/psc.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/devices/at90pwm316.hpp"
#include <filesystem>

#include "vioavr/core/hex_image.hpp"

namespace {
using namespace vioavr::core;
using namespace vioavr::core::devices;
}

TEST_CASE("AT90PWM316 PSC PID Control Loop Integration") {
    // 1. Create Machine
    Machine machine {at90pwm316};
    machine.reset();

    // 2. Load Firmware
    std::string hex_path = "tests/psc_pid.hex"; // From build root
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../psc_pid.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "../tests/psc_pid.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        hex_path = "build/tests/psc_pid.hex";
    }
    if (!std::filesystem::exists(hex_path)) {
        FAIL("Firmware hex not found: tests/psc_pid.hex");
    }
    auto image = HexImageLoader::load_file(hex_path, at90pwm316);
    machine.bus().load_image(image);
    machine.reset(); // Reset AFTER loading to pick up entry point

    auto adcs = machine.peripherals_of_type<Adc>();
    REQUIRE(!adcs.empty());
    auto* adc = adcs[0];

    // 3. Set Feedback Voltage (ADC0 / PB0)
    // 0.5V / 5.0V = 0.1
    adc->set_channel_voltage(0, 0.1);

    // 4. Run Machine
    machine.run(1000000); 

    // 5. Verify PSC Output
    auto pscs = machine.peripherals_of_type<Psc>();
    REQUIRE(!pscs.empty());
    Psc* psc = pscs[0];

    // Debug: Check if code is running
    // std::cout << "Final PC: " << std::hex << machine.cpu().pc() << std::endl;
    // std::cout << "PCTL0: " << (int)psc->read(0xD8) << std::endl;

    // Read internal OCR0RA (Little-Endian buffering check)
    u16 ocrra = (static_cast<u16>(psc->read(at90pwm316.pscs[0].ocrra_address + 1)) << 8);
    ocrra |= psc->read(at90pwm316.pscs[0].ocrra_address);
    
    if (ocrra == 0) {
        printf("FAILED! PC: 0x%04X, SP: 0x%04X\n", (int)machine.cpu().program_counter(), 
               (int)machine.cpu().stack_pointer());
        printf("ADCSRA: 0x%02X\n", (int)machine.bus().read_data(at90pwm316.adcs[0].adcsra_address));
    }
    
    CHECK(ocrra != 0);
    CHECK(ocrra == 150);

    // 6. Change Feedback to be ABOVE setpoint (3.0V)
    // 3.0V / 5.0V = 0.6
    adc->set_channel_voltage(0, 0.6);
    machine.run(500000);

    ocrra = psc->read(at90pwm316.pscs[0].ocrra_address + 1) << 8;
    ocrra |= psc->read(at90pwm316.pscs[0].ocrra_address);
    
    if (ocrra != 10) {
        printf("SECOND STEP FAILED! ocrra: %d, PC: 0x%04X, SP: 0x%04X\n", (int)ocrra, (int)machine.cpu().program_counter(), 
               (int)machine.cpu().stack_pointer());
        printf("ADCSRA: 0x%02X, ADMUX: 0x%02X\n", (int)machine.bus().read_data(at90pwm316.adcs[0].adcsra_address),
               (int)machine.bus().read_data(at90pwm316.adcs[0].admux_address));
        printf("ADC DATA: %d\n", (int)(machine.bus().read_data(at90pwm316.adcs[0].adch_address) << 8 | machine.bus().read_data(at90pwm316.adcs[0].adcl_address)));
    }
    
    CHECK(ocrra == 10);
}
