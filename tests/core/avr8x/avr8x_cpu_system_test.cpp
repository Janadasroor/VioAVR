#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/clkctrl.hpp"
#include "vioavr/core/slpctrl.hpp"
#include "vioavr/core/rstctrl.hpp"
#include "vioavr/core/devices/atmega4809.hpp"

using namespace vioavr::core;
using namespace vioavr::core::devices;

TEST_CASE("AVR8X CPU System Control Integration Test")
{
    MemoryBus bus {atmega4809};
    
    // Instantiate and attach peripherals so the CPU can find them
    auto clk = std::make_unique<ClkCtrl>(atmega4809.clkctrl, 20'000'000U);
    auto slp = std::make_unique<SlpCtrl>(atmega4809.slpctrl);
    auto rst = std::make_unique<RstCtrl>(atmega4809.rstctrl);
    
    bus.attach_peripheral(*clk);
    bus.attach_peripheral(*slp);
    bus.attach_peripheral(*rst);
    
    AvrCpu cpu {bus};
    cpu.reset();

    SUBCASE("ClkCtrl frequency scaling integration") {
        // Default frequency should be 3.333 MHz (20MHz / 6)
        CHECK(cpu.cycles_per_second() == 3'333'333U);

        // Change prescaler to /1 (MCLKCTRLB = 0x00)
        bus.write_data(atmega4809.clkctrl.ctrlb_address, 0x00U);
        CHECK(cpu.cycles_per_second() == 20'000'000U);

        // Change clock source to OSCULP32K (MCLKCTRLA = 0x01)
        bus.write_data(atmega4809.clkctrl.ctrla_address, 0x01U);
        CHECK(cpu.cycles_per_second() == 32'768U);
    }

    SUBCASE("SlpCtrl sleep integration") {
        bus.load_image(HexImage {
            .flash_words = {
                0x9588U, // SLEEP
                0x0000U
            },
            .entry_word = 0U
        });
        cpu.reset();

        // 1. Try sleep when disabled (default)
        cpu.step(); // SLEEP
        CHECK(cpu.state() == CpuState::running);
        CHECK(cpu.program_counter() == 1U);

        // 2. Enable sleep (SLPCTRL.CTRLA = SEN=1 | SMODE=IDLE=0 -> 0x01)
        cpu.reset();
        bus.write_data(atmega4809.slpctrl.ctrla_address, 0x01U);
        cpu.step(); // SLEEP
        CHECK(cpu.state() == CpuState::sleeping);
        
        // Check clock domains in IDLE mode (all active except CPU)
        u8 domains = cpu.active_clock_domains();
        CHECK((domains & static_cast<u8>(ClockDomain::io)) != 0);
        CHECK((domains & static_cast<u8>(ClockDomain::adc)) != 0);

        // 3. Change to Power Down (SMODE=2 -> bits[3:1]=010 -> 0x04 | SEN=0x01 -> 0x05)
        cpu.reset();
        bus.write_data(atmega4809.slpctrl.ctrla_address, 0x05U);
        cpu.step(); // SLEEP
        CHECK(cpu.state() == CpuState::sleeping);
        
        domains = cpu.active_clock_domains();
        CHECK((domains & static_cast<u8>(ClockDomain::io)) == 0);
        CHECK((domains & static_cast<u8>(ClockDomain::adc)) == 0);
        CHECK((domains & static_cast<u8>(ClockDomain::watchdog)) != 0);
    }

    SUBCASE("RstCtrl reset cause integration") {
        // 1. Power-on reset
        cpu.reset(ResetCause::power_on);
        CHECK(bus.read_data(atmega4809.rstctrl.rstfr_address) == 0x01U);

        // 2. Clear flags
        bus.write_data(atmega4809.rstctrl.rstfr_address, 0x01U);
        CHECK(bus.read_data(atmega4809.rstctrl.rstfr_address) == 0x00U);

        // 3. Watchdog reset
        cpu.reset(ResetCause::watchdog);
        CHECK(bus.read_data(atmega4809.rstctrl.rstfr_address) == 0x08U);
    }
}
