#include "vioavr/core/arduino_board.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/device.hpp"
#include "ansi.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string find_arduino_cli() {
    static const char* candidates[] = {
        "/home/jnd/cpp_projects/VioAVR/bin/arduino-cli",
        "/usr/local/bin/arduino-cli",
        "/usr/bin/arduino-cli",
    };
    for (auto* path : candidates) {
        if (fs::exists(path)) return path;
    }
    const char* path_env = std::getenv("PATH");
    if (path_env) {
        std::string paths(path_env);
        std::istringstream ss(paths);
        std::string dir;
        while (std::getline(ss, dir, ':')) {
            fs::path candidate = fs::path(dir) / "arduino-cli";
            if (fs::exists(candidate)) return candidate.string();
        }
    }
    return "arduino-cli";
}

std::string run_cmd(const std::string& cmd) {
    std::array<char, 4096> buf;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error(std::string("popen failed: ") + std::strerror(errno));
    }
    while (fgets(buf.data(), buf.size(), pipe.get()) != nullptr)
        result += buf.data();
    return result;
}

fs::path find_ino_file(const fs::path& sketch_path) {
    if (fs::is_regular_file(sketch_path)) return sketch_path;
    for (auto& entry : fs::directory_iterator(sketch_path))
        if (entry.path().extension() == ".ino") return entry.path();
    return {};
}

void print_board_info(const vioavr::core::ArduinoBoard& board) {
    using namespace vioavr::core;

    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ " << board.name << " ═══" << Terminal::reset_all() << "\n";

    auto kv = [](const char* k, auto v) {
        std::cout << "  " << Terminal::fg(Terminal::Color::bright_black) << k
                  << Terminal::reset_all() << v << "\n";
    };

    kv("FQBN:     ", board.fqbn);
    kv("MCU:      ", board.mcu);
    kv("Clock:    ", std::to_string(board.f_cpu / 1'000'000) + " MHz");
    kv("Flash:    ", std::to_string(board.flash_bytes) + " bytes");
    kv("SRAM:     ", std::to_string(board.sram_bytes) + " bytes");
    kv("Serial:   ", board.has_serial ? "yes" : "no");
    kv("LED:      ", board.led_builtin == 0xFF ? "none" : "D" + std::to_string(board.led_builtin));
    kv("Analog:   ", std::to_string(board.analog_inputs) + " inputs");

    if (!board.important_pins.empty()) {
        std::cout << Terminal::fg(Terminal::Color::bright_black)
                  << Terminal::style(Terminal::Style::bold)
                  << "Pins:" << Terminal::reset_all() << "\n";
        for (auto& p : board.important_pins) {
            std::cout << "  D" << std::dec << static_cast<int>(p.arduino_pin)
                      << " → " << Terminal::fg(Terminal::Color::green) << p.port
                      << Terminal::reset_all() << static_cast<int>(p.bit)
                      << (p.label.empty() ? "" : " (")
                      << (!p.label.empty() ? Terminal::fg(Terminal::Color::yellow) : "")
                      << p.label
                      << (!p.label.empty() ? Terminal::reset_all() : "")
                      << (!p.label.empty() ? ")" : "")
                      << "\n";
        }
    }

    auto* dev = vioavr::core::DeviceCatalog::find(board.mcu);
    if (dev) {
        std::cout << Terminal::fg(Terminal::Color::bright_black)
                  << Terminal::style(Terminal::Style::bold)
                  << "Ports:" << Terminal::reset_all() << " ";
        for (u8 i = 0; i < dev->port_count; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << Terminal::fg(Terminal::Color::green) << dev->ports[i].name
                      << Terminal::reset_all();
        }
        std::cout << "\n";
    }
}

// ---------------------------------------------------------------------------
// Sub-subcommand: list
// ---------------------------------------------------------------------------
int cmd_arduino_list() {
    auto names = vioavr::core::list_arduino_boards();
    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "Arduino Boards (" << names.size() << ")"
              << Terminal::reset_all() << "\n\n";

    std::string last_mcu;
    for (auto& name : names) {
        auto* board = vioavr::core::find_arduino_board(name);
        if (!board) continue;
        if (board->mcu != last_mcu) {
            if (!last_mcu.empty()) std::cout << "\n";
            last_mcu = board->mcu;
            std::cout << Terminal::fg(Terminal::Color::green) << Terminal::style(Terminal::Style::bold)
                      << board->mcu << Terminal::reset_all() << "\n";
        }
        std::cout << "  " << Terminal::fg(Terminal::Color::yellow) << std::left
                  << std::setw(20) << board->name << Terminal::reset_all()
                  << Terminal::fg(Terminal::Color::bright_black)
                  << std::setw(16) << board->fqbn
                  << std::setw(6) << (std::to_string(board->f_cpu / 1'000'000) + "MHz")
                  << Terminal::reset_all() << "\n";
    }
    std::cout << "\n";
    return 0;
}

