#include "vioavr/core/machine.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/tracing.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/avr_cpu.hpp"

#include "vioavr/core/bridge_shm_server.hpp"
#include "vioavr/core/gdb_stub.hpp"

#include <chrono>
#include <csignal>
#include <exception>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <sstream>
#include <functional>
#include <cstring>
#include <cstdio>
#include <type_traits>

#include "ansi.hpp"
#include "docs.hpp"

#ifdef HAVE_SHM_OPEN
vioavr::core::BridgeShmServer* g_bridge_server = nullptr;

extern "C" void bridge_signal_handler(int) {
    if (g_bridge_server) {
        std::cout << "\nTerminating bridge server..." << std::endl;
        g_bridge_server->stop();
    }
}
#endif

using namespace vioavr::core;

namespace {

// ---------------------------------------------------------------------------
// Minimal subcommand-aware argument parser
// ---------------------------------------------------------------------------
class Args {
public:
    std::string subcommand;
    std::vector<std::string> positional;
    std::map<std::string, std::string> options;

    bool has(const std::string& key) const {
        return options.find(key) != options.end();
    }

    const std::string& get(const std::string& key, const std::string& fallback = "") const {
        auto it = options.find(key);
        return it != options.end() ? it->second : fallback;
    }

    template <typename T>
    T get_as(const std::string& key, T fallback = {}) const {
        auto it = options.find(key);
        if (it == options.end()) return fallback;
        T val{};
        if constexpr (std::is_same_v<T, bool>) {
            return it->second == "true" || it->second == "1";
        } else if constexpr (std::is_integral_v<T>) {
            return static_cast<T>(std::stoull(it->second));
        }
        return val;
    }

    u64 get_cycles(const std::string& key, u64 fallback = 0) const {
        auto it = options.find(key);
        if (it == options.end()) return fallback;
        return parse_cycles(it->second);
    }

    static u64 parse_cycles(const std::string& s) {
        u64 val = 0;
        char suffix = 0;
        unsigned long long tmp = 0;
        if (std::sscanf(s.c_str(), "%llu%c", &tmp, &suffix) >= 1) {
        val = static_cast<u64>(tmp);
            if (suffix == 'k' || suffix == 'K') val *= 1000ULL;
            else if (suffix == 'm' || suffix == 'M') val *= 1'000'000ULL;
        }
        return val;
    }
};

static Args parse_args(int argc, char** argv) {
    Args args;
    // First positional = subcommand
    int i = 1;
    if (i < argc && argv[i][0] != '-') {
        args.subcommand = argv[i++];
    }
    while (i < argc) {
        std::string_view arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            // Placeholder -- the subcommand handler will process this
            args.options["--help"] = "true";
            ++i;
        } else if (arg.substr(0, 2) == "--") {
            std::string key(arg.substr(2));
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                args.options[key] = argv[++i];
            } else {
                args.options[key] = "true";
            }
            ++i;
        } else if (arg == "--color" && i + 1 < argc) {
            Terminal::set_color_mode(Terminal::parse_color_mode(argv[++i]));
            ++i;
        } else if (arg.substr(0, 8) == "--color=") {
            Terminal::set_color_mode(Terminal::parse_color_mode(arg.substr(8).data()));
            ++i;
        } else if (arg.substr(0, 1) == "-") {
            // Short option: accept -<char> <value>
            std::string key(1, arg[1]);
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                args.options[key] = argv[++i];
            } else {
                args.options[key] = "true";
            }
            ++i;
        } else {
            args.positional.push_back(argv[i++]);
        }
    }
    return args;
}

// ---------------------------------------------------------------------------
// Trace hook for run/trace subcommands
// ---------------------------------------------------------------------------
class CliTraceHook final : public vioavr::core::ITraceHook {
public:
    bool verbose = false;
    bool show_registers = true;

    void on_instruction(vioavr::core::u32 address, vioavr::core::u16 opcode, std::string_view mnemonic) override {
        std::cout << "  " << Terminal::fg(Terminal::Color::bright_black) << "[0x"
                  << std::hex << std::right << std::setw(4) << std::setfill('0') << address << "]"
                  << Terminal::reset_all() << " "
                  << Terminal::fg(Terminal::Color::green) << std::setw(10) << std::left << std::setfill(' ') << mnemonic
                  << Terminal::reset_all()
                  << Terminal::fg(Terminal::Color::bright_black) << " (0x"
                  << std::hex << std::right << std::setw(4) << std::setfill('0') << opcode << ")"
                  << Terminal::reset_all() << std::endl;
    }

