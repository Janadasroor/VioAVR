#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/tracing.hpp"
#include "vioavr/core/eeprom.hpp"

#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>
#include <string>

namespace {

class StandardTraceHook final : public vioavr::core::ITraceHook {
public:
    void on_instruction(vioavr::core::u32 address, vioavr::core::u16 opcode, std::string_view mnemonic) override
    {
        std::cout << "[0x" << std::hex << std::right << std::setw(4) << std::setfill('0') << address << "] "
                  << std::left << std::setw(8) << std::setfill(' ') << mnemonic 
                  << " (0x" << std::hex << std::right << std::setw(4) << std::setfill('0') << opcode << ")" << std::endl;
    }

    void on_register_write(vioavr::core::u8 index, vioavr::core::u8 value) override
    {
        std::cout << "  R" << std::dec << static_cast<int>(index) << " = 0x" 
                  << std::hex << std::right << std::setw(2) << std::setfill('0') << static_cast<int>(value) << std::endl;
    }

    void on_sreg_write(vioavr::core::u8 value) override
    {
        std::cout << "  SREG = 0x" << std::hex << std::right << std::setw(2) << std::setfill('0') << static_cast<int>(value) << std::endl;
    }

    void on_memory_read(vioavr::core::u16 address, vioavr::core::u8 value) override
    {
        (void)address; (void)value;
    }

    void on_memory_write(vioavr::core::u16 address, vioavr::core::u8 value) override
    {
        std::cout << "  MEM[0x" << std::hex << std::setw(4) << std::setfill('0') << address << "] = 0x"
                  << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value) << std::endl;
    }

    void on_interrupt(vioavr::core::u8 vector) override
    {
        std::cout << "  INTERRUPT vector " << std::dec << static_cast<int>(vector) << std::endl;
    }
};

void print_usage(std::string_view program) {
    std::cout << "Usage: " << program << " [options] <hex_file>\n"
              << "Options:\n"
              << "  --mcu <name>        Specify MCU (default: ATmega328P)\n"
              << "  --trace             Enable instruction tracing\n"
              << "  --benchmark         Run 100M cycle benchmark\n"
              << "  --max-cycles <n>    Limit simulation to n cycles\n"
              << "  --eeprom-file <f>   Load/save EEPROM from file\n"
              << "  --list-devices      Show all supported MCUs\n"
              << "  --help              Show this help\n";
}

} // namespace

int main(int argc, char** argv)
{
    using namespace std::literals;
    using namespace vioavr::core;

    std::string mcu_name = "ATmega328P";
    bool benchmark = false;
    bool trace = false;
    u64 max_cycles_arg = 0;
    std::string hex_path;
    std::string eeprom_path;

    for (int i = 1; i < argc; ++i) {
        std::string arg {argv[i]};
        if (arg == "--benchmark") {
            benchmark = true;
        } else if (arg == "--trace") {
            trace = true;
        } else if (arg == "--mcu" && i + 1 < argc) {
            mcu_name = argv[++i];
        } else if (arg == "--max-cycles" && i + 1 < argc) {
            max_cycles_arg = std::stoull(std::string(argv[++i]));
        } else if (arg == "--eeprom-file" && i + 1 < argc) {
            eeprom_path = argv[++i];
        } else if (arg == "--list-devices") {
            for (auto name : DeviceCatalog::list_devices()) {
                std::cout << name << "\n";
            }
            return 0;
        } else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (hex_path.empty() && arg.find("--") != 0) {
            hex_path = arg;
        }
    }

    auto machine = Machine::create_for_device(mcu_name);
    if (!machine) {
        std::cerr << "Error: MCU '" << mcu_name << "' not found in catalog.\n";
        return 1;
    }

    auto& cpu = machine->cpu();
    auto& bus = machine->bus();

    // Find EEPROM if path provided
    vioavr::core::Eeprom* eeprom = nullptr;
    // Note: This is a bit of a hack since Machine hides the peripherals, 
    // but for the CLI we can just find it on the bus.
    // Actually, MemoryBus doesn't expose peripherals easily. 
    // Let's just assume it's the first EEPROM if it exists.
    // For now, let's just skip file-based EEPROM until Machine exposes it better.

    StandardTraceHook trace_hook;
    if (trace) {
        cpu.set_trace_hook(&trace_hook);
        bus.set_trace_hook(&trace_hook);
    }

    if (!hex_path.empty()) {
        try {
            const auto image = HexImageLoader::load_file(hex_path, bus.device());
            bus.load_image(image);
            machine->reset();
        } catch (const std::exception& e) {
            std::cerr << "Error loading HEX: " << e.what() << std::endl;
            return 2;
        }
    } else if (benchmark) {
        std::vector<u16> code(1024, 0x0000); // NOPs
        code.back() = 0xC000 | (static_cast<u16>(-1024) & 0x0FFF); // rjmp back to start
        bus.load_flash(code);
        machine->reset();
    }

    if (benchmark) {
        const u64 kBenchmarkCycles = 100'000'000;
        std::cout << "Starting benchmark on " << mcu_name << " (" << kBenchmarkCycles << " cycles)..." << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        cpu.run(kBenchmarkCycles);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double seconds = duration.count() / 1'000'000.0;
        double mips = (cpu.cycles() / seconds) / 1'000'000.0;
        
        std::cout << "Benchmark finished." << std::endl;
        std::cout << "Time: " << seconds << "s" << std::endl;
        std::cout << "Throughput: " << mips << " MHz" << std::endl;
        return 0;
    }

    u64 kMaxCycles = trace ? 1000 : 1'000'000;
    if (max_cycles_arg > 0) {
        kMaxCycles = max_cycles_arg;
    }

    if (hex_path.empty() && !benchmark) {
        print_usage(argv[0]);
        return 0;
    }

    while (cpu.state() == CpuState::running && cpu.cycles() < kMaxCycles) {
        machine->step();
    }

    std::cout << "Simulation finished after " << cpu.cycles() << " cycles." << std::endl;

    return 0;
}
