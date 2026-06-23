#include "vioavr/core/arduino_board.hpp"
#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/usb.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/device.hpp"
#include "ansi.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <deque>
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

#ifdef _WIN32
#  include <conio.h>
#else
#  include <sys/ioctl.h>
#  include <sys/select.h>
#  include <unistd.h>
#  include <fcntl.h>
#endif

namespace fs = std::filesystem;

namespace {

// ---------------------------------------------------------------------------
// Serial mode
// ---------------------------------------------------------------------------
enum class SerialMode { none, stdio, pty };

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
#ifdef _WIN32
        const char sep = ';';
#else
        const char sep = ':';
#endif
        while (std::getline(ss, dir, sep)) {
#ifdef _WIN32
            fs::path candidate = fs::path(dir) / "arduino-cli.exe";
            if (fs::exists(candidate)) return candidate.string();
#endif
            fs::path candidate = fs::path(dir) / "arduino-cli";
            if (fs::exists(candidate)) return candidate.string();
        }
    }
    return ""; // not found
}

static bool check_arduino_cli() {
    auto cli = find_arduino_cli();
    if (cli.empty()) {
        std::cerr << Terminal::fg(Terminal::Color::red)
                  << "error: arduino-cli not found.\n"
                  << Terminal::reset_all()
                  << "Install it from https://arduino.github.io/arduino-cli/ or place it in PATH.\n";
        return false;
    }
    return true;
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

// Locate a bootloader hex file.
// If board has bootloader_hex set, search for that relative path under the
// installed arduino cores. Otherwise fall back to MCU-based mapping.
fs::path find_bootloader_hex(const vioavr::core::ArduinoBoard* board,
                             const std::string& mcu_name)
{
    using namespace vioavr::core;

    std::string hex_name;
    if (board && !board->bootloader_hex.empty()) {
        hex_name = std::string(board->bootloader_hex);
    }
    if (hex_name.empty()) {
        // MCU-to-optiboot mapping (fallback for --fqbn mode without board match)
        auto mcn = [](std::string_view s) {
            std::string r;
            for (auto c : s) r += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return r;
        };
        static const std::vector<std::pair<std::string_view, std::string_view>> mcu_to_hex = {
            {"atmega328p", "optiboot/optiboot_atmega328.hex"},
            {"atmega328",  "optiboot/optiboot_atmega328.hex"},
            {"atmega168p", "optiboot/optiboot_atmega168.hex"},
            {"atmega168",  "optiboot/optiboot_atmega168.hex"},
            {"atmega8",    "optiboot/optiboot_atmega8.hex"},
            {"atmega88",   "optiboot/optiboot_atmega8.hex"},
            {"atmega48",   "optiboot/optiboot_atmega8.hex"},
            {"atmega32u4", "caterina/Caterina-Leonardo.hex"},
            {"atmega2560", "stk500v2/stk500boot_v2_mega2560.hex"},
            {"atmega4809", "atmega4809_uart_bl.hex"},
        };
        std::string mcn_lc = mcn(mcu_name);
        for (auto& [prefix, hex] : mcu_to_hex) {
            if (mcn_lc == prefix) { hex_name = hex; break; }
        }
    }
    if (hex_name.empty()) return {};

    // 1) Search local bootloaders/ directory
    const fs::path search_roots[] = {".", "..", "../.."};
    for (auto& root : search_roots) {
        auto p = fs::absolute(fs::path(root) / "bootloaders" / hex_name);
        if (fs::exists(p)) return p;
    }

    // 2) Helper: search for hex_name under a core root
    auto search_core = [&](const fs::path& core_root) -> fs::path {
        if (!fs::is_directory(core_root)) return {};
        for (auto& entry : fs::directory_iterator(core_root)) {
            if (!entry.is_directory()) continue;
            auto bl = entry.path() / "bootloaders" / hex_name;
            if (fs::exists(bl)) return bl;
        }
        return {};
    };

    auto data_dir = []() -> fs::path {
        const char* env = std::getenv("ARDUINO_DATA_DIR");
        if (env) return fs::path(env);
        const char* home = std::getenv("HOME");
        return home ? fs::path(home) / ".arduino15" : fs::path{};
    }();
    if (data_dir.empty()) return {};

    // Try arduino:avr core first
    auto bl = search_core(data_dir / "packages" / "arduino" / "hardware" / "avr");
    if (!bl.empty()) return bl;

    // Then arduino:megaavr core (ATmega4809 bootloaders)
    bl = search_core(data_dir / "packages" / "arduino" / "hardware" / "megaavr");
    if (!bl.empty()) return bl;

    return {};
}

// Build FQBN with board options appended (e.g. "arduino:avr:nano:cpu=atmega328")
// Returns {fqbn_with_opts, board_opts_string}.
static std::pair<std::string, std::string>
build_fqbn(const vioavr::core::ArduinoBoard* board,
           const std::map<std::string, std::string>& options)
{
    using namespace vioavr::core;

    auto bo_it = options.find("board-options");
    std::string board_opts;
    if (bo_it != options.end() && !bo_it->second.empty()) {
        board_opts = bo_it->second;
    } else if (board && !board->default_board_options.empty()) {
        board_opts = std::string(board->default_board_options);
    }

    std::string fqbn = board ? std::string(board->fqbn) : "";
    if (!board_opts.empty()) {
        std::istringstream ss(board_opts);
        std::string opt;
        bool first = true;
        while (std::getline(ss, opt, ',')) {
            if (!opt.empty()) {
                fqbn += (first ? ":" : ",") + opt;
                first = false;
            }
        }
    }
    return {fqbn, board_opts};
}

// Validate board options by querying arduino-cli.
// Returns true if valid (or arduino-cli not found); false + error message on failure.
static bool validate_board_options(const std::string& base_fqbn,
                                   const std::string& board_opts,
                                   std::string& error)
{
    if (board_opts.empty()) return true;
    std::string arduino_bin = find_arduino_cli();
    if (arduino_bin.empty()) return true; // can't validate without arduino-cli

    std::ostringstream cmd;
#ifdef _WIN32
    cmd << arduino_bin
#else
    cmd << arduino_bin << " 2>&1"
#endif
        << " board details -b " << base_fqbn
        << " --board-options " << board_opts;

    std::string out = run_cmd(cmd.str());
    // If arduino-cli prints an error, it starts with "Error getting board details:"
    if (out.find("Error") != std::string::npos || out.find("invalid value") != std::string::npos) {
        // Trim the output to first meaningful line
        auto nl = out.find('\n');
        error = (nl == std::string::npos) ? out : out.substr(0, nl);
        return false;
    }
    return true;
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
// FQBN -> MCU resolution
// ---------------------------------------------------------------------------
std::string resolve_mcu_from_fqbn(const std::string& fqbn) {
    auto* board = vioavr::core::find_arduino_board(fqbn);
    if (board) return std::string(board->mcu);

    auto c1 = fqbn.find(':');
    if (c1 == std::string::npos) return {};
    auto c2 = fqbn.find(':', c1 + 1);
    if (c2 == std::string::npos) return {};
    auto c3 = fqbn.find(':', c2 + 1);
    auto board_id = (c3 == std::string::npos)
        ? std::string_view(fqbn).substr(c2 + 1)
        : std::string_view(fqbn).substr(c2 + 1, c3 - c2 - 1);

    for (auto& b : vioavr::core::kArduinoBoards) {
        auto bfqbn = std::string_view(b.fqbn);
        auto b1 = bfqbn.find(':');
        if (b1 == std::string_view::npos) continue;
        auto b2 = bfqbn.find(':', b1 + 1);
        if (b2 == std::string_view::npos) continue;
        auto b3 = bfqbn.find(':', b2 + 1);
        auto bid = (b3 == std::string_view::npos)
                   ? bfqbn.substr(b2 + 1)
                   : bfqbn.substr(b2 + 1, b3 - b2 - 1);
        if (bid == board_id)
            return std::string(b.mcu);
    }

    return {};
}

// ---------------------------------------------------------------------------
// PTY creation (Linux / macOS)
// ---------------------------------------------------------------------------
#ifndef _WIN32
static int create_pty(std::string& slave_path) {
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master == -1) return -1;
    if (grantpt(master) == -1) { close(master); return -1; }
    if (unlockpt(master) == -1) { close(master); return -1; }
    const char* name = ptsname(master);
    if (!name) { close(master); return -1; }
    slave_path = name;
    // Non-blocking reads on master
    int flags = fcntl(master, F_GETFL);
    if (flags != -1) fcntl(master, F_SETFL, flags | O_NONBLOCK);
    return master;
}
#endif

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
int cmd_arduino_info(const std::vector<std::string>& positional,
                     const std::map<std::string, std::string>& options)
{
    auto fqbn_it = options.find("fqbn");
    if (fqbn_it != options.end()) {
        std::string mcu;
        auto mcu_it = options.find("mcu");
        if (mcu_it != options.end()) mcu = mcu_it->second;
        if (mcu.empty()) mcu = resolve_mcu_from_fqbn(fqbn_it->second);
        if (mcu.empty()) {
            std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                      << Terminal::reset_all() << "cannot resolve MCU from FQBN '"
                      << fqbn_it->second << "'. Use --mcu to specify.\n";
            return 1;
        }
        auto* dev = vioavr::core::DeviceCatalog::find(mcu);
        if (!dev) {
            std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                      << Terminal::reset_all() << "MCU '" << mcu << "' not found in device catalog.\n"
                      << "Supported MCUs: ";
            auto devices = vioavr::core::DeviceCatalog::list_devices();
            for (size_t i = 0; i < devices.size(); ++i) {
                if (i > 0) std::cerr << ", ";
                std::cerr << devices[i];
            }
            std::cerr << "\n";
            return 1;
        }
        vioavr::core::ArduinoBoard virt{};
        virt.name = fqbn_it->second;
        virt.fqbn = fqbn_it->second;
        virt.mcu = mcu;
        virt.f_cpu = 0;
        virt.sram_bytes = 0;
        virt.flash_bytes = 0;
        virt.led_builtin = 0xFF;
        virt.analog_inputs = 0;
        virt.has_serial = false;
        virt.important_pins = {};
        print_board_info(virt);
        return 0;
    }

    if (positional.empty()) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "missing board name. Usage: vioavr arduino info [--fqbn <fqbn>] [<board>]\n";
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
// Compile helper (shared by run and compile)
// ---------------------------------------------------------------------------
struct CompileResult {
    fs::path hex_file;
    fs::path build_dir;
};

CompileResult compile_sketch(const fs::path& sketch_path, const std::string& fqbn,
                              const char* display_name)
{
    CompileResult result;

    fs::path ino_file = find_ino_file(sketch_path);
    if (ino_file.empty()) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "cannot find .ino file at '"
                  << sketch_path.string() << "'\n";
        return result;
    }

    fs::path build_dir = fs::temp_directory_path() / "vioavr-arduino-XXXXXX";
    {
        std::string tmpl = build_dir.string();
        if (mkdtemp(tmpl.data()) == nullptr) {
            std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                      << Terminal::reset_all() << "failed to create temp dir\n";
            return result;
        }
        build_dir = tmpl;
    }
    result.build_dir = build_dir;

    std::string arduino_bin = find_arduino_cli();
    std::cout << Terminal::fg(Terminal::Color::bright_black)
              << "Compiling " << ino_file.filename().string()
              << " for " << display_name << " (" << fqbn << ")"
              << Terminal::reset_all() << "\n";

#ifdef _WIN32
    std::string arduino_cmd = arduino_bin;
#else
    std::string arduino_cmd = arduino_bin + " 2>&1";
#endif
    std::ostringstream compile_cmd;
    compile_cmd << arduino_cmd
                << " compile --fqbn " << fqbn
                << " --output-dir \"" << build_dir.string() << "\""
                << " \"" << sketch_path.string() << "\"";

    std::string compile_out = run_cmd(compile_cmd.str());
    std::cout << compile_out;

    for (auto& entry : fs::directory_iterator(build_dir)) {
        if (entry.path().extension() == ".hex") {
            result.hex_file = entry.path();
            break;
        }
    }

    if (result.hex_file.empty()) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "compilation produced no .hex file\n";
        std::cerr << "  Build dir: " << build_dir.string() << "\n";
    }

    return result;
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
        opt("--fqbn <fqbn>",     "Full FQBN (bypasses --board, e.g. arduino:avr:uno)");
        opt("--mcu <name>",      "MCU override (with --fqbn, e.g. ATmega328P)");
        opt("--max-cycles <n>",  "Cycle limit (e.g. 100k, 1M)");
        opt("--show-state",      "Show peripheral state after run");
        opt("--serial [mode]",   "Serial monitor (default=stdio, or 'pty')");
        opt("--bootloader [path]", "Load board bootloader hex (auto-detect or path)");
        opt("--board-options <kv>", "Comma-separated board options (e.g. cpu=16MHzatmega328)");
        opt("--gdb <port>",      "Start GDB stub on port for debugging");
        opt("--color <mode>",    "Color mode: auto, always, never");
        opt("--help",            "Show this help");
        return positional.empty() ? 1 : 0;
    }

    // --fqbn mode bypasses board lookup entirely
    std::string fqbn_override;
    auto fqbn_it = options.find("fqbn");
    if (fqbn_it != options.end()) fqbn_override = fqbn_it->second;

    std::string mcu_override;
    auto mcu_it = options.find("mcu");
    if (mcu_it != options.end()) mcu_override = mcu_it->second;

    const ArduinoBoard* board = nullptr;
    std::string resolved_mcu;
    std::string fqbn;
    std::string board_opts;

    if (!fqbn_override.empty()) {
        // --fqbn given: bypass board lookup, use FQBN directly for compilation
        board = find_arduino_board(fqbn_override); // optional, for metadata

        // Also apply --board-options if provided with --fqbn
        auto bo_it = options.find("board-options");
        if (bo_it != options.end() && !bo_it->second.empty()) {
            board_opts = bo_it->second;
            fqbn = fqbn_override + ":" + board_opts;
        } else {
            fqbn = fqbn_override;
        }

        resolved_mcu = mcu_override.empty() ? resolve_mcu_from_fqbn(fqbn) : mcu_override;
        if (resolved_mcu.empty()) {
            std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                      << Terminal::reset_all() << "cannot resolve MCU from FQBN '"
                      << fqbn << "'. Use --mcu to specify one of:\n  ";
            auto devices = DeviceCatalog::list_devices();
            bool first = true;
            for (auto& d : devices) {
                if (!first) std::cerr << ", ";
                std::cerr << d;
                first = false;
            }
            std::cerr << "\n";
            return 1;
        }
    } else {
        // Legacy --board mode
        std::string board_name = "Uno";
        auto it = options.find("board");
        if (it != options.end()) board_name = it->second;

        board = find_arduino_board(board_name);
        if (!board) {
            std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                      << Terminal::reset_all() << "unknown board '" << board_name << "'\n";
            return 1;
        }

        // Build FQBN with board options
        auto fb = build_fqbn(board, options);
        fqbn = fb.first;
        board_opts = fb.second;
    }

    // Validate board options early (before running simulation)
    if (!board_opts.empty()) {
        std::string base_fqbn = board ? std::string(board->fqbn) : fqbn_override;
        std::string ve;
        if (!validate_board_options(base_fqbn, board_opts, ve)) {
            std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                      << Terminal::reset_all() << ve << "\n";
            return 1;
        }
    }

    int gdb_port = -1;
    auto gdb_it = options.find("gdb");
    if (gdb_it != options.end()) {
        gdb_port = std::stoi(gdb_it->second);
    }

    fs::path sketch_path(positional[0]);

    const char* display_name = board ? board->name.data() : fqbn_override.c_str();
    if (!check_arduino_cli()) return 1;
    auto cr = compile_sketch(sketch_path, fqbn, display_name);
    if (cr.hex_file.empty())
        return 1;
    fs::path build_dir = cr.build_dir;
    fs::path hex_file = cr.hex_file;

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

    {
        const char* mcu_for_machine = board ? board->mcu.data() : resolved_mcu.c_str();
        if (DeviceCatalog::find(mcu_for_machine) == nullptr) {
            std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                      << Terminal::reset_all() << "MCU '" << mcu_for_machine << "' not found in device catalog.\n";
            return 1;
        }
    }
    auto machine = Machine::create_for_device(board ? std::string(board->mcu) : resolved_mcu);
    if (!machine) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "MCU '" << (board ? board->mcu : resolved_mcu) << "' not supported\n";
        return 1;
    }

    auto& cpu = machine->cpu();
    auto& bus = machine->bus();

    // Parse --bootloader flag
    auto bootloader_it = options.find("bootloader");
    bool use_bootloader = bootloader_it != options.end();

    try {
        if (use_bootloader) {
            auto& dev = bus.device();

            fs::path bootloader_path;
            if (bootloader_it->second != "true" && !bootloader_it->second.empty()) {
                bootloader_path = bootloader_it->second;
                if (!fs::exists(bootloader_path)) {
                    std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                              << Terminal::reset_all() << "bootloader hex not found: "
                              << bootloader_path.string() << std::endl;
                    return 2;
                }
            } else {
                const char* mcu_name = board ? board->mcu.data() : resolved_mcu.c_str();
                bootloader_path = find_bootloader_hex(board, mcu_name);
                if (bootloader_path.empty()) {
                    std::cerr << Terminal::fg(Terminal::Color::yellow) << "Warning: "
                              << Terminal::reset_all() << "no bootloader hex found for "
                              << mcu_name << ", running without bootloader\n";
                    use_bootloader = false;
                }
            }

            if (use_bootloader) {
                std::cout << Terminal::fg(Terminal::Color::bright_black)
                          << "Bootloader: " << bootloader_path.filename().string()
                          << Terminal::reset_all() << "\n";

                // Load user application hex (overwrites flash starting at 0)
                auto user_image = HexImageLoader::load_file(hex_file.string(), dev);
                bus.load_image(user_image);

                // Overlay bootloader hex at its natural addresses.
                // We use write_program_word for each non-zero word so that
                // the bootloader section (e.g. 0x3800) is programmed without
                // zero-padding from the hex overwriting the user app.
                auto boot_image = HexImageLoader::load_file(bootloader_path.string(), dev);
                for (u32 wi = 0; wi < boot_image.flash_words.size(); ++wi) {
                    if (boot_image.flash_words[wi] != 0) {
                        bus.write_program_word(wi, boot_image.flash_words[wi]);
                    }
                }

                // Reset with external cause so bootloader sees EXTRF and enters programming mode.
                // If --serial is not used, the bootloader will timeout and jump to the app.
                machine->reset(ResetCause::external);

                if (dev.boot_start_address != 0) {
                    cpu.set_program_counter(dev.boot_start_address);
                }
            } else {
                auto image = HexImageLoader::load_file(hex_file.string(), dev);
                bus.load_image(image);
                machine->reset();
            }
        } else {
            auto image = HexImageLoader::load_file(hex_file.string(), bus.device());
            bus.load_image(image);
            machine->reset();
        }
    } catch (const std::exception& e) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "loading HEX: " << e.what() << std::endl;
        return 2;
    }

    // Parse --serial mode
    SerialMode serial_mode = SerialMode::none;
    auto serial_it = options.find("serial");
    if (serial_it != options.end()) {
        if (serial_it->second == "pty") serial_mode = SerialMode::pty;
        else serial_mode = SerialMode::stdio;
    }

    constexpr u64 kSerialPollInterval = 1000;
    std::deque<u8> rx_pending;
    std::string pty_slave_path;
    int pty_master_fd = -1;

    auto drain_tx = [&](Uart* uart) {
        if (!uart) return;
        u16 data = 0;
        while (uart->consume_transmitted_byte(data)) {
            u8 byte = static_cast<u8>(data & 0xFF);
            if (serial_mode == SerialMode::pty && pty_master_fd != -1) {
                [[maybe_unused]] ssize_t n = ::write(pty_master_fd, &byte, 1);
            } else {
                std::cout.put(static_cast<char>(byte));
            }
        }
        std::cout.flush();
    };

    auto uart_can_rx = [](Uart* uart) -> bool {
        if (!uart) return false;
        u8 ucsra = uart->read(uart->descriptor().ucsra_address);
        u8 ucsrb = uart->read(uart->descriptor().ucsrb_address);
        bool rxen = (ucsrb & uart->descriptor().rxen_mask) != 0;
        bool rxc_clear = (ucsra & uart->descriptor().rxc_mask) == 0;
        return rxen && rxc_clear;
    };

    bool uart_is_at_baud = false;
    u64 inject_count = 0;

    auto inject_rx = [&](Uart* uart) {
        if (!uart) return;
        u8 ubrrl = uart->read(uart->descriptor().ubrrl_address);
        u8 ubrrh = uart->read(uart->descriptor().ubrrh_address);
        u16 ubrr = (static_cast<u16>(ubrrh & 0x0F) << 8) | ubrrl;
        bool sketch_mode = (ubrr > 50);
        if (sketch_mode) uart_is_at_baud = true;
        if (!uart_is_at_baud) return;

        int read_fd = (serial_mode == SerialMode::pty && pty_master_fd != -1)
                      ? pty_master_fd : STDIN_FILENO;

#ifndef _WIN32
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(read_fd, &fds);
        struct timeval tv = {0, 0};
        while (select(read_fd + 1, &fds, nullptr, nullptr, &tv) > 0) {
            char c;
            if (::read(read_fd, &c, 1) > 0)
                rx_pending.push_back(static_cast<u8>(c));
            else break;
        }
#else
        if (serial_mode == SerialMode::stdio) {
            while (_kbhit()) {
                int c = _getch();
                if (c != EOF) rx_pending.push_back(static_cast<u8>(c));
            }
        }
#endif
        if (rx_pending.empty()) return;

        if (uart->inject_received_byte(rx_pending.front())) {
            rx_pending.pop_front();
            ++inject_count;
        }
    };

    Uart* serial_uart = nullptr;
