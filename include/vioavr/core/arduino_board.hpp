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

// Full Arduino pin mappings (all pins exposed on board headers)
// Correct pin mappings verified against official Arduino core variants (pins_arduino.h) on 2026-06-28
inline constexpr ArduinoPin kUnoFullPins[] = {
    { 0, 'D', 0, "RX" }, { 1, 'D', 1, "TX" },
    { 2, 'D', 2, "INT0" }, { 3, 'D', 3, "INT1/OC2B" },
    { 4, 'D', 4, "" }, { 5, 'D', 5, "OC0B" },
    { 6, 'D', 6, "OC0A" }, { 7, 'D', 7, "" },
    { 8, 'B', 0, "ICP1" }, { 9, 'B', 1, "OC1A" },
    { 10, 'B', 2, "OC1B/SS" }, { 11, 'B', 3, "MOSI/OC2A" },
    { 12, 'B', 4, "MISO" }, { 13, 'B', 5, "LED/SCK" },
    { 14, 'C', 0, "A0" }, { 15, 'C', 1, "A1" },
    { 16, 'C', 2, "A2" }, { 17, 'C', 3, "A3" },
    { 18, 'C', 4, "A4/SDA" }, { 19, 'C', 5, "A5/SCL" },
};

inline constexpr ArduinoPin kMegaFullPins[] = {
    { 0, 'E', 0, "RX0" }, { 1, 'E', 1, "TX0" },
    { 2, 'E', 4, "INT4" }, { 3, 'E', 5, "INT5/OC3B" },
    { 4, 'G', 5, "OC0B" }, { 5, 'E', 3, "OC3B" },
    { 6, 'H', 3, "OC4A" }, { 7, 'H', 4, "OC4B" },
    { 8, 'H', 5, "OC4C" }, { 9, 'H', 6, "OC2B" },
    { 10, 'B', 4, "OC2A/SS" }, { 11, 'B', 5, "OC1A" },
    { 12, 'B', 6, "OC1B" }, { 13, 'B', 7, "LED/OC0A" },
    { 14, 'J', 1, "TX3" }, { 15, 'J', 0, "RX3" },
    { 16, 'H', 1, "TX2" }, { 17, 'H', 0, "RX2" },
    { 18, 'D', 3, "TX1/INT3" }, { 19, 'D', 2, "RX1/INT2" },
    { 20, 'D', 1, "INT1/SDA" }, { 21, 'D', 0, "INT0/SCL" },
    { 22, 'A', 0, "D22" }, { 23, 'A', 1, "D23" },
    { 24, 'A', 2, "D24" }, { 25, 'A', 3, "D25" },
    { 26, 'A', 4, "D26" }, { 27, 'A', 5, "D27" },
    { 28, 'A', 6, "D28" }, { 29, 'A', 7, "D29" },
    { 30, 'C', 7, "D30" }, { 31, 'C', 6, "D31" },
    { 32, 'C', 5, "D32" }, { 33, 'C', 4, "D33" },
    { 34, 'C', 3, "D34" }, { 35, 'C', 2, "D35" },
    { 36, 'C', 1, "D36" }, { 37, 'C', 0, "D37" },
    { 38, 'D', 7, "D38" }, { 39, 'G', 2, "D39" },
    { 40, 'G', 1, "D40" }, { 41, 'G', 0, "D41" },
    { 42, 'L', 7, "D42" }, { 43, 'L', 6, "D43" },
    { 44, 'L', 5, "D44" }, { 45, 'L', 4, "D45" },
    { 46, 'L', 3, "D46" }, { 47, 'L', 2, "D47" },
    { 48, 'L', 1, "D48" }, { 49, 'L', 0, "D49" },
    { 50, 'B', 3, "MISO" }, { 51, 'B', 2, "MOSI" },
    { 52, 'B', 1, "SCK" }, { 53, 'B', 0, "SS" },
    { 54, 'F', 0, "A0" }, { 55, 'F', 1, "A1" },
    { 56, 'F', 2, "A2" }, { 57, 'F', 3, "A3" },
    { 58, 'F', 4, "A4" }, { 59, 'F', 5, "A5" },
    { 60, 'F', 6, "A6" }, { 61, 'F', 7, "A7" },
    { 62, 'K', 0, "A8" }, { 63, 'K', 1, "A9" },
    { 64, 'K', 2, "A10" }, { 65, 'K', 3, "A11" },
    { 66, 'K', 4, "A12" }, { 67, 'K', 5, "A13" },
    { 68, 'K', 6, "A14" }, { 69, 'K', 7, "A15" },
};

inline constexpr ArduinoPin kLeonardoFullPins[] = {
    { 0, 'D', 2, "RX/INT2" }, { 1, 'D', 3, "TX/INT3" },
    { 2, 'D', 1, "SDA/INT1" }, { 3, 'D', 0, "SCL/OC0B/INT0" },
    { 4, 'D', 4, "" }, { 5, 'C', 6, "OC3A" },
    { 6, 'D', 7, "OC0D" }, { 7, 'E', 6, "INT6/AIN0" },
    { 8, 'B', 4, "SS" }, { 9, 'B', 5, "OC1A/OC4B" },
    { 10, 'B', 6, "OC1B/OC4C" }, { 11, 'B', 7, "OC0A/OC1C" },
    { 12, 'D', 6, "OC4D/AIN1" }, { 13, 'C', 7, "LED/OC4C" },
    { 14, 'B', 3, "MISO" }, { 15, 'B', 1, "SCK" },
    { 16, 'B', 2, "MOSI" }, { 17, 'B', 0, "SS/RXLED" },
    { 18, 'F', 7, "A0" }, { 19, 'F', 6, "A1" },
    { 20, 'F', 5, "A2" }, { 21, 'F', 4, "A3" },
    { 22, 'F', 1, "A4" }, { 23, 'F', 0, "A5" },
    { 24, 'D', 4, "A6" }, { 25, 'D', 7, "A7" },
    { 26, 'B', 4, "A8" }, { 27, 'B', 5, "A9" },
    { 28, 'B', 6, "A10" }, { 29, 'D', 6, "A11" },
    { 30, 'D', 5, "TXLED" },
};

inline constexpr ArduinoPin kNanoEveryFullPins[] = {
    { 0, 'C', 5, "RX" }, { 1, 'C', 4, "TX" },
    { 2, 'A', 0, "D2" }, { 3, 'F', 5, "D3" },
    { 4, 'C', 6, "D4" }, { 5, 'B', 2, "D5" },
    { 6, 'F', 4, "D6" }, { 7, 'A', 1, "D7" },
    { 8, 'E', 3, "D8" }, { 9, 'B', 0, "D9" },
    { 10, 'B', 1, "D10" }, { 11, 'E', 0, "D11" },
    { 12, 'E', 1, "D12" }, { 13, 'E', 2, "LED" },
    { 14, 'D', 3, "A0" }, { 15, 'D', 2, "A1" },
    { 16, 'D', 1, "A2" }, { 17, 'D', 0, "A3" },
    { 18, 'F', 2, "A4" }, { 19, 'F', 3, "A5" },
    { 20, 'D', 4, "A6" }, { 21, 'D', 5, "A7" },
    { 22, 'A', 2, "SDA" }, { 23, 'A', 3, "SCL" },
    { 24, 'B', 5, "RXDEBUG" }, { 25, 'B', 4, "TXDEBUG" },
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