// ---------------------------------------------------------------------------
// Sub-subcommand: info
// ---------------------------------------------------------------------------
int cmd_arduino_info(const std::vector<std::string>& positional) {
    if (positional.empty()) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "missing board name. Usage: vioavr arduino info <board>\n";
        return 1;
    }

    auto* board = vioavr::core::find_arduino_board(positional[0]);
    if (!board) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "unknown board '" << positional[0] << "'\n";
        return 1;
    }

    print_board_info(*board);
    return 0;
}

// ---------------------------------------------------------------------------
// Sub-subcommand: run
// ---------------------------------------------------------------------------
int cmd_arduino_run(const std::vector<std::string>& positional,
                    const std::map<std::string, std::string>& options)
{
    using namespace vioavr::core;

    if (positional.empty()) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr arduino run"
                  << Terminal::reset_all() << " [options] <" << Terminal::fg(Terminal::Color::green)
                  << "sketch" << Terminal::reset_all() << ">\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--board <name>",    "Arduino board (default: Uno)");
        opt("--max-cycles <n>",  "Cycle limit (e.g. 100k, 1M)");
        opt("--show-state",      "Show peripheral state after run");
        opt("--serial",          "Attach Serial monitor (stdio)");
        opt("--color <mode>",    "Color mode: auto, always, never");
        opt("--help",            "Show this help");
        return positional.empty() ? 1 : 0;
    }

    std::string board_name = "Uno";
    auto it = options.find("board");
    if (it != options.end()) board_name = it->second;

    auto* board = find_arduino_board(board_name);
    if (!board) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "unknown board '" << board_name << "'\n";
        return 1;
    }

    fs::path sketch_path(positional[0]);
    fs::path ino_file = find_ino_file(sketch_path);
    if (ino_file.empty()) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "cannot find .ino file at '" << positional[0] << "'\n";
        return 1;
    }

    // Build directory for compiled output
    fs::path build_dir = fs::temp_directory_path() / "vioavr-arduino-XXXXXX";
    {
        std::string tmpl = build_dir.string();
        if (mkdtemp(tmpl.data()) == nullptr) {
            std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                      << Terminal::reset_all() << "failed to create temp dir\n";
            return 1;
        }
        build_dir = tmpl;
    }

    // Compile with arduino-cli
    std::string arduino_bin = find_arduino_cli();
    std::cout << Terminal::fg(Terminal::Color::bright_black)
              << "Compiling " << ino_file.filename().string()
              << " for " << board->name << " (" << board->fqbn << ")"
              << Terminal::reset_all() << "\n";

#ifdef _WIN32
    std::string arduino_cmd = arduino_bin;
#else
    std::string arduino_cmd = arduino_bin + " 2>&1";
