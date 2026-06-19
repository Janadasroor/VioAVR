#pragma once

#include "vioavr/core/types.hpp"
#include <array>
#include <string_view>
#include <span>
#include <vector>

namespace vioavr::core {

struct ArduinoPin {
    u8 arduino_pin;
    char port;
    u8 bit;
    std::string_view label;
};

struct ArduinoBoard {
    std::string_view name;
    std::string_view fqbn;
    std::string_view mcu;
    uint32_t f_cpu;
    u16 sram_bytes;
    u32 flash_bytes;
    u8 led_builtin;
    u8 analog_inputs;
    bool has_serial;
    std::span<const ArduinoPin> important_pins;
    std::string_view default_board_options;
};

inline constexpr ArduinoPin kUnoPins[] = {
    { 0, 'D', 0, "RX"  },
    { 1, 'D', 1, "TX"  },
    { 13, 'B', 5, "LED" },
    { 18, 'D', 3, "SCL" }, // A5
    { 19, 'D', 2, "SDA" }, // A4
};

inline constexpr ArduinoPin kNanoPins[] = {
    { 0, 'D', 0, "RX"  },
    { 1, 'D', 1, "TX"  },
    { 13, 'B', 5, "LED" },
    { 18, 'D', 3, "SCL" }, // A5
    { 19, 'D', 2, "SDA" }, // A4
};

inline constexpr ArduinoPin kMegaPins[] = {
    { 0, 'E', 0, "RX0"  },
    { 1, 'E', 1, "TX0"  },
    { 13, 'B', 7, "LED" },
    { 14, 'J', 1, "TX3" },
    { 15, 'J', 0, "RX3" },
    { 16, 'H', 1, "TX2" },
    { 17, 'H', 0, "RX2" },
    { 18, 'D', 3, "TX1" },
    { 19, 'D', 2, "RX1" },
    { 20, 'D', 1, "SDA" },
    { 21, 'D', 0, "SCL" },
};

inline constexpr ArduinoPin kLeonardoPins[] = {
    { 0, 'D', 2, "RX"  },
    { 1, 'D', 3, "TX"  },
    { 13, 'C', 7, "LED" },
    { 2, 'D', 1, "SDA" },
    { 3, 'D', 0, "SCL" },
};

inline constexpr ArduinoPin kNanoEveryPins[] = {
    { 0, 'B', 0, "RX"  },
    { 1, 'B', 1, "TX"  },
    { 13, 'F', 5, "LED" },
};

inline constexpr ArduinoBoard kArduinoBoards[] = {
    {
        "Uno",
        "arduino:avr:uno",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins
    },
    {
        "Nano",
        "arduino:avr:nano",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 8, true,
        kNanoPins
    },
    {
        "Mega2560",
        "arduino:avr:mega",
        "ATmega2560",
        16'000'000,
        8192, 262144,
        13, 16, true,
        kMegaPins
    },
    {
        "Leonardo",
        "arduino:avr:leonardo",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins
    },
    {
        "Micro",
        "arduino:avr:micro",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins
    },
    {
        "Nano Every",
        "arduino:megaavr:nona4809",
        "ATmega4809",
        20'000'000,
        6144, 49152,
        13, 8, true,
        kNanoEveryPins
    },
    {
        "Uno WiFi Rev2",
        "arduino:megaavr:uno2018",
        "ATmega4809",
        16'000'000,
        6144, 49152,
        25, 8, true,
        {}
    },
    {
        "Pro Mini 16V",
        "arduino:avr:pro",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 8, true,
        kUnoPins
    },
    {
        "Pro Mini 8V",
        "arduino:avr:pro",
        "ATmega328P",
        8'000'000,
        2048, 32256,
        13, 8, true,
        kUnoPins
    },
    {
        "Mini",
        "arduino:avr:mini",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 8, true,
        kUnoPins
    },
    {
        "Ethernet",
        "arduino:avr:ethernet",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins
    },
    {
        "Duemilanove",
        "arduino:avr:diecimila",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins
    },
    {
        "Fio",
        "arduino:avr:fio",
        "ATmega328P",
        8'000'000,
        2048, 32256,
        13, 8, true,
        kUnoPins
    },
    {
        "LilyPad",
        "arduino:avr:lilypad",
        "ATmega328P",
        8'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins
    },
    {
        "LilyPad USB",
        "arduino:avr:LilyPadUSB",
        "ATmega32U4",
        8'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins
    },
    {
        "Gemma",
        "arduino:avr:gemma",
        "ATmega328P",
        8'000'000,
        2048, 32256,
        1, 3, false,
        {}
    },
    {
        "Esplora",
        "arduino:avr:esplora",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 6, true,
        kLeonardoPins
    },
    {
        "Yun",
        "arduino:avr:yun",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins
    },
    {
        "Mega ADK",
        "arduino:avr:megaADK",
        "ATmega2560",
        16'000'000,
        8192, 262144,
        13, 16, true,
        kMegaPins
    },
    {
        "Robot Control",
        "arduino:avr:robotControl",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 6, true,
        kLeonardoPins
    },
    {
        "Robot Motor",
        "arduino:avr:robotMotor",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 6, true,
        kLeonardoPins
    },
    {
        "BT",
        "arduino:avr:bt",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins
    },
    {
        "Industrial 101",
        "arduino:avr:chiwawa",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 6, true,
        kLeonardoPins
    },
    {
        "Leonardo ETH",
        "arduino:avr:leonardoeth",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins
    },
    {
        "NG (ATmega8)",
        "arduino:avr:atmegang",
        "ATmega8",
        16'000'000,
        1024, 8192,
        13, 6, true,
        kUnoPins,
        "cpu=16MHzatmega8"
    },
    {
        "NG (ATmega168)",
        "arduino:avr:atmegang",
        "ATmega168",
        16'000'000,
        1024, 16384,
        13, 6, true,
        kUnoPins,
        "cpu=16MHzatmega168"
    },
    {
        "UNO Mini",
        "arduino:avr:unomini",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins
    },
    {
        "UNO WiFi",
        "arduino:avr:unowifi",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins
    },
    {
        "Yun Mini",
        "arduino:avr:yunmini",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins
    },
    {
        "Linino One",
        "arduino:avr:one",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins
    },
    {
        "Circuit Playground",
        "arduino:avr:circuitplay32u4cat",
        "ATmega32U4",
        8'000'000,
        2560, 28672,
        13, 7, true,
        kLeonardoPins
    },
};

[[nodiscard]] inline const ArduinoBoard*
find_arduino_board(std::string_view name_or_fqbn) noexcept {
    for (const auto& board : kArduinoBoards) {
        if (board.name == name_or_fqbn || board.fqbn == name_or_fqbn)
            return &board;
    }
    return nullptr;
}

[[nodiscard]] inline std::vector<std::string_view>
list_arduino_boards() noexcept {
    std::vector<std::string_view> names;
    names.reserve(std::size(kArduinoBoards));
    for (const auto& board : kArduinoBoards)
        names.push_back(board.name);
    return names;
}

} // namespace vioavr::core
