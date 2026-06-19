#include "vioavr/core/machine.hpp"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/tracing.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/process_stats.hpp"
#include "vioavr/core/uart.hpp"

#include "ansi.hpp"
#include "args.hpp"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace vioavr::core;

namespace {

struct Breakpoint {
    u64 id;
    u32 address;
    bool enabled;
};

struct DebugSession {
    std::unique_ptr<Machine> machine;
    std::vector<Breakpoint> breakpoints;
    u64 next_bp_id = 1;
    u64 step_count = 0;
    bool running = true;
    std::string mcu;
};

class DebugTraceHook final : public ITraceHook {
public:
    bool show_trace = false;

    void on_instruction(u32 address, u16 opcode, std::string_view mnemonic) override {
        if (!show_trace) return;
        std::cout << "  " << Terminal::fg(Terminal::Color::bright_black) << "[0x"
                  << std::hex << std::right << std::setw(4) << std::setfill('0') << address << "]"
                  << Terminal::reset_all() << " "
                  << Terminal::fg(Terminal::Color::green) << std::left << std::setw(12) << std::setfill(' ') << mnemonic
                  << Terminal::reset_all()
                  << Terminal::fg(Terminal::Color::bright_black) << "(0x"
                  << std::hex << std::right << std::setw(4) << std::setfill('0') << opcode << ")"
                  << Terminal::reset_all() << "\n";
    }