#ifndef _WIN32
    int dtr_state = -1; // -1 = unknown, 0 = high, 1 = low
    u64 dtr_reset_cooldown = 0;
#endif
    if (serial_mode != SerialMode::none && (board ? board->has_serial : true)) {
        auto uarts = machine->peripherals_of_type<Uart>();
        if (!uarts.empty()) serial_uart = uarts[0];
        if (serial_uart) {
#ifndef _WIN32
            if (serial_mode == SerialMode::pty) {
                pty_master_fd = create_pty(pty_slave_path);
                if (pty_master_fd == -1) {
                    std::cerr << Terminal::fg(Terminal::Color::red) << "Warning: "
                              << Terminal::reset_all() << "failed to create PTY, falling back to stdio\n";
                    serial_mode = SerialMode::stdio;
                } else {
                    std::cout << Terminal::fg(Terminal::Color::green)
                              << "Serial PTY: " << Terminal::reset_all()
                              << pty_slave_path << "\n"
                              << Terminal::fg(Terminal::Color::bright_black)
                              << "Connect: picocom " << pty_slave_path
                              << Terminal::reset_all() << "\n";
                    // Capture initial DTR state
                    int mctrl = 0;
                    if (ioctl(pty_master_fd, TIOCMGET, &mctrl) == 0)
                        dtr_state = (mctrl & TIOCM_DTR) ? 0 : 1;
                }
            }
#else
            if (serial_mode == SerialMode::pty) {
                std::cerr << Terminal::fg(Terminal::Color::red) << "Warning: "
                          << Terminal::reset_all() << "PTY not supported on Windows, falling back to stdio\n";
                serial_mode = SerialMode::stdio;
            }
#endif
            if (serial_mode == SerialMode::stdio)
                std::cout << Terminal::fg(Terminal::Color::bright_black)
                          << "Serial monitor: stdin → UART RX, UART TX → stdout"
                          << Terminal::reset_all() << "\n";
        }
    }

    // GDB stub (cross-platform since WinSock2 port)
    if (gdb_port > 0) {
        machine->enable_gdb(static_cast<uint16_t>(gdb_port));
        std::cout << Terminal::fg(Terminal::Color::green)
                  << "GDB stub on port " << Terminal::reset_all()
                  << gdb_port << "\n"
                  << Terminal::fg(Terminal::Color::bright_black)
                  << "Connect: gdb-multiarch -ex 'target remote :" << gdb_port << "'"
                  << Terminal::reset_all() << "\n";
    }

    // USB connect simulation for USB-native bootloaders (Caterina, etc.)
    bool usb_connect_injected = false;
    Usb* usb_peripheral = nullptr;
    if (use_bootloader && bus.device().usb_count > 0) {
        auto usbs = machine->peripherals_of_type<Usb>();
        if (!usbs.empty()) usb_peripheral = usbs[0];
    }

    while (cpu.state() == CpuState::running && cpu.cycles() < max_cycles) {
        cpu.run(serial_uart ? kSerialPollInterval : max_cycles);

        // Simulate USB connect after bootloader initializes USB
        if (usb_peripheral && !usb_connect_injected && cpu.cycles() >= 20000) {
            usb_peripheral->simulate_vbus_event(true);
            usb_peripheral->simulate_usb_reset();
            usb_connect_injected = true;
        }

#ifndef _WIN32
        // DTR-based auto-reset for bootloader (PTY only)
        if (serial_mode == SerialMode::pty && pty_master_fd != -1 && use_bootloader
            && cpu.cycles() >= dtr_reset_cooldown) {
            int mctrl = 0;
            if (ioctl(pty_master_fd, TIOCMGET, &mctrl) == 0) {
                bool dtr_low = (mctrl & TIOCM_DTR) == 0;
                // Detect high→low transition
                if (dtr_state == 0 && dtr_low) {
                    machine->reset(ResetCause::external);
                    if (bus.device().boot_start_address != 0)
                        cpu.set_program_counter(bus.device().boot_start_address);
                    dtr_reset_cooldown = cpu.cycles() + 100000; // debounce
                }
                dtr_state = dtr_low ? 1 : 0;
            }
        }
#endif

        if (serial_uart) {
            drain_tx(serial_uart);
            inject_rx(serial_uart);
        }
    }

    if (serial_uart) {
        drain_tx(serial_uart);
        // Wait for remaining RX pipeline to drain and TX to complete
        u64 drain_cycles = 0;
        while (drain_cycles < 1'000'000) {
            machine->step();
            ++drain_cycles;
            if (drain_cycles % kSerialPollInterval == 0) {
                drain_tx(serial_uart);
                if (!rx_pending.empty())
                    inject_rx(serial_uart);
                if (rx_pending.empty()) break;
            }
        }
        // Extra cycles to let the last byte's TX complete
        for (u64 i = 0; i < 50000; ++i) {
            machine->step();
        }
        drain_tx(serial_uart);
        std::cout.flush();
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
                  << Terminal::reset_all() << (board ? board->name.data() : fqbn_override.c_str()) << "\n";
        std::cout << Terminal::fg(Terminal::Color::bright_black) << "Cycles: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow)
                  << std::dec << cpu.cycles() << Terminal::reset_all() << "\n";
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

    // Cleanup
    fs::remove_all(build_dir);
    if (pty_master_fd != -1) close(pty_master_fd);

    return 0;
}