    void on_register_write(vioavr::core::u8 index, vioavr::core::u8 value) override {
        if (show_registers) {
            std::cout << "    " << Terminal::fg(Terminal::Color::yellow) << "R"
                      << std::dec << static_cast<int>(index) << Terminal::reset_all()
                      << " = " << Terminal::fg(Terminal::Color::cyan) << "0x"
                      << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value)
                      << Terminal::reset_all() << std::endl;
        }
    }

    void on_sreg_write(vioavr::core::u8 value) override {
        if (show_registers) {
            std::cout << "    " << Terminal::fg(Terminal::Color::magenta) << "SREG"
                      << Terminal::reset_all() << " = 0x"
                      << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value)
                      << Terminal::fg(Terminal::Color::bright_black) << " (" << Terminal::reset_all();
            constexpr const char* names = "CZNVSH TI";
            for (int i = 0; i < 8; ++i) {
                if (value & (1 << i)) {
                    std::cout << Terminal::fg(Terminal::Color::green) << names[i]
                              << Terminal::reset_all();
                } else {
                    std::cout << Terminal::fg(Terminal::Color::bright_black) << names[i];
                }
                if (i == 4) std::cout << " ";
            }
            std::cout << Terminal::fg(Terminal::Color::bright_black) << ")"
                      << Terminal::reset_all() << std::endl;
        }
    }

    void on_memory_read(vioavr::core::u16, vioavr::core::u8) override {}
    void on_memory_write(vioavr::core::u16 address, vioavr::core::u8 value) override {
        if (verbose) {
            std::cout << "    " << Terminal::fg(Terminal::Color::blue) << "MEM[0x"
                      << std::hex << std::setw(4) << std::setfill('0') << address << "]"
                      << Terminal::reset_all() << " = 0x"
                      << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value)
                      << std::endl;
        }
    }

    void on_interrupt(vioavr::core::u8 vector) override {
        std::cout << "  " << Terminal::fg(Terminal::Color::red) << Terminal::style(Terminal::Style::bold)
                  << "** INTERRUPT " << Terminal::reset_all()
                  << Terminal::fg(Terminal::Color::yellow) << "vector "
                  << std::dec << static_cast<int>(vector) << Terminal::reset_all() << std::endl;
    }
};

// ---------------------------------------------------------------------------
// Logger configuration helpers
// ---------------------------------------------------------------------------
static void log_callback_quiet(vioavr::core::LogLevel level, std::string_view message) {
    if (level == vioavr::core::LogLevel::error) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "[ERROR]"
                  << Terminal::reset_all() << " " << message << std::endl;
    }
}

static void log_callback_default(vioavr::core::LogLevel level, std::string_view message) {
    if (level == vioavr::core::LogLevel::debug) return;
    const char* tag = "INFO";
    std::string color;
    switch (level) {
        case vioavr::core::LogLevel::error:   tag = "ERROR"; color = Terminal::fg(Terminal::Color::red); break;
        case vioavr::core::LogLevel::warning: tag = "WARN";  color = Terminal::fg(Terminal::Color::yellow); break;
        case vioavr::core::LogLevel::info:    tag = "INFO";  color = Terminal::fg(Terminal::Color::green); break;
        default: break;
    }
    std::cerr << color << "[" << tag << "]" << Terminal::reset_all() << " " << message << std::endl;
}

void configure_logger(bool quiet, bool verbose) {
    using namespace vioavr::core;
    if (quiet) {
        Logger::set_callback(log_callback_quiet);
    } else if (!verbose) {
        Logger::set_callback(log_callback_default);
    }
}