#endif
    std::ostringstream compile_cmd;
    compile_cmd << arduino_cmd
                << " compile --fqbn " << board->fqbn
                << " --output-dir \"" << build_dir.string() << "\""
                << " \"" << sketch_path.string() << "\"";

    std::string compile_out = run_cmd(compile_cmd.str());
    std::cout << compile_out;

    // Find the .hex file
    fs::path hex_file;
    for (auto& entry : fs::directory_iterator(build_dir)) {
        if (entry.path().extension() == ".hex") {
            hex_file = entry.path();
            break;
        }
    }

    if (hex_file.empty()) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "compilation produced no .hex file\n";

        // Keep build dir for debugging
        std::cerr << "  Build dir: " << build_dir.string() << "\n";
        return 1;
    }

    // Run the .hex in VioAVR
    std::cout << Terminal::fg(Terminal::Color::green) << "Running "
              << hex_file.filename().string() << Terminal::reset_all() << "\n";

    u64 max_cycles = 1'000'000;
    auto cycles_it = options.find("max-cycles");
    if (cycles_it != options.end()) {
        char suffix = 0;
        unsigned long long tmp = 0;
        if (std::sscanf(cycles_it->second.c_str(), "%llu%c", &tmp, &suffix) >= 1) {
            max_cycles = static_cast<u64>(tmp);
            if (suffix == 'k' || suffix == 'K') max_cycles *= 1000ULL;
            else if (suffix == 'm' || suffix == 'M') max_cycles *= 1'000'000ULL;
        }
    }

    bool show_state = options.find("show-state") != options.end();

    auto machine = Machine::create_for_device(std::string(board->mcu));
    if (!machine) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "MCU '" << board->mcu << "' not supported\n";
        return 1;
    }

    auto& cpu = machine->cpu();
    auto& bus = machine->bus();

    try {
        auto image = HexImageLoader::load_file(hex_file.string(), bus.device());
        bus.load_image(image);
        machine->reset();
    } catch (const std::exception& e) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "loading HEX: " << e.what() << std::endl;
        return 2;
    }

    bool serial_enabled = options.find("serial") != options.end();
    u64 last_serial_check = 0;

    while (cpu.state() == CpuState::running && cpu.cycles() < max_cycles) {
        machine->step();

        // Print bytes transmitted by the AVR UART (Serial.print output)
        if (serial_enabled && board->has_serial) {
            auto uarts = machine->peripherals_of_type<Uart>();
            if (!uarts.empty()) {
                u16 data = 0;
                while (uarts[0]->consume_transmitted_byte(data)) {
                    std::cout.put(static_cast<char>(data & 0xFF));
                }
            }
        }
    }

    if (!options.empty() || true) {
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
                  << "═══ Arduino Simulation Summary ═══" << Terminal::reset_all() << "\n";
        std::cout << Terminal::fg(Terminal::Color::bright_black) << "Board:  "
                  << Terminal::reset_all() << board->name << "\n";
        std::cout << Terminal::fg(Terminal::Color::bright_black) << "Cycles: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow)
                  << cpu.cycles() << Terminal::reset_all() << "\n";
        std::cout << Terminal::fg(Terminal::Color::bright_black) << "State:  "
                  << Terminal::reset_all() << state_str(cpu.state()) << Terminal::reset_all() << "\n";
        std::cout << Terminal::fg(Terminal::Color::bright_black) << "PC:     "
                  << Terminal::reset_all() << "0x" << std::hex << cpu.program_counter() << std::dec << "\n";

        if (show_state) {
            std::cout << Terminal::fg(Terminal::Color::bright_black)
                      << Terminal::style(Terminal::Style::bold)
                      << "═══ GPIO State ═══" << Terminal::reset_all() << "\n";
            auto& pmux = machine->pin_mux();
            auto ports = machine->peripherals_of_type<GpioPort>();
            for (auto* port : ports) {
                u8 port_idx = static_cast<u8>(port->name().back() - 'A');
                u8 ddr = port->ddr_register();
                u8 prt = port->port_register();
                std::cout << "  " << Terminal::fg(Terminal::Color::green) << "PORT"
                          << port->name().back() << Terminal::reset_all()
                          << ": " << Terminal::fg(Terminal::Color::bright_black) << "DDR"
                          << Terminal::reset_all() << "=0x" << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(ddr)
                          << " " << Terminal::fg(Terminal::Color::bright_black) << "PORT"
                          << Terminal::reset_all() << "=0x" << static_cast<int>(prt)
                          << std::dec;
                std::cout << " (";
                for (int b = 7; b >= 0; --b) {
                    auto ps = pmux.get_state(port_idx, static_cast<u8>(b));
                    if (ps.is_output) {
                        std::cout << (ps.drive_level
                                     ? Terminal::fg(Terminal::Color::green) + "1"
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

    // Cleanup build dir
    fs::remove_all(build_dir);

    return 0;
}

} // namespace

// ---------------------------------------------------------------------------
// Entry point for "arduino" subcommand
// ---------------------------------------------------------------------------
int cmd_arduino(int argc, char** argv) {
    // argv[0] = "arduino", argv[1] = action ("list", "info", "run"), rest = args
    std::string action;
    std::vector<std::string> positional;
    std::map<std::string, std::string> options;

    int i = 1;
    if (i < argc && argv[i][0] != '-') {
        action = argv[i++];
    }

    while (i < argc) {
        std::string_view arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            options["--help"] = "true";
            ++i;
        } else if (arg.substr(0, 2) == "--") {
            std::string key(arg.substr(2));
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                options[key] = argv[++i];
            } else {
                options[key] = "true";
            }
            ++i;
        } else {
            positional.push_back(argv[i++]);
        }
    }

    bool help = options.find("--help") != options.end();

    if (action.empty() || help) {
        if (!help) {
            std::cerr << Terminal::fg(Terminal::Color::red) << "Error: missing action\n"
                      << Terminal::reset_all();
        }
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr arduino"
                  << Terminal::reset_all() << " <" << Terminal::fg(Terminal::Color::green)
                  << "action" << Terminal::reset_all() << "> [options]\n\n";
        std::cout << Terminal::style(Terminal::Style::bold) << "Actions:"
                  << Terminal::reset_all() << "\n";
        std::cout << "  " << Terminal::fg(Terminal::Color::green) << "list"
                  << Terminal::reset_all() << "             List known Arduino boards\n";
        std::cout << "  " << Terminal::fg(Terminal::Color::green) << "info <board>"
                  << Terminal::reset_all() << "        Show board details\n";
        std::cout << "  " << Terminal::fg(Terminal::Color::green) << "run <sketch>"
                  << Terminal::reset_all() << "       Compile and run a sketch\n";
        return action.empty() ? 1 : 0;
    }

    if (action == "list")
        return cmd_arduino_list();
    else if (action == "info")
        return cmd_arduino_info(positional);
    else if (action == "run")
        return cmd_arduino_run(positional, options);
    else {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "unknown action '" << action << "'\n";
        return 1;
    }
}