// ---------------------------------------------------------------------------
// Sub-subcommand: compile
// ---------------------------------------------------------------------------
int cmd_arduino_compile(const std::vector<std::string>& positional,
                        const std::map<std::string, std::string>& options)
{
    using namespace vioavr::core;

    if (positional.empty()) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr arduino compile"
                  << Terminal::reset_all() << " [options] <" << Terminal::fg(Terminal::Color::green)
                  << "sketch" << Terminal::reset_all() << ">\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--board <name>",    "Arduino board (default: Uno)");
        opt("--fqbn <fqbn>",     "Full FQBN (bypasses --board)");
        opt("--board-options <kv>", "Comma-separated board options");
        opt("--help",            "Show this help");
        return 1;
    }

    std::string fqbn_override;
    auto fqbn_it = options.find("fqbn");
    if (fqbn_it != options.end()) fqbn_override = fqbn_it->second;

    const ArduinoBoard* board = nullptr;
    std::string fqbn;
    std::string board_opts;

    if (!fqbn_override.empty()) {
        board = find_arduino_board(fqbn_override);

        // Also apply --board-options if provided with --fqbn
        auto bo_it = options.find("board-options");
        if (bo_it != options.end() && !bo_it->second.empty()) {
            board_opts = bo_it->second;
            fqbn = fqbn_override + ":" + board_opts;
        } else {
            fqbn = fqbn_override;
        }
    } else {
        std::string board_name = "Uno";
        auto it = options.find("board");
        if (it != options.end()) board_name = it->second;

        board = find_arduino_board(board_name);
        if (!board) {
            std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                      << Terminal::reset_all() << "unknown board '" << board_name << "'\n";
            return 1;
        }

        // Build FQBN with board options
        auto fb = build_fqbn(board, options);
        fqbn = fb.first;
        board_opts = fb.second;
    }

    fs::path sketch_path(positional[0]);
    const char* display_name = board ? board->name.data() : fqbn_override.c_str();

    if (!check_arduino_cli()) return 1;
    auto cr = compile_sketch(sketch_path, fqbn, display_name);
    if (cr.hex_file.empty())
        return 1;

    std::cout << Terminal::fg(Terminal::Color::green) << "Hex: "
              << Terminal::reset_all() << cr.hex_file.string()
              << Terminal::fg(Terminal::Color::bright_black) << "\n  Build dir: "
              << Terminal::reset_all() << cr.build_dir.string() << "\n";

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
        std::cout << "  " << Terminal::fg(Terminal::Color::green) << "info [--fqbn <fqbn>] [<board>]"
                  << Terminal::reset_all() << "  Show board details\n";
        std::cout << "  " << Terminal::fg(Terminal::Color::green) << "compile [--fqbn <fqbn>] <sketch>"
                  << Terminal::reset_all() << "  Compile a sketch (print .hex path)\n";
        std::cout << "  " << Terminal::fg(Terminal::Color::green) << "run [--fqbn <fqbn>] <sketch>"
                  << Terminal::reset_all() << "  Compile and run a sketch\n";
        return action.empty() ? 1 : 0;
    }

    if (action == "list")
        return cmd_arduino_list();
    else if (action == "info")
        return cmd_arduino_info(positional, options);
    else if (action == "compile")
        return cmd_arduino_compile(positional, options);
    else if (action == "run")
        return cmd_arduino_run(positional, options);
    else {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "unknown action '" << action << "'\n";
        return 1;
    }
}