// ---------------------------------------------------------------------------
// Device info helper
// ---------------------------------------------------------------------------
void print_device_info(const DeviceDescriptor& dev) {
    auto print_count = [](const char* label, int count) {
        if (count > 0) std::cout << "  " << Terminal::fg(Terminal::Color::green) << label
                                 << Terminal::reset_all() << ": " << count << "\n";
    };
    auto label_val = [](const char* label, auto val) {
        std::cout << "  " << Terminal::fg(Terminal::Color::bright_black) << label
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << val
                  << Terminal::reset_all() << "\n";
    };

    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ " << dev.name << " ═══" << Terminal::reset_all() << "\n";
    label_val("Flash:      ", std::to_string(dev.flash_words * 2) + " bytes (" + std::to_string(dev.flash_words) + " words)");
    label_val("SRAM:       ", std::to_string(dev.sram_bytes) + " bytes");
    label_val("EEPROM:     ", std::to_string(dev.eeprom_bytes) + " bytes");
    label_val("Frequency:  ", std::to_string(dev.cpu_frequency_hz / 1'000'000.0) + " MHz");
    label_val("PC width:   ", std::to_string(dev.pc_width_bytes()) + " bytes");
    label_val("Vectors:    ", std::to_string(dev.interrupt_vector_count));
    label_val("Voltage:    ", std::to_string(dev.operating_voltage_v) + " V");

    std::cout << Terminal::fg(Terminal::Color::bright_black) << "Ports:      "
              << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow)
              << static_cast<int>(dev.port_count) << Terminal::reset_all() << " (";
    for (u8 i = 0; i < dev.port_count; ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << Terminal::fg(Terminal::Color::green) << dev.ports[i].name
                  << Terminal::reset_all();
    }
    std::cout << ")\n";

    std::cout << Terminal::fg(Terminal::Color::bright_black)
              << Terminal::style(Terminal::Style::bold)
              << "Peripherals:" << Terminal::reset_all() << "\n";
    print_count("TIMER8",   dev.timer8_count);
    print_count("TIMER16",  dev.timer16_count);
    print_count("TIMER10",  dev.timer10_count);
    print_count("TCA",      dev.tca_count);
    print_count("TCB",      dev.tcb_count);
    print_count("TCD",      dev.tcd_count);
    print_count("TCE",      dev.tce_count);
    print_count("TC (XMEGA)",       dev.tc_count);
    print_count("AWEX",     dev.awex_count);
    print_count("RTC",      dev.rtc_count);
    print_count("UART",     dev.uart_count);
    print_count("UART8X",   dev.uart8x_count);
    print_count("SPI",      dev.spi_count);
    print_count("SPI8X",    dev.spi8x_count);
    print_count("TWI",      dev.twi_count);
    print_count("TWI8X",    dev.twi8x_count);
    print_count("USI",      dev.usi_count);
    print_count("ADC",      dev.adc_count);
    print_count("ADC8X",    dev.adc8x_count);
    print_count("ADC10B",   dev.adc10b_count);
    print_count("AC",       dev.ac_count);
    print_count("AC8X",     dev.ac8x_count);
    print_count("DAC",      dev.dac_count);
    print_count("DAC8X",    dev.dac8x_count);
    print_count("CAN",      dev.can_count);
    print_count("USB",      dev.usb_count);
    print_count("USB8X",    dev.usb8x_count);
    print_count("CRC",      dev.crc8x_count);
    print_count("WDT",      dev.wdt_count);
    print_count("WDT8X",    dev.wdt8x_count);
    print_count("NVMCTRL",  dev.nvm_ctrl_count);
    print_count("CPUINT",   dev.cpu_int_count);
    print_count("PCINT",    dev.pcint_count);
    print_count("EXTINT",   dev.ext_interrupt_count);
    print_count("EEPROM",   dev.eeprom_count);
    print_count("CCL",      dev.ccl.lut_count > 0 ? 1 : 0);
    print_count("OPAMP",    dev.opamp_count);
    print_count("ZCD",      dev.zcd_count);
    print_count("DMA",      dev.dma_count);
    print_count("LIN",      dev.lin_count);
    print_count("PTC",      dev.ptc_count);
    print_count("CFD",      dev.cfd_count);
    print_count("LCD",      dev.lcd_count);
    print_count("EBI",      dev.ebi_count);
    print_count("IRCOM",    dev.ircom_count);
    print_count("PSC",      dev.psc_count);
    print_count("EUSART",   dev.eusart_count);
}

// ---------------------------------------------------------------------------
// Subcommand: help
// ---------------------------------------------------------------------------
int cmd_help(const Args& args, const std::string& prog) {
    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "VioAVR – AVR Microcontroller Simulator"
              << Terminal::reset_all() << "\n\n";
    std::cout << "Usage: " << Terminal::fg(Terminal::Color::yellow) << prog
              << Terminal::reset_all() << " <" << Terminal::fg(Terminal::Color::green)
              << "subcommand" << Terminal::reset_all() << "> [options]\n\n";
    std::cout << Terminal::style(Terminal::Style::bold) << "Subcommands:"
              << Terminal::reset_all() << "\n";
    auto cmd = [](const char* name, const char* desc) {
        std::cout << "  " << Terminal::fg(Terminal::Color::green) << name
                  << Terminal::reset_all() << "         " << desc << "\n";
    };
    cmd("run <hex>",       "Run firmware and show summary");
    cmd("trace <hex>",     "Trace execution cycle by cycle");
    cmd("benchmark",       "Run performance benchmark");
    cmd("info <device>",   "Show device information");
    cmd("list-devices",    "List all supported MCUs");
    cmd("bridge",          "Start co-simulation bridge daemon");
    cmd("docs [topic]",    "Show built-in documentation");
    cmd("gdb <hex>",       "Start GDB stub with firmware loaded");
    cmd("help",            "Show this help");
    std::cout << "\nRun '" << Terminal::fg(Terminal::Color::yellow) << prog
              << Terminal::reset_all() << " <" << Terminal::fg(Terminal::Color::green)
              << "subcommand" << Terminal::reset_all() << "> "
              << Terminal::style(Terminal::Style::dim) << "--help"
              << Terminal::reset_all() << "' for subcommand-specific options.\n";
    return 0;
}