    void on_register_write(u8, u8) override {}
    void on_sreg_write(u8) override {}
    void on_memory_read(u16, u8) override {}
    void on_memory_write(u16, u8) override {}
    void on_interrupt(u8 vector) override {
        if (show_trace) {
            std::cout << "  " << Terminal::fg(Terminal::Color::red) << Terminal::style(Terminal::Style::bold)
                      << "** INTERRUPT " << Terminal::reset_all()
                      << Terminal::fg(Terminal::Color::yellow) << "vector "
                      << std::dec << static_cast<int>(vector) << Terminal::reset_all() << "\n";
        }
    }
};

static DebugTraceHook g_trace_hook;

static std::string state_str(CpuState s) {
    switch (s) {
        case CpuState::running:  return std::string(Terminal::fg(Terminal::Color::green)) + "Running";
        case CpuState::halted:   return std::string(Terminal::fg(Terminal::Color::red)) + "Halted";
        case CpuState::sleeping: return std::string(Terminal::fg(Terminal::Color::blue)) + "Sleeping";
        case CpuState::paused:   return std::string(Terminal::fg(Terminal::Color::yellow)) + "Paused";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// Command dispatch
// ---------------------------------------------------------------------------
void show_regs(DebugSession& session) {
    auto& cpu = session.machine->cpu();
    auto snap = cpu.snapshot();

    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ Registers ═══" << Terminal::reset_all() << "\n";

    // GPRs in rows of 8
    for (int row = 0; row < 4; ++row) {
        std::cout << "  ";
        for (int col = 0; col < 8; ++col) {
            int idx = row * 8 + col;
            std::cout << Terminal::fg(Terminal::Color::yellow) << "R" << std::dec << std::setw(2) << std::setfill(' ') << idx
                      << Terminal::reset_all() << "=0x"
                      << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(snap.gpr[idx])
                      << Terminal::fg(Terminal::Color::bright_black) << "  " << Terminal::reset_all();
        }
        std::cout << "\n";
    }

    std::cout << "  "
              << Terminal::fg(Terminal::Color::cyan) << "PC " << Terminal::reset_all() << "=0x"
              << std::hex << snap.program_counter
              << "  "
              << Terminal::fg(Terminal::Color::magenta) << "SP " << Terminal::reset_all() << "=0x"
              << std::hex << snap.stack_pointer
              << "  "
              << Terminal::fg(Terminal::Color::green) << "SREG" << Terminal::reset_all() << "=0x"
              << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(snap.sreg)
              << Terminal::fg(Terminal::Color::bright_black) << " (" << Terminal::reset_all();

    constexpr const char* flag_names = "CZNVSH TI";
    for (int i = 0; i < 8; ++i) {
        if (snap.sreg & (1 << i)) {
            std::cout << Terminal::fg(Terminal::Color::green) << flag_names[i]
                      << Terminal::reset_all();
        } else {
            std::cout << Terminal::fg(Terminal::Color::bright_black) << flag_names[i];
        }
        if (i == 4) std::cout << " ";
    }
    std::cout << Terminal::fg(Terminal::Color::bright_black) << ")"
              << Terminal::reset_all() << "\n";

    std::cout << "  Cycles: " << Terminal::fg(Terminal::Color::yellow)
              << snap.cycles << Terminal::reset_all()
              << "  State: " << state_str(snap.state) << "\n";
}

void show_pins(DebugSession& session) {
    auto& pmux = session.machine->pin_mux();
    auto ports = session.machine->peripherals_of_type<GpioPort>();

    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ Pin States ═══" << Terminal::reset_all() << "\n";

    for (auto* port : ports) {
        char port_letter = '\0';
        auto n = port->name();
        if (!n.empty()) port_letter = static_cast<char>(n.back());

        u8 port_idx = port_letter >= 'A' ? static_cast<u8>(port_letter - 'A') : 0xFF;
        u8 ddr = port->ddr_register();
        u8 prt = port->port_register();

        std::cout << "  " << Terminal::fg(Terminal::Color::green) << "PORT" << port_letter
                  << Terminal::reset_all()
                  << "  DDR=0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(ddr)
                  << "  PORT=0x" << static_cast<int>(prt) << std::dec
                  << "  ";
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
        std::cout << "\n";
    }
}

void show_mem(DebugSession& session, u16 addr, int count) {
    auto& bus = session.machine->bus();
    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ Memory (0x" << std::hex << addr << std::dec << ") ═══"
              << Terminal::reset_all() << "\n";

    for (int i = 0; i < count; i += 16) {
        u16 line_addr = static_cast<u16>(addr + i);
        std::cout << "  " << Terminal::fg(Terminal::Color::bright_black)
                  << "0x" << std::hex << std::setw(4) << std::setfill('0') << line_addr << ":"
                  << Terminal::reset_all() << " ";

        for (int j = 0; j < 16 && (i + j) < count; ++j) {
            u8 val = bus.read_data(static_cast<u16>(line_addr + j));
            std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(val) << " ";
        }
        std::cout << " ";
        for (int j = 0; j < 16 && (i + j) < count; ++j) {
            u8 val = bus.read_data(static_cast<u16>(line_addr + j));
            std::cout << (val >= 32 && val < 127 ? static_cast<char>(val) : '.');
        }
        std::cout << "\n";
    }
}

void show_disasm(DebugSession& session, u32 start_addr, int count) {
    auto& bus = session.machine->bus();
    auto& cpu = session.machine->cpu();
    auto flash = bus.flash_words();
    u32 pc = cpu.program_counter();

    char hexbuf[32];
    std::snprintf(hexbuf, sizeof(hexbuf), "═══ Disassembly (0x%04X) ═══", static_cast<unsigned>(start_addr));
    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << hexbuf << Terminal::reset_all() << "\n";

    u32 addr = start_addr;
    for (int i = 0; i < count; ++i) {
        if (addr >= flash.size()) {
            std::cout << "  (end of flash)\n";
            break;
        }
        u16 opcode = flash[addr];
        std::string_view mnemonic = AvrCpu::lookup_mnemonic(opcode);
        u8 wsize = AvrCpu::classify_word_size(opcode);

        std::snprintf(hexbuf, sizeof(hexbuf), "0x%04X:  %04X  ", static_cast<unsigned>(addr), static_cast<unsigned>(opcode));
        std::cout << "  " << Terminal::fg(Terminal::Color::bright_black) << hexbuf
                  << Terminal::reset_all()
                  << Terminal::fg(Terminal::Color::green) << std::left << std::setw(12) << std::setfill(' ') << mnemonic
                  << Terminal::reset_all();

        if (wsize == 2 && addr + 1 < flash.size()) {
            u16 opcode2 = flash[addr + 1];
            std::snprintf(hexbuf, sizeof(hexbuf), " %04X", static_cast<unsigned>(opcode2));
            std::cout << Terminal::fg(Terminal::Color::bright_black) << hexbuf
                      << Terminal::reset_all();
        }

        if (addr == pc) {
            std::cout << Terminal::fg(Terminal::Color::red) << "  ← PC"
                      << Terminal::reset_all();
        }
        std::cout << "\n";

        addr += wsize;
    }
}

void show_trace(DebugSession& session) {
    auto history = session.machine->trace_history();
    if (history.empty()) {
        std::cout << Terminal::fg(Terminal::Color::yellow) << "  (trace buffer empty — not enabled)"
                  << Terminal::reset_all() << "\n";
        std::cout << "  Enable with: trace on\n";
        return;
    }

    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ Trace History (" << history.size() << " snapshots) ═══"
              << Terminal::reset_all() << "\n";

    auto snapshots = history;
    // Show last 20
    size_t start = snapshots.size() > 20 ? snapshots.size() - 20 : 0;
    for (size_t i = start; i < snapshots.size(); ++i) {
        auto& s = snapshots[i];
        std::cout << "  " << Terminal::fg(Terminal::Color::bright_black) << "#"
                  << std::dec << i << Terminal::reset_all()
                  << "  PC=0x" << std::hex << s.program_counter
                  << "  SP=0x" << s.stack_pointer
                  << "  SREG=0x" << std::setw(2) << std::setfill('0') << static_cast<int>(s.sreg)
                  << "  Cycles=" << std::dec << s.cycles
                  << Terminal::reset_all() << "\n";
    }
}

void list_breakpoints(DebugSession& session) {
    if (session.breakpoints.empty()) {
        std::cout << "  (no breakpoints set)\n";
        return;
    }
    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ Breakpoints ═══" << Terminal::reset_all() << "\n";
    for (auto& bp : session.breakpoints) {
        std::cout << "  " << Terminal::fg(Terminal::Color::yellow) << "#" << bp.id
                  << Terminal::reset_all()
                  << "  0x" << std::hex << bp.address << std::dec
                  << (bp.enabled ? Terminal::fg(Terminal::Color::green) + "  enabled"
                                 : Terminal::fg(Terminal::Color::bright_black) + "  disabled")
                  << Terminal::reset_all() << "\n";
    }
}

bool check_breakpoint(DebugSession& session) {
    u32 pc = session.machine->cpu().program_counter();
    for (auto& bp : session.breakpoints) {
        if (bp.enabled && bp.address == pc) {
            std::cout << Terminal::fg(Terminal::Color::red) << Terminal::style(Terminal::Style::bold)
                      << "\n** Breakpoint #" << bp.id << " hit at 0x"
                      << std::hex << pc << std::dec << Terminal::reset_all() << "\n\n";
            return true;
        }
    }
    return false;
}

void show_stats(DebugSession& session) {
    auto& cpu = session.machine->cpu();
    auto snap = cpu.snapshot();
    u64 rss = process_rss_bytes();
    double ipc = snap.instructions_executed > 0
        ? static_cast<double>(snap.cycles) / static_cast<double>(snap.instructions_executed)
        : 0.0;

    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ Execution Statistics ═══" << Terminal::reset_all() << "\n";

    auto line = [](const char* label, auto val) {
        std::cout << "  " << Terminal::fg(Terminal::Color::bright_black) << label
                  << Terminal::reset_all() << val << "\n";
    };

    line("Cycles:      ", std::to_string(snap.cycles));
    line("Instructions:", std::to_string(snap.instructions_executed));
    line("IPC:         ", std::to_string(ipc));
    line("Sleep cycles:", std::to_string(snap.sleep_cycles));
    line("CPU cycles:  ", std::to_string(snap.cycles - snap.sleep_cycles) + " (active)");
    line("RSS memory:  ", std::to_string(rss / 1024) + " KB");

#ifdef VIOAVR_HAVE_JIT
    if (cpu.jit_enabled()) {
        auto jstats = cpu.jit_debug_stats();
        line("JIT blocks:  ", std::to_string(jstats.translate_count));
        line("JIT execs:   ", std::to_string(jstats.execute_count));
        line("JIT cycles:  ", std::to_string(jstats.execute_cycles));
    }
#endif

}

// ---------------------------------------------------------------------------
// REPL loop
// ---------------------------------------------------------------------------
void debug_repl(DebugSession& session) {
    g_trace_hook.show_trace = false;
    session.machine->cpu().set_trace_hook(&g_trace_hook);
    session.machine->cpu().halt();

    std::cout << Terminal::fg(Terminal::Color::green) << "  Debugger ready."
              << Terminal::reset_all() << " Type " << Terminal::fg(Terminal::Color::yellow) << "help"
              << Terminal::reset_all() << " for commands.\n\n";

    std::string line;
    while (session.running) {
        std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
                  << "vioavr-debug"
                  << Terminal::reset_all()
                  << ":" << Terminal::fg(Terminal::Color::bright_black) << "0x"
                  << std::hex << session.machine->cpu().program_counter()
                  << Terminal::reset_all() << "$ " << std::flush;

        if (!std::getline(std::cin, line)) {
            session.running = false;
            break;
        }

        // Trim and skip empty
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };
        trim(line);
        if (line.empty()) continue;

        // Parse tokens
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);

        auto& cmd = tokens[0];

        // quit/exit
        if (cmd == "quit" || cmd == "q" || cmd == "exit") {
            session.running = false;
            break;
        }

        // help
        if (cmd == "help" || cmd == "h") {
            std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
                      << "Commands:" << Terminal::reset_all() << "\n";
            auto c = [](const char* name, const char* desc) {
                std::cout << "  " << Terminal::fg(Terminal::Color::green) << name
                          << Terminal::reset_all() << "  " << desc << "\n";
            };
            c("regs / r",         "Show CPU registers (GPRs, PC, SP, SREG)");
            c("pins",              "Show GPIO pin states");
            c("step [N] / s [N]",  "Execute N instructions (default: 1)");
            c("continue / c",      "Run until breakpoint or halt");
            c("break <addr> / b <addr>",   "Set breakpoint at PC address");
            c("break list",        "List breakpoints");
            c("break clear <id>",  "Remove breakpoint");
            c("mem <addr> [bytes] / x <addr> [bytes]", "Examine data space memory");
            c("disasm [addr] [N] / d [addr] [N]", "Disassemble flash (default: PC, 8 insns)");
            c("trace on/off",      "Enable/disable instruction trace");
            c("stats / status",    "Show execution statistics (cycles, IPC, memory, JIT)");
            c("help / h",          "Show this help");
            c("quit / q / exit",   "Exit debugger");
            return;
        }

        // regs
        if (cmd == "regs" || cmd == "r") {
            show_regs(session);
            continue;
        }

        // pins
        if (cmd == "pins") {
            show_pins(session);
            continue;
        }

        // step
        if (cmd == "step" || cmd == "s") {
            int n = 1;
            if (tokens.size() > 1) {
                try { n = std::stoi(tokens[1]); } catch (...) {}
                if (n < 1) n = 1;
            }
            g_trace_hook.show_trace = true;
            for (int i = 0; i < n; ++i) {
                auto& cpu = session.machine->cpu();
                if (cpu.state() == CpuState::halted) {
                    std::cout << Terminal::fg(Terminal::Color::red) << "CPU is halted."
                              << Terminal::reset_all() << "\n";
                    break;
                }
                cpu.resume();
                session.machine->step();
                cpu.halt();
                ++session.step_count;
            }
            g_trace_hook.show_trace = false;
            // Show current instruction
            auto& cpu = session.machine->cpu();
            u32 pc = cpu.program_counter();
            auto flash = session.machine->bus().flash_words();
            if (pc < flash.size()) {
                u16 opcode = flash[pc];
                std::string_view mnemonic = AvrCpu::lookup_mnemonic(opcode);
                std::cout << "  → 0x" << std::hex << pc << ":  " << mnemonic
                          << Terminal::reset_all() << "\n";
            }
            continue;
        }

        // continue
        if (cmd == "continue" || cmd == "c") {
            auto& cpu = session.machine->cpu();
            if (cpu.state() == CpuState::halted) {
                std::cout << Terminal::fg(Terminal::Color::red) << "CPU is halted."
                          << Terminal::reset_all() << "\n";
                continue;
            }
            cpu.resume();
            while (cpu.state() != CpuState::halted) {
                if (session.breakpoints.empty()) {
                    // No breakpoints — run until halt
                    cpu.run(UINT64_MAX);
                    break;
                }
                // Step and check breakpoints
                session.machine->step();
                ++session.step_count;
                if (check_breakpoint(session)) {
                    cpu.halt();
                    break;
                }
            }
            if (cpu.state() == CpuState::halted) {
                std::cout << Terminal::fg(Terminal::Color::red) << "CPU halted."
                          << Terminal::reset_all() << "\n";
            }
            // Show current location
            u32 pc = cpu.program_counter();
            auto flash = session.machine->bus().flash_words();
            if (pc < flash.size()) {
                u16 opcode = flash[pc];
                std::string_view mnemonic = AvrCpu::lookup_mnemonic(opcode);
                std::cout << "  Stopped at 0x" << std::hex << pc << ":  " << mnemonic
                          << Terminal::reset_all() << "\n";
            }
            std::cout << "  Cycles: " << cpu.cycles() << "\n";
            continue;
        }

        // break
        if (cmd == "break" || cmd == "b") {
            if (tokens.size() < 2) {
                list_breakpoints(session);
                continue;
            }
            if (tokens[1] == "list") {
                list_breakpoints(session);
                continue;
            }
            if (tokens[1] == "clear" && tokens.size() > 2) {
                u64 id = 0;
                try { id = std::stoull(tokens[2]); } catch (...) {}
                auto it = std::find_if(session.breakpoints.begin(), session.breakpoints.end(),
                    [id](auto& bp) { return bp.id == id; });
                if (it != session.breakpoints.end()) {
                    std::cout << "  Removed breakpoint #" << id << " at 0x"
                              << std::hex << it->address << std::dec << "\n";
                    session.breakpoints.erase(it);
                } else {
                    std::cout << "  Breakpoint #" << id << " not found.\n";
                }
                continue;
            }
            // Set breakpoint
            u32 addr = 0;
            try { addr = static_cast<u32>(std::stoul(tokens[1], nullptr, 0)); } catch (...) {
                std::cout << "  Invalid address.\n";
                continue;
            }
            session.breakpoints.push_back({session.next_bp_id++, addr, true});
            std::cout << "  Breakpoint #" << (session.next_bp_id - 1) << " set at 0x"
                      << std::hex << addr << std::dec << "\n";
            continue;
        }

        // mem / x
        if (cmd == "mem" || cmd == "x") {
            u16 addr = 0;
            int count = 64;
            if (tokens.size() > 1) {
                try { addr = static_cast<u16>(std::stoul(tokens[1], nullptr, 0)); } catch (...) {}
            }
            if (tokens.size() > 2) {
                try { count = std::stoi(tokens[2]); } catch (...) {}
            }
            show_mem(session, addr, count);
            continue;
        }

        // disasm / d
        if (cmd == "disasm" || cmd == "d") {
            u32 addr = session.machine->cpu().program_counter();
            int count = 8;
            if (tokens.size() > 1) {
                try { addr = static_cast<u32>(std::stoul(tokens[1], nullptr, 0)); } catch (...) {}
            }
            if (tokens.size() > 2) {
                try { count = std::stoi(tokens[2]); } catch (...) {}
            }
            show_disasm(session, addr, count);
            continue;
        }

        // trace
        if (cmd == "trace") {
            if (tokens.size() > 1 && tokens[1] == "on") {
                g_trace_hook.show_trace = true;
                std::cout << "  Trace output enabled.\n";
            } else if (tokens.size() > 1 && tokens[1] == "off") {
                g_trace_hook.show_trace = false;
                std::cout << "  Trace output disabled.\n";
            } else {
                show_trace(session);
            }
            continue;
        }

        // stats / status
        if (cmd == "stats" || cmd == "status") {
            show_stats(session);
            continue;
        }

        // history / backtrace
        if (cmd == "history" || cmd == "backtrace") {
            show_trace(session);
            continue;
        }

        std::cout << Terminal::fg(Terminal::Color::red) << "Unknown command: "
                  << Terminal::reset_all() << cmd << "\n";
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------
int cmd_debug(const Args& args) {
    if (args.has("--help") || args.positional.empty()) {
        std::cout << Terminal::style(Terminal::Style::bold) << "Usage: "
                  << Terminal::reset_all() << Terminal::fg(Terminal::Color::yellow) << "vioavr debug"
                  << Terminal::reset_all() << " [options] <" << Terminal::fg(Terminal::Color::green)
                  << "hex_file" << Terminal::reset_all() << ">\n";
        auto opt = [](const char* flag, const char* desc) {
            std::cout << "  " << Terminal::fg(Terminal::Color::cyan) << flag
                      << Terminal::reset_all() << "  " << desc << "\n";
        };
        std::cout << Terminal::style(Terminal::Style::bold) << "Options:"
                  << Terminal::reset_all() << "\n";
        opt("--mcu <name>",      "MCU (default: ATmega328P)");
        opt("--color <mode>",    "Color mode: auto, always, never");
        opt("--help",            "Show this help");
        return args.positional.empty() ? 1 : 0;
    }

    std::string mcu = args.get("mcu", "ATmega328P");
    std::string hex = args.positional[0];

    DebugSession session;
    session.mcu = mcu;

    session.machine = Machine::create_for_device(mcu);
    if (!session.machine) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "MCU '" << mcu << "' not found in catalog.\n";
        return 1;
    }

    // Enable trace buffer
    session.machine->enable_trace_buffer(10000);

    auto& cpu = session.machine->cpu();
    auto& bus = session.machine->bus();

    try {
        auto image = HexImageLoader::load_file(hex, bus.device());
        bus.load_image(image);
        session.machine->reset();
    } catch (const std::exception& e) {
        std::cerr << Terminal::fg(Terminal::Color::red) << "Error: "
                  << Terminal::reset_all() << "loading HEX: " << e.what() << std::endl;
        return 2;
    }

    std::cout << Terminal::fg(Terminal::Color::cyan) << Terminal::style(Terminal::Style::bold)
              << "═══ VioAVR Interactive Debugger ═══" << Terminal::reset_all() << "\n";
    std::cout << "  " << Terminal::fg(Terminal::Color::bright_black) << "MCU:"
              << Terminal::reset_all() << " " << mcu
              << Terminal::fg(Terminal::Color::bright_black) << "  HEX:"
              << Terminal::reset_all() << " " << hex << "\n";

    debug_repl(session);
    return 0;
}
