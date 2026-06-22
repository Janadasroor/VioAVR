#pragma once

#include "vioavr/core/machine.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/uart8x.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/adc8x.hpp"
#include "vioavr/core/analog_signal_bank.hpp"

#include "ansi.hpp"
#include "args.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <sstream>
#include <cctype>

using namespace vioavr::core;

// ── Data structures ──

struct TimedStimulus {
    u64 at_cycle;
    std::function<void(Machine&)> apply;
};

struct StimulusConfig {
    std::vector<TimedStimulus> timed_stimuli;
    std::vector<u8> pending_uart_bytes;  // UART RX bytes buffered until UART is ready
    size_t uart_progress = 0;            // how many bytes from pending_uart_bytes have been injected
};

// ── Parsing ──

// Parse --pin PORTB.3=1 or PORTB.3=0 or PORTB.3=1@50000
// Comma-separated: --pin PORTB.3=1,PORTC.0=0@10000
// Port name can be "PORTB" or just "B"
static std::optional<std::pair<std::string, u8>> parse_pin_spec(const std::string& spec) {
    auto dot = spec.find('.');
    if (dot == std::string::npos) return std::nullopt;
    std::string port_name = spec.substr(0, dot);
    if (port_name.size() == 1) port_name = "PORT" + port_name;
    else if (port_name.size() > 4 && port_name.substr(0, 4) != "PORT") return std::nullopt;
    else if (port_name.size() <= 4 && port_name.substr(0, 1) != "P") port_name = "PORT" + port_name;
    std::string bit_str = spec.substr(dot + 1);
    char* end = nullptr;
    long bit = std::strtol(bit_str.c_str(), &end, 10);
    if (end != bit_str.c_str() + bit_str.size() || bit < 0 || bit > 7) return std::nullopt;
    return {{port_name, static_cast<u8>(bit)}};
}