// ---------------------------------------------------------------------------
// Subcommand: run
// ---------------------------------------------------------------------------
int cmd_run(const Args& args) {
    using namespace vioavr::core;

    if (args.has("--help") || args.positional.empty()) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr run"
                  << Terminal::reset_all() << " [options] <" << Terminal::fg(Terminal::Color::green)
                  << "hex_file" << Terminal::reset_all() << ">\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--mcu <name>",      "MCU (default: ATmega328P)");
        opt("--max-cycles <n>",  "Cycle limit (e.g. 100k, 1M)");
        opt("--summary",         "Show execution summary");
        opt("--show-state",      "Show peripheral state after run");
        opt("--eeprom-file <f>", "Load/save EEPROM");
        opt("--verbose",         "Enable debug output");
        opt("--quiet",           "Suppress non-error output");
        opt("--color <mode>",    "Color mode: auto, always, never");
        opt("--help",            "Show this help");
        return args.positional.empty() ? 1 : 0;
    }

    std::string mcu = args.get("mcu", "ATmega328P");
    std::string hex = args.positional[0];
    u64 max_cycles = args.get_cycles("max-cycles", 1'000'000);
    bool summary = args.has("summary") || args.get_as<bool>("summary");
    bool show_state = args.has("show-state") || args.get_as<bool>("show-state");
    bool verbose = args.has("verbose");
    bool quiet = args.has("quiet");
    std::string eeprom_path = args.get("eeprom-file");

    configure_logger(quiet, verbose);

    auto machine = Machine::create_for_device(mcu);
    if (!machine) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "MCU '" << mcu << "' not found in catalog.\n";
        return 1;
    }

    auto& cpu = machine->cpu();
    auto& bus = machine->bus();

    try {
        auto image = HexImageLoader::load_file(hex, bus.device());
        bus.load_image(image);
        machine->reset();
    } catch (const std::exception& e) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "loading HEX: " << e.what() << std::endl;
        return 2;
    }

    if (!eeprom_path.empty()) {
        auto eeproms = machine->peripherals_of_type<Eeprom>();
        if (!eeproms.empty()) {
            eeproms[0]->load_from_file(eeprom_path);
        }
    }

    while (cpu.state() == CpuState::running && cpu.cycles() < max_cycles) {
        machine->step();
    }

    if (!eeprom_path.empty()) {
        auto eeproms = machine->peripherals_of_type<Eeprom>();
        if (!eeproms.empty()) {
            eeproms[0]->save_to_file(eeprom_path);
        }
    }

    if (!quiet) {
        if (summary) {
            auto state_str = [](CpuState s) {
                switch (s) {
                    case CpuState::running:  return std::string(Terminal::fg(Terminal::Color::green) + "Running");
                    case CpuState::halted:   return std::string(Terminal::fg(Terminal::Color::red) + "Halted");
                    case CpuState::sleeping: return std::string(Terminal::fg(Terminal::Color::blue) + "Sleeping");
                    case CpuState::paused:   return std::string(Terminal::fg(Terminal::Color::yellow) + "Paused");
                }
                return std::string("Unknown");
            };
            std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
                      << "═══ Simulation Summary ═══" << Terminal::reset_all() << "\n";
            std::cout << Terminal::fg(Terminal::Color::bright_black) << "Device: "
                      << Terminal::reset_all() << mcu << "\n";
            std::cout << Terminal::fg(Terminal::Color::bright_black) << "Cycles: "
                      << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow)
                      << cpu.cycles() << Terminal::reset_all() << "\n";
            std::cout << Terminal::fg(Terminal::Color::bright_black) << "State:  "
                      << Terminal::reset_all() << state_str(cpu.state()) << Terminal::reset_all() << "\n";
            std::cout << Terminal::fg(Terminal::Color::bright_black) << "PC:     "
                      << Terminal::reset_all() << "0x" << std::hex << cpu.program_counter() << std::dec << "\n";
            std::cout << Terminal::fg(Terminal::Color::bright_black) << "SP:     "
                      << Terminal::reset_all() << "0x" << std::hex << cpu.stack_pointer() << std::dec << "\n";
            std::cout << Terminal::fg(Terminal::Color::bright_black) << "SREG:   "
                      << Terminal::reset_all() << "0x" << std::hex << static_cast<int>(cpu.sreg()) << std::dec;
            // Show SREG flags
            u8 sreg = cpu.sreg();
            const char* flag_names[] = {"C","Z","N","V","S","H","T","I"};
            std::cout << " (";
            for (int i = 0; i < 8; ++i) {
                if (i > 0) std::cout << " ";
                if (sreg & (1 << i)) {
                    std::cout << Terminal::fg(Terminal::Color::green) << flag_names[i]
                              << Terminal::reset_all();
                } else {
                    std::cout << Terminal::fg(Terminal::Color::bright_black) << flag_names[i]
                              << Terminal::reset_all();
                }
            }
            std::cout << ")\n";
        } else {
            std::cout << Terminal::fg(Terminal::Color::green) << "✓ "
                      << Terminal::reset_all() << "Simulation finished after "
                      << Terminal::fg(Terminal::Color::yellow) << cpu.cycles()
                      << Terminal::reset_all() << " cycles.\n";
        }

        if (show_state) {
            std::cout << Terminal::fg(Terminal::Color::bright_black)
                      << Terminal::style(Terminal::Style::bold)
                      << "═══ GPIO State ═══" << Terminal::reset_all() << "\n";
            auto ports = machine->peripherals_of_type<GpioPort>();
            for (auto* port : ports) {
                u8 ddr = port->ddr_register();
                u8 prt = port->port_register();
                u8 pin = port->output_levels();
                std::cout << "  " << Terminal::fg(Terminal::Color::green) << "PORT"
                          << port->name().back() << Terminal::reset_all()
                          << ": " << Terminal::fg(Terminal::Color::bright_black) << "DDR"
                          << Terminal::reset_all() << "=0x" << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(ddr)
                          << " " << Terminal::fg(Terminal::Color::bright_black) << "PORT"
                          << Terminal::reset_all() << "=0x" << static_cast<int>(prt)
                          << " " << Terminal::fg(Terminal::Color::bright_black) << "OUT"
                          << Terminal::reset_all() << "=0x" << static_cast<int>(pin)
                          << std::dec;
                // Show pin states as bits
                std::cout << " (";
                for (int b = 7; b >= 0; --b) {
                    if (ddr & (1 << b)) {
                        std::cout << ((prt & (1 << b)) ? Terminal::fg(Terminal::Color::green) + "1"
                                                       : Terminal::fg(Terminal::Color::red) + "0");
                    } else {
                        std::cout << Terminal::fg(Terminal::Color::bright_black) << "z";
                    }
                    std::cout << Terminal::reset_all();
                }
                std::cout << ")\n";
            }
        }
    }

    return 0;
}

