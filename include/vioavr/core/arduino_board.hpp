#pragma once

#include "vioavr/core/types.hpp"
#include <algorithm>
#include <array>
#include <cctype>
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
    std::string_view bootloader_hex; // relative path in arduino core bootloaders/
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
        kUnoPins,
        {},
        "optiboot/optiboot_atmega328.hex"
    },
    {
        "Nano",
        "arduino:avr:nano",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 8, true,
        kNanoPins,
        {},
        "optiboot/optiboot_atmega328.hex"
    },
    {
        "Mega2560",
        "arduino:avr:mega",
        "ATmega2560",
        16'000'000,
        8192, 262144,
        13, 16, true,
        kMegaPins,
        {},
        "stk500v2/stk500boot_v2_mega2560.hex"
    },
    {
        "Leonardo",
        "arduino:avr:leonardo",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins,
        {},
        "caterina/Caterina-Leonardo.hex"
    },
    {
        "Micro",
        "arduino:avr:micro",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins,
        {},
        "caterina/Caterina-Micro.hex"
    },
    {
        "Nano Every",
        "arduino:megaavr:nona4809",
        "ATmega4809",
        20'000'000,
        6144, 49152,
        13, 8, true,
        kNanoEveryPins,
        {},
        "atmega4809_uart_bl.hex"
    },
    {
        "Uno WiFi Rev2",
        "arduino:megaavr:uno2018",
        "ATmega4809",
        16'000'000,
        6144, 49152,
        25, 8, true,
        {},
        {},
        "atmega4809_uart_bl.hex"
    },
    {
        "Pro Mini 16V",
        "arduino:avr:pro",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 8, true,
        kUnoPins,
        {},
        "atmega/ATmegaBOOT_168_atmega328.hex"
    },
    {
        "Pro Mini 8V",
        "arduino:avr:pro",
        "ATmega328P",
        8'000'000,
        2048, 32256,
        13, 8, true,
        kUnoPins,
        {},
        "atmega/ATmegaBOOT_168_atmega328_pro_8MHz.hex"
    },
    {
        "Mini",
        "arduino:avr:mini",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 8, true,
        kUnoPins,
        {},
        "optiboot/optiboot_atmega328-Mini.hex"
    },
    {
        "Ethernet",
        "arduino:avr:ethernet",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins,
        {},
        "optiboot/optiboot_atmega328.hex"
    },
    {
        "Duemilanove",
        "arduino:avr:diecimila",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins,
        {},
        "atmega/ATmegaBOOT_168_atmega328.hex"
    },
    {
        "Fio",
        "arduino:avr:fio",
        "ATmega328P",
        8'000'000,
        2048, 32256,
        13, 8, true,
        kUnoPins,
        {},
        "atmega/ATmegaBOOT_168_atmega328_pro_8MHz.hex"
    },
    {
        "LilyPad",
        "arduino:avr:lilypad",
        "ATmega328P",
        8'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins,
        {},
        "atmega/ATmegaBOOT_168_atmega328_pro_8MHz.hex"
    },
    {
        "LilyPad USB",
        "arduino:avr:LilyPadUSB",
        "ATmega32U4",
        8'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins,
        {},
        "caterina-LilyPadUSB/Caterina-LilyPadUSB.hex"
    },
    {
        "Gemma",
        "arduino:avr:gemma",
        "ATmega328P",
        8'000'000,
        2048, 32256,
        1, 3, false,
        {},
        ""
    },
    {
        "Esplora",
        "arduino:avr:esplora",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 6, true,
        kLeonardoPins,
        {},
        "caterina/Caterina-Esplora.hex"
    },
    {
        "Yun",
        "arduino:avr:yun",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins,
        {},
        "caterina/Caterina-Yun.hex"
    },
    {
        "Mega ADK",
        "arduino:avr:megaADK",
        "ATmega2560",
        16'000'000,
        8192, 262144,
        13, 16, true,
        kMegaPins,
        {},
        "stk500v2/stk500boot_v2_mega2560.hex"
    },
    {
        "Robot Control",
        "arduino:avr:robotControl",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 6, true,
        kLeonardoPins,
        {},
        "caterina-Arduino_Robot/Caterina-Robot-Control.hex"
    },
    {
        "Robot Motor",
        "arduino:avr:robotMotor",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 6, true,
        kLeonardoPins,
        {},
        "caterina-Arduino_Robot/Caterina-Robot-Motor.hex"
    },
    {
        "BT",
        "arduino:avr:bt",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins,
        {},
        "bt/ATmegaBOOT_168_atmega328_bt.hex"
    },
    {
        "Industrial 101",
        "arduino:avr:chiwawa",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 6, true,
        kLeonardoPins,
        {},
        "caterina/Caterina-Industrial101.hex"
    },
    {
        "Leonardo ETH",
        "arduino:avr:leonardoeth",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins,
        {},
        "caterina/Caterina-LeonardoEthernet.hex"
    },
    {
        "NG (ATmega8)",
        "arduino:avr:atmegang",
        "ATmega8",
        16'000'000,
        1024, 8192,
        13, 6, true,
        kUnoPins,
        "cpu=atmega8",
        "atmega8/ATmegaBOOT-prod-firmware-2009-11-07.hex"
    },
    {
        "NG (ATmega168)",
        "arduino:avr:atmegang",
        "ATmega168",
        16'000'000,
        1024, 16384,
        13, 6, true,
        kUnoPins,
        "cpu=atmega168",
        "atmega/ATmegaBOOT_168_ng.hex"
    },
    {
        "UNO Mini",
        "arduino:avr:unomini",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins,
        {},
        "optiboot/optiboot_atmega328.hex"
    },
    {
        "UNO WiFi",
        "arduino:avr:unowifi",
        "ATmega328P",
        16'000'000,
        2048, 32256,
        13, 6, true,
        kUnoPins,
        {},
        "optiboot/optiboot_atmega328.hex"
    },
    {
        "Yun Mini",
        "arduino:avr:yunmini",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins,
        {},
        "caterina/Caterina-YunMini.hex"
    },
    {
        "Linino One",
        "arduino:avr:one",
        "ATmega32U4",
        16'000'000,
        2560, 28672,
        13, 12, true,
        kLeonardoPins,
        {},
        "caterina/Caterina-LininoOne.hex"
    },
    {
        "Circuit Playground",
        "arduino:avr:circuitplay32u4cat",
        "ATmega32U4",
        8'000'000,
        2560, 28672,
        13, 7, true,
        kLeonardoPins,
        {},
        "caterina/Caterina-Circuitplay32u4.hex"
    },
};

[[nodiscard]] inline bool iequals(std::string_view a, std::string_view b) noexcept {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](char ca, char cb) { return std::tolower(static_cast<unsigned char>(ca)) == std::tolower(static_cast<unsigned char>(cb)); });
}

[[nodiscard]] inline const ArduinoBoard*
find_arduino_board(std::string_view name_or_fqbn) noexcept {
    for (const auto& board : kArduinoBoards) {
        if (iequals(board.name, name_or_fqbn) || iequals(board.fqbn, name_or_fqbn))
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