// Parse --adc 0=2.5 or 1=1.8@100000
// Comma-separated: --adc 0=2.5,1=1.8@50000
static bool parse_stimulus_flag(const std::string& key, const std::string& value, StimulusConfig& config) {
    if (key == "pin") {
        // Parse comma-separated pin stimuli
        std::istringstream ss(value);
        std::string item;
        while (std::getline(ss, item, ',')) {
            // Format: PORTB.3=1[@cycle]
            auto at_pos = item.find('@');
            u64 at_cycle = 0;
            std::string core = (at_pos != std::string::npos) ? item.substr(0, at_pos) : item;
            if (at_pos != std::string::npos) {
                at_cycle = Args::parse_cycles(item.substr(at_pos + 1));
            }
            auto eq_pos = core.find('=');
            if (eq_pos == std::string::npos) {
                std::cerr << "  Invalid --pin format: " << item << " (expected PORT.X=0|1)\n";
                return false;
            }
            std::string pin_spec = core.substr(0, eq_pos);
            std::string val_str = core.substr(eq_pos + 1);
            int level = std::strtol(val_str.c_str(), nullptr, 10);
            if (level != 0 && level != 1) {
                std::cerr << "  Invalid --pin value: " << val_str << " (expected 0 or 1)\n";
                return false;
            }
            auto pin = parse_pin_spec(pin_spec);
            if (!pin) {
                std::cerr << "  Invalid --pin spec: " << pin_spec << " (expected PORT.X)\n";
                return false;
            }
            config.timed_stimuli.push_back({at_cycle, [pin, level](Machine& m) {
                auto* port = m.get_port(pin->first);
                if (!port) {
                    std::cerr << "  Unknown port: " << pin->first << "\n";
                    return;
                }
                m.bus().propagate_external_pin_change(
                    port->pin_address(), pin->second,
                    level ? PinLevel::high : PinLevel::low);
            }});
        }
        return true;
    }
    else if (key == "adc") {
        std::istringstream ss(value);
        std::string item;
        while (std::getline(ss, item, ',')) {
            auto at_pos = item.find('@');
            u64 at_cycle = 0;
            std::string core = (at_pos != std::string::npos) ? item.substr(0, at_pos) : item;
            if (at_pos != std::string::npos) {
                at_cycle = Args::parse_cycles(item.substr(at_pos + 1));
            }
            auto eq_pos = core.find('=');
            if (eq_pos == std::string::npos) {
                std::cerr << "  Invalid --adc format: " << item << " (expected channel=voltage)\n";
                return false;
            }
            long channel = std::strtol(core.substr(0, eq_pos).c_str(), nullptr, 10);
            double voltage = std::strtod(core.substr(eq_pos + 1).c_str(), nullptr);
            if (channel < 0 || channel > 127) {
                std::cerr << "  Invalid ADC channel: " << channel << "\n";
                return false;
            }
            config.timed_stimuli.push_back({at_cycle, [channel, voltage](Machine& m) {
                // Set voltage on analog signal bank (for modern ADCs)
                m.analog_signal_bank().set_voltage(
                    static_cast<u8>(channel), voltage);
                // Also try classic Adc directly
                auto adcs = m.peripherals_of_type<Adc>();
                for (auto* adc : adcs) {
                    adc->set_channel_voltage(static_cast<u8>(channel), voltage);
                }
            }});
        }
        return true;
    }
    else if (key == "uart-rx") {
        // Parse hex string: 48656C6C6F → bytes [0x48, 0x65, 0x6C, 0x6C, 0x6F]
        std::string hex = value;
        hex.erase(std::remove_if(hex.begin(), hex.end(), ::isspace), hex.end());
        if (hex.size() % 2 != 0) {
            std::cerr << "  Invalid --uart-rx hex string (must have even length)\n";
            return false;
        }
        for (size_t i = 0; i + 1 < hex.size(); i += 2) {
            u8 byte = static_cast<u8>(std::strtoul(hex.substr(i, 2).c_str(), nullptr, 16));
            config.pending_uart_bytes.push_back(byte);
        }
        return true;
    }
    return false;
}

// Parse all stimulus flags from Args
static StimulusConfig parse_stimulus_config(const Args& args) {
    StimulusConfig config;
    for (const auto& [key, value] : args.options) {
        parse_stimulus_flag(key, value, config);
    }
    // Sort by at_cycle for efficient timed dispatch
    std::sort(config.timed_stimuli.begin(), config.timed_stimuli.end(),
        [](const TimedStimulus& a, const TimedStimulus& b) {
            return a.at_cycle < b.at_cycle;
        });
    return config;
}

// Apply all stimuli that are due at or before the given cycle
// Returns index of next stimulus to apply
static size_t apply_due_stimuli(Machine& machine, StimulusConfig& config,
                                 u64 current_cycle, size_t next_idx) {
    while (next_idx < config.timed_stimuli.size() &&
           config.timed_stimuli[next_idx].at_cycle <= current_cycle) {
        config.timed_stimuli[next_idx].apply(machine);
        next_idx++;
    }
    // Retry UART RX injection on every call — buffer stays until UART is ready
    if (config.uart_progress < config.pending_uart_bytes.size()) {
        auto uarts = machine.peripherals_of_type<Uart>();
        auto uarts8x = machine.peripherals_of_type<Uart8x>();
        while (config.uart_progress < config.pending_uart_bytes.size()) {
            u8 byte = config.pending_uart_bytes[config.uart_progress];
            bool ok = false;
            for (auto* u : uarts) {
                if (u->inject_received_byte(byte)) { ok = true; break; }
            }
            if (!ok) {
                for (auto* u : uarts8x) {
                    u->inject_received_byte(byte);
                    ok = true;
                    break;
                }
            }
            if (ok) {
                config.uart_progress++;
            } else {
                break; // UART not ready yet, retry next call
            }
        }
    }
    return next_idx;
}
