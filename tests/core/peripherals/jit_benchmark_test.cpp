#include "doctest.h"

#ifdef VIOAVR_HAVE_JIT
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/atmega328p.hpp"
#include "vioavr/core/devices/atmega4809.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/logger.hpp"
#include <cstdio>
#include <chrono>
#include <vector>
#include <filesystem>

using namespace vioavr::core;
using namespace vioavr::core::devices;

static std::vector<u16> make_firmware() {
    constexpr auto ldi = [](u8 dst, u8 imm) {
        return static_cast<u16>(0xE000U | ((imm & 0xF0U) << 4U) | ((dst - 16U) << 4U) | (imm & 0x0FU));
    };
    constexpr auto add_ = [](u8 dst, u8 src) {
        return static_cast<u16>(0x0C00U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
    };
    constexpr auto sub_ = [](u8 dst, u8 src) {
        return static_cast<u16>(0x1800U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
    };
    constexpr auto and_ = [](u8 dst, u8 src) {
        return static_cast<u16>(0x2000U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
    };
    constexpr auto eor_ = [](u8 dst, u8 src) {
        return static_cast<u16>(0x2400U | (dst << 4U) | (src & 0x0FU) | ((src & 0x10U) << 5U));
    };
    constexpr auto out_ = [](u8 reg, u8 io_offs) {
        return static_cast<u16>(0xB800U | (reg << 4U) | ((io_offs & 0x30U) << 5U) | (io_offs & 0x0FU));
    };
    constexpr auto in_ = [](u8 reg, u8 io_offs) {
        return static_cast<u16>(0xB000U | (reg << 4U) | ((io_offs & 0x30U) << 5U) | (io_offs & 0x0FU));
    };
    constexpr auto push = [](u8 reg) { return static_cast<u16>(0x920FU | (reg << 4U)); };
    constexpr auto pop_ = [](u8 reg) { return static_cast<u16>(0x900FU | (reg << 4U)); };
    constexpr auto rjmp = [](int16_t disp) { return static_cast<u16>(0xC000U | (disp & 0x0FFFU)); };
    constexpr auto nop = []() { return static_cast<u16>(0x0000U); };

    std::vector<u16> fw;
    for (int i = 0; i < 32; i++) {
        fw.push_back(ldi(16, 0xAA));
        fw.push_back(ldi(17, 0x55));
        fw.push_back(add_(16, 17));
        fw.push_back(sub_(16, 17));
        fw.push_back(and_(16, 17));
        fw.push_back(eor_(16, 17));
        fw.push_back(out_(16, 5));
        fw.push_back(in_(17, 5));
        fw.push_back(push(16));
        fw.push_back(pop_(17));
        fw.push_back(nop());
        fw.push_back(nop());
    }
    fw.push_back(rjmp(-static_cast<int16_t>(32 * 12 + 1)));
    return fw;
}

TEST_CASE("JIT Performance Benchmark") {
    auto fw = make_firmware();
    const u64 cycle_target = 20000000;
    const u64 big_cycle_target = 100000000;

    // Interpreter baseline (100M cycles)
    {
        MemoryBus bus{atmega328p};
        AvrCpu cpu{bus};
        bus.load_image(HexImage{.flash_words = fw, .entry_word = 0U});
        cpu.reset();
        auto start = std::chrono::high_resolution_clock::now();
        cpu.run(big_cycle_target);
        auto end = std::chrono::high_resolution_clock::now();
        double sec = std::chrono::duration<double>(end - start).count();
        double mhz = (big_cycle_target / sec) / 1e6;
        std::printf("[JIT-BENCH] Interpreter (100M): %llu cyc in %.3f s = %.1f MHz\n",
                    (unsigned long long)big_cycle_target, sec, mhz);
        CHECK(cpu.cycles() >= big_cycle_target);
    }

    // JIT (100M cycles)
    {
        MemoryBus bus{atmega328p};
        AvrCpu cpu{bus};
        cpu.enable_jit(true);
        bus.load_image(HexImage{.flash_words = fw, .entry_word = 0U});
        cpu.reset();
        auto start = std::chrono::high_resolution_clock::now();
        cpu.run(big_cycle_target);
        auto end = std::chrono::high_resolution_clock::now();
        double sec = std::chrono::duration<double>(end - start).count();
        double mhz = (big_cycle_target / sec) / 1e6;
        std::printf("[JIT-BENCH] JIT (100M): %llu cyc in %.3f s = %.1f MHz\n",
                    (unsigned long long)big_cycle_target, sec, mhz);
        auto stats = cpu.jit_debug_stats();
        std::printf("[JIT-BENCH-DBG] translates=%llu executes=%llu exec_cycles=%llu\n",
                    (unsigned long long)stats.translate_count,
                    (unsigned long long)stats.execute_count,
                    (unsigned long long)stats.execute_cycles);
        CHECK(cpu.cycles() >= big_cycle_target);
    }

    // JIT warm cache (after reset) - 20M
    {
        MemoryBus bus{atmega328p};
        AvrCpu cpu{bus};
        cpu.enable_jit(true);
        bus.load_image(HexImage{.flash_words = fw, .entry_word = 0U});
        cpu.reset();
        cpu.run(100000); // warm up
        cpu.reset();
        auto start = std::chrono::high_resolution_clock::now();
        cpu.run(cycle_target);
        auto end = std::chrono::high_resolution_clock::now();
        double sec = std::chrono::duration<double>(end - start).count();
        double mhz = (cycle_target / sec) / 1e6;
        std::printf("[JIT-BENCH] JIT warm (20M): %llu cyc in %.3f s = %.1f MHz\n",
                    (unsigned long long)cycle_target, sec, mhz);
        CHECK(cpu.cycles() >= cycle_target);
    }
}

static std::string find_hex(const std::string& name) {
    std::string paths[] = {
        "build/tests/" + name + ".hex",
        "../" + name + ".hex",
        "../../build/tests/" + name + ".hex",
        "../../tests/" + name + ".hex",
    };
    for (auto& p : paths) {
        if (std::filesystem::exists(p)) return p;
    }
    return "";
}

TEST_CASE("JIT vs Interpreter: Real Benchmark Hex") {
    Logger::set_callback([](LogLevel, std::string_view) {});

    std::string hex = find_hex("benchmark");
    REQUIRE(!hex.empty());

    const u64 cycle_limit = 100000000;

    // Interpreter
    {
        auto machine_ptr = Machine::create_for_device("atmega328p");
        REQUIRE(machine_ptr != nullptr);
        auto& machine = *machine_ptr;
        auto image = HexImageLoader::load_file(hex, machine.bus().device());
        machine.bus().load_image(image);
        machine.reset();
        auto start = std::chrono::high_resolution_clock::now();
        machine.cpu().run(cycle_limit);
        auto end = std::chrono::high_resolution_clock::now();
        double sec = std::chrono::duration<double>(end - start).count();
        double mhz = (cycle_limit / sec) / 1e6;
        std::printf("[BENCH] Interpreter: 100M cyc in %.2f s = %.1f MHz\n", sec, mhz);
    }

    // JIT
    {
        auto machine_ptr = Machine::create_for_device("atmega328p");
        REQUIRE(machine_ptr != nullptr);
        auto& machine = *machine_ptr;
        machine.cpu().enable_jit(true);
        auto image = HexImageLoader::load_file(hex, machine.bus().device());
        machine.bus().load_image(image);
        machine.reset();
        auto start = std::chrono::high_resolution_clock::now();
        machine.cpu().run(cycle_limit);
        auto end = std::chrono::high_resolution_clock::now();
        double sec = std::chrono::duration<double>(end - start).count();
        double mhz = (cycle_limit / sec) / 1e6;
        auto stats = machine.cpu().jit_debug_stats();
        std::printf("[BENCH] JIT: 100M cyc in %.2f s = %.1f MHz\n", sec, mhz);
        std::printf("[BENCH] JIT stats: translates=%llu executes=%llu exec_cycles=%llu\n",
                    (unsigned long long)stats.translate_count,
                    (unsigned long long)stats.execute_count,
                    (unsigned long long)stats.execute_cycles);
    }
}

TEST_CASE("JIT vs Interpreter: EVSYS ADC PWM (ATmega4809)") {
    Logger::set_callback([](LogLevel, std::string_view) {});

    // Built from tests/core/peripherals/evsys_adc_pwm_benchmark/evsys_adc_pwm.c
    // avr-gcc -mmcu=atmega4809 -Os -std=c99 -DF_CPU=16000000UL
    static const u16 firmware[] = {
        0x940C, 0x0050, 0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A,
        0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A,
        0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A,
        0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A,
        0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A,
        0x940C, 0x005A, 0x940C, 0x005C, 0x940C, 0x005A, 0x940C, 0x005A,
        0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A,
        0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A,
        0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A,
        0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A, 0x940C, 0x005A,
        0x2411, 0xBE1F, 0xEFCF, 0xBFCD, 0xE3DF, 0xBFDE, 0x940E, 0x006F,
        0x940C, 0x0093, 0x940C, 0x0000, 0x938F, 0x939F, 0x93EF, 0x93FF,
        0xE0E0, 0xE0F6, 0x8980, 0x8991, 0x9380, 0x0A28, 0x9390, 0x0A29,
        0xE081, 0x8783, 0x91FF, 0x91EF, 0x919F, 0x918F, 0x9518, 0xE081,
        0x9380, 0x0400, 0x9380, 0x0A00, 0xE190, 0x9390, 0x0A01, 0xEF2F,
        0xEF3F, 0x9320, 0x0A26, 0x9330, 0x0A27, 0xE020, 0xE830, 0x9320,
        0x0A28, 0x9330, 0x0A29, 0x9210, 0x0601, 0xE590, 0x9390, 0x0602,
        0xE093, 0x9390, 0x0600, 0x9210, 0x0606, 0x9380, 0x0608, 0x9380,
        0x060A, 0x9478, 0xCFFF, 0x94F8, 0xCFFF,
    };
    HexImage fw_image;
    fw_image.flash_words.assign(std::begin(firmware), std::end(firmware));
    fw_image.entry_word = 0;

    const u64 cycle_limit = 10000000;

    // Debug: fine-grained step
    {
        auto machine_ptr = Machine::create_for_device("atmega4809");
        REQUIRE(machine_ptr != nullptr);
        auto& machine = *machine_ptr;
        machine.cpu().enable_jit(true);
        machine.bus().load_image(fw_image);
        machine.reset();
        auto& bus = machine.bus();
        auto& cpu = machine.cpu();

        for (int n = 0; n <= 100; n++) {
            cpu.run(2); // 2 cycle steps
            u64 cyc = cpu.cycles();
            u16 cmp0 = bus.read_data(0x0A28) | (bus.read_data(0x0A29) << 8);
            u8 intf = bus.read_data(0x060B);
            u8 intc = bus.read_data(0x060A);
            u8 sreg = bus.read_data(machine.bus().device().sreg_address);
            u8 adc_ctrla = bus.read_data(0x0600);
            u16 res = bus.read_data(0x0610) | (bus.read_data(0x0611) << 8);
            if (cyc >= 50 && cyc <= 140) {
                std::printf("[S] cyc=%3llu CMP0=%-5u INTFL=0x%02X R=%-3u SREG=0x%02X CTRLA=0x%02X\n",
                            (unsigned long long)cyc, cmp0, intf, res, sreg, adc_ctrla);
            }
            if (cyc >= 140) break;
        }
    }

    // Interpreter
    {
        auto machine_ptr = Machine::create_for_device("atmega4809");
        REQUIRE(machine_ptr != nullptr);
        auto& machine = *machine_ptr;
        machine.bus().load_image(fw_image);
        machine.reset();
        auto& bus = machine.bus();
        u16 cmp0_init = bus.read_data(0x0A28) | (bus.read_data(0x0A29) << 8);
        std::printf("[DBG] INTERP initial: CMP0=%u\n", cmp0_init);

        auto start = std::chrono::high_resolution_clock::now();
        machine.cpu().run(cycle_limit);
        auto end = std::chrono::high_resolution_clock::now();
        double sec = std::chrono::duration<double>(end - start).count();
        double mhz = (cycle_limit / sec) / 1e6;
        std::printf("[EVSYS-ADC] Interpreter (10M): %llu cyc in %.3f s = %.1f MHz\n",
                    (unsigned long long)cycle_limit, sec, mhz);
        CHECK(machine.cpu().cycles() >= cycle_limit);

        u16 cmp0_final = bus.read_data(0x0A28) | (bus.read_data(0x0A29) << 8);
        u8 intflags = bus.read_data(0x060B);
        u64 cyc = machine.cpu().cycles();
        std::printf("[DBG] INTERPRETER FINAL: cyc=%llu ADC INTFLAGS=0x%02X TCA CMP0=%u CMP0changed=%s\n",
                    (unsigned long long)cyc, intflags, cmp0_final,
                    (cmp0_final != cmp0_init) ? "YES" : "NO");
    }

    // JIT
    {
        auto machine_ptr = Machine::create_for_device("atmega4809");
        REQUIRE(machine_ptr != nullptr);
        auto& machine = *machine_ptr;
        machine.cpu().enable_jit(true);
        machine.bus().load_image(fw_image);
        machine.reset();
        auto& bus = machine.bus();

        u16 cmp0_init = bus.read_data(0x0A28) | (bus.read_data(0x0A29) << 8);
        std::printf("[DBG-JIT] initial: CMP0=%u\n", cmp0_init);

        auto start = std::chrono::high_resolution_clock::now();
        machine.cpu().run(cycle_limit);
        auto end = std::chrono::high_resolution_clock::now();
        double sec = std::chrono::duration<double>(end - start).count();
        double mhz = (cycle_limit / sec) / 1e6;
        auto stats = machine.cpu().jit_debug_stats();
        std::printf("[EVSYS-ADC] JIT (10M): %llu cyc in %.3f s = %.1f MHz\n",
                    (unsigned long long)cycle_limit, sec, mhz);
        std::printf("[EVSYS-ADC] JIT stats: translates=%llu executes=%llu exec_cycles=%llu\n",
                    (unsigned long long)stats.translate_count,
                    (unsigned long long)stats.execute_count,
                    (unsigned long long)stats.execute_cycles);
        CHECK(machine.cpu().cycles() >= cycle_limit);

        u16 cmp0_final = bus.read_data(0x0A28) | (bus.read_data(0x0A29) << 8);
        u8 intflags = bus.read_data(0x060B);
        u64 cyc = machine.cpu().cycles();
        std::printf("[DBG-JIT] FINAL: cyc=%llu ADC INTFLAGS=0x%02X TCA CMP0=%u CMP0changed=%s\n",
                    (unsigned long long)cyc, intflags, cmp0_final,
                    (cmp0_final != cmp0_init) ? "YES" : "NO");
    }
}
#else
TEST_CASE("JIT Performance Benchmark") {}
TEST_CASE("JIT vs Interpreter: Real Benchmark Hex") {}
TEST_CASE("JIT vs Interpreter: EVSYS ADC PWM (ATmega4809)") {}
#endif