// ---------------------------------------------------------------------------
// Subcommand: trace
// ---------------------------------------------------------------------------
int cmd_trace(const Args& args) {
    using namespace vioavr::core;

    if (args.has("--help") || args.positional.empty()) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr trace"
                  << Terminal::reset_all() << " [options] <" << Terminal::fg(Terminal::Color::green)
                  << "hex_file" << Terminal::reset_all() << ">\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--mcu <name>",      "MCU (default: ATmega328P)");
        opt("--max-cycles <n>",  "Cycle limit (default: 1000)");
        opt("--registers",       "Show register writes (default: on)");
        opt("--no-registers",    "Hide register writes");
        opt("--mem-writes",      "Show memory writes");
        opt("--color <mode>",    "Color mode: auto, always, never");
        opt("--help",            "Show this help");
        return args.positional.empty() ? 1 : 0;
    }

    std::string mcu = args.get("mcu", "ATmega328P");
    std::string hex = args.positional[0];
    u64 max_cycles = args.get_cycles("max-cycles", 1000);
    bool show_regs = !args.has("no-registers");
    bool mem_writes = args.has("mem-writes");
    bool verbose = args.has("verbose") || mem_writes;

    auto machine = Machine::create_for_device(mcu);
    if (!machine) {
        std::cerr << "Error: MCU '" << mcu << "' not found in catalog.\n";
        return 1;
    }

    auto& cpu = machine->cpu();
    auto& bus = machine->bus();

    try {
        auto image = HexImageLoader::load_file(hex, bus.device());
        bus.load_image(image);
        machine->reset();
    } catch (const std::exception& e) {
        std::cerr << "Error loading HEX: " << e.what() << std::endl;
        return 2;
    }

    CliTraceHook hook;
    hook.show_registers = show_regs;
    hook.verbose = mem_writes;
    cpu.set_trace_hook(&hook);
    bus.set_trace_hook(&hook);

    while (cpu.state() == CpuState::running && cpu.cycles() < max_cycles) {
        machine->step();
    }

    std::cout << "\n" << Terminal::fg(Terminal::Color::green) << "─── "
              << Terminal::reset_all() << "Trace complete: "
              << Terminal::fg(Terminal::Color::yellow) << cpu.cycles()
              << Terminal::reset_all() << " cycles "
              << Terminal::fg(Terminal::Color::green) << "───"
              << Terminal::reset_all() << "\n";
    return 0;
}

// ---------------------------------------------------------------------------
// Subcommand: benchmark
// ---------------------------------------------------------------------------
int cmd_benchmark(const Args& args) {
    using namespace vioavr::core;

    if (args.has("--help")) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr benchmark"
                  << Terminal::reset_all() << " [options]\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--mcu <name>",      "MCU (default: ATmega328P)");
        opt("--cycles <n>",      "Cycle count (default: 100M)");
        opt("--color <mode>",    "Color mode: auto, always, never");
        opt("--help",            "Show this help");
        return 0;
    }

    std::string mcu = args.get("mcu", "ATmega328P");
    u64 total_cycles = args.get_cycles("cycles", 100'000'000);

    auto machine = Machine::create_for_device(mcu);
    if (!machine) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "MCU '" << mcu << "' not found in catalog.\n";
        return 1;
    }

    auto& cpu = machine->cpu();
    auto& bus = machine->bus();

    // Fill flash with NOPs + rjmp $-1024
    std::vector<u16> code(1024, 0x0000);
    code.back() = 0xC000 | (static_cast<u16>(-1024) & 0x0FFF);
    bus.load_flash(code);
    machine->reset();

    int tw = Terminal::width();
    int bar_w = std::min(tw - 20, 60);
    if (bar_w < 10) bar_w = 10;

    std::cout << Terminal::fg(Terminal::Color::bright_black) << "Benchmark: "
              << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << mcu
              << Terminal::reset_all() << Terminal::fg(Terminal::Color::bright_black) << " ("
              << Terminal::reset_all() << total_cycles << Terminal::fg(Terminal::Color::bright_black)
              << " cycles)" << Terminal::reset_all() << "\n";

    bool show_progress = Terminal::is_tty() && total_cycles >= 10'000;
    if (show_progress) {
        std::cout << "\n" << Terminal::cursor_hide();
    }

    constexpr u64 kReportInterval = 1'000'000;
    auto start = std::chrono::high_resolution_clock::now();

    if (show_progress) {
        // Chunked run with progress
        u64 done = 0;
        while (done < total_cycles) {
            u64 chunk = std::min(kReportInterval, total_cycles - done);
            cpu.run(chunk);
            done += chunk;
            double pct = static_cast<double>(done) / static_cast<double>(total_cycles);
            std::cout << Terminal::cursor_up() << Terminal::clear_line()
                      << Terminal::progress_label(pct, "Benchmarking")
                      << std::flush;
        }
        std::cout << "\n" << Terminal::cursor_show();
    } else {
        cpu.run(total_cycles);
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double seconds = duration.count() / 1'000'000.0;
    double mhz = (cpu.cycles() / seconds) / 1'000'000.0;

    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ Benchmark Results ═══" << Terminal::reset_all() << "\n";
    std::cout << Terminal::fg(Terminal::Color::bright_black) << "Time:       "
              << Terminal::reset_all() << seconds << " s\n";
    std::cout << Terminal::fg(Terminal::Color::bright_black) << "Throughput: "
              << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow)
              << std::fixed << std::setprecision(1) << mhz << Terminal::reset_all()
              << Terminal::fg(Terminal::Color::green) << " MHz" << Terminal::reset_all() << "\n";
    return 0;
}

// ---------------------------------------------------------------------------
// Subcommand: info
// ---------------------------------------------------------------------------
int cmd_info(const Args& args) {
    using namespace vioavr::core;

    if (args.has("--help") || args.positional.empty()) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr info"
                  << Terminal::reset_all() << " <" << Terminal::fg(Terminal::Color::green)
                  << "device" << Terminal::reset_all() << ">\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--help",  "Show this help");
        return args.positional.empty() ? 1 : 0;
    }

    std::string name = args.positional[0];
    auto* dev = DeviceCatalog::find(name);
    if (!dev) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "device '" << name << "' not found.\n";
        return 1;
    }

    print_device_info(*dev);
    return 0;
}

// ---------------------------------------------------------------------------
// Subcommand: list-devices
// ---------------------------------------------------------------------------
int cmd_list_devices(const Args& args) {
    using namespace vioavr::core;

    if (args.has("--help")) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr list-devices"
                  << Terminal::reset_all() << " [options]\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--filter <text>",  "Only show devices containing text");
        opt("--color <mode>",   "Color mode: auto, always, never");
        opt("--help",           "Show this help");
        return 0;
    }

    std::string filter = args.get("filter");

    auto names = DeviceCatalog::list_devices();
    if (!filter.empty()) {
        std::string lower = filter;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        auto it = std::remove_if(names.begin(), names.end(), [&](std::string_view n) {
            std::string name_lower(n);
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            return name_lower.find(lower) == std::string::npos;
        });
        names.erase(it, names.end());
    }

    std::cout << Terminal::fg(Terminal::Color::bright_black)
              << "Supported devices (" << Terminal::reset_all()
              << Terminal::fg(Terminal::Color::yellow) << names.size()
              << Terminal::reset_all() << Terminal::fg(Terminal::Color::bright_black) << "):\n"
              << Terminal::reset_all();

    std::string last_group;
    for (auto& n : names) {
        std::string_view sv(n);
        auto dot = sv.find('.');
        std::string group(sv.substr(0, dot == std::string_view::npos ? sv.size() : dot));
        std::string family;
        for (char c : group) {
            if (std::isalpha(c)) family += c; else break;
        }
        if (family != last_group) {
            std::cout << "\n" << Terminal::style(Terminal::Style::bold)
                      << Terminal::fg(Terminal::Color::green) << family
                      << Terminal::reset_all() << ":\n";
            last_group = family;
        }
        std::cout << "  " << Terminal::fg(Terminal::Color::yellow) << n
                  << Terminal::reset_all() << "\n";
    }
    std::cout << "\n";
    return 0;
}

// ---------------------------------------------------------------------------
// Subcommand: bridge
// ---------------------------------------------------------------------------
#ifdef HAVE_SHM_OPEN
int cmd_bridge(const Args& args) {
    using namespace vioavr::core;

    if (args.has("--help")) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr bridge"
                  << Terminal::reset_all() << " [options]\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--mcu <name>",        "MCU (default: atmega328p)");
        opt("--instance <id>",     "SHM instance name");
        opt("--gdb <port>",        "Start GDB stub on port");
        opt("--freq <hz>",         "CPU frequency (default: 16000000)");
        opt("--color <mode>",      "Color mode: auto, always, never");
        opt("--help",              "Show this help");
        return 0;
    }

    std::string mcu = args.get("mcu", "atmega328p");
    std::string instance = args.get("instance", mcu);
    int gdb_port = args.get_as<int>("gdb", -1);
    uint32_t freq = args.get_as<uint32_t>("freq", 16000000);

    std::signal(SIGINT, bridge_signal_handler);
    std::signal(SIGTERM, bridge_signal_handler);

    auto const_device = DeviceCatalog::find(mcu);
    if (!const_device) {
        std::cerr << "Error: MCU '" << mcu << "' not found in catalog.\n";
        return 1;
    }

    DeviceDescriptor device = *const_device;
    device.cpu_frequency_hz = freq;

    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ VioAVR SHM Bridge Daemon ═══" << Terminal::reset_all() << "\n";
    auto line = [](const char* label, const auto& val) {
        std::cout << "  " << Terminal::fg(Terminal::Color::bright_black) << label
                  << Terminal::reset_all() << val << "\n";
    };
    line("MCU:      ", mcu);
    line("Instance: ", instance);
    line("Clock:    ", std::to_string(freq) + " Hz");

    try {
        BridgeShmServer server(device, instance);
        g_bridge_server = &server;

        if (gdb_port > 0) {
            line("GDB:      ", "port " + std::to_string(gdb_port));
            server.start_gdb(static_cast<uint16_t>(gdb_port));
        }

        std::cout << Terminal::fg(Terminal::Color::green) << "  Ready."
                  << Terminal::reset_all() << " Press Ctrl+C to stop.\n";
        server.run_loop();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }

    g_bridge_server = nullptr;
    return 0;
}
#else
int cmd_bridge(const Args&) {
    std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
              << Terminal::reset_all() << "SHM bridge not available on this platform.\n";
    return 1;
}
#endif

// ---------------------------------------------------------------------------
// Subcommand: gdb
// ---------------------------------------------------------------------------
#ifndef _WIN32
int cmd_gdb(const Args& args) {
    using namespace vioavr::core;

    if (args.has("--help") || args.positional.empty()) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr gdb"
                  << Terminal::reset_all() << " [options] <" << Terminal::fg(Terminal::Color::green)
                  << "hex_file" << Terminal::reset_all() << ">\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--mcu <name>",      "MCU (default: ATmega328P)");
        opt("--port <n>",        "GDB port (default: 1234)");
        opt("--color <mode>",    "Color mode: auto, always, never");
        opt("--help",            "Show this help");
        return args.positional.empty() ? 1 : 0;
    }

    std::string mcu = args.get("mcu", "ATmega328P");
    std::string hex = args.positional[0];
    uint16_t port = static_cast<uint16_t>(args.get_as<int>("port", 1234));

    auto machine = Machine::create_for_device(mcu);
    if (!machine) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "MCU '" << mcu << "' not found in catalog.\n";
        return 1;
    }

    try {
        auto image = HexImageLoader::load_file(hex, machine->bus().device());
        machine->bus().load_image(image);
        machine->reset();
    } catch (const std::exception& e) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "loading HEX: " << e.what() << std::endl;
        return 2;
    }

    std::cout << Terminal::fg(Terminal::Color::green) << "  Starting GDB stub on port "
              << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << port
              << Terminal::reset_all() << "\n";
    std::cout << Terminal::fg(Terminal::Color::bright_black) << "  Connect with: "
              << Terminal::reset_all() << "gdb-multiarch -ex 'target remote :"
              << Terminal::fg(Terminal::Color::cyan) << port << Terminal::reset_all() << "'\n";

    machine->enable_gdb(port);
    machine->cpu().run(UINT64_MAX);

    return 0;
}
#else
int cmd_gdb(const Args&) {
    std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
              << Terminal::reset_all() << "GDB stub not available on Windows.\n";
    return 1;
}
#endif

// ---------------------------------------------------------------------------
// Subcommand: docs
// ---------------------------------------------------------------------------
int cmd_docs(const Args& args) {
    if (args.has("--help")) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr docs"
                  << Terminal::reset_all() << " [options] [" << Terminal::fg(Terminal::Color::green)
                  << "topic" << Terminal::reset_all() << "]\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--search <kw>",   "Search documentation for keyword");
        opt("--color <mode>",  "Color mode: auto, always, never");
        opt("--help",          "Show this help");
        std::cout << "\nTopics: overview, mcus, run, trace, benchmark, bridge, gdb, hex,\n"
                  << "        eeprom, evsys, jit, co-sim, programming-guide, a-device,\n"
                  << "        co-sim-tests, device-descriptors, xspice-models\n";
        return 0;
    }

    // --search <keyword> or --search=<keyword>
    if (args.has("search")) {
        print_search_results(args.get("search"));
        return 0;
    }

    // Positional topic argument
    if (!args.positional.empty()) {
        docs_show_topic(args.positional[0]);
        return 0;
    }

    // No arguments: list topics
    print_topic_list();
    return 0;
}

} // namespace

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    using namespace vioavr::core;

    std::string prog = argv[0];
    // Strip path
    auto slash = prog.find_last_of("/\\");
    if (slash != std::string::npos) prog = prog.substr(slash + 1);

    // If invoked as the old name, default to `run` behavior
    if (prog == "vioavr_cli" || prog == "vioavr-cli") {
        // Translate old-style args to subcommand
        std::vector<const char*> new_argv;
        new_argv.push_back(argv[0]);
        new_argv.push_back("run");
        for (int i = 1; i < argc; ++i) new_argv.push_back(argv[i]);
        auto args = parse_args(static_cast<int>(new_argv.size()), const_cast<char**>(new_argv.data()));
        return cmd_run(args);
    }

    auto args = parse_args(argc, argv);

    // No subcommand -> show help
    if (args.subcommand.empty() || args.subcommand == "help") {
        return cmd_help(args, prog);
    }

    struct Cmd { std::string_view name; std::function<int(const Args&)> fn; };
    static const Cmd cmds[] = {
        {"run",           cmd_run},
        {"trace",         cmd_trace},
        {"benchmark",     cmd_benchmark},
        {"info",          cmd_info},
        {"list-devices",  cmd_list_devices},
        {"bridge",        cmd_bridge},
        {"gdb",           cmd_gdb},
        {"docs",          cmd_docs},
        {"help",          [&](const Args&) { return cmd_help(args, prog); }},
    };

    for (auto& cmd : cmds) {
        if (args.subcommand == cmd.name) {
            return cmd.fn(args);
        }
    }

    std::cerr << "Unknown subcommand: " << args.subcommand << "\n";
    cmd_help(args, prog);
    return 1;
}
