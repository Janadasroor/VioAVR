#pragma once

#include "vioavr/core/types.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "ansi.hpp"

using namespace vioavr::core;

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

inline bool is_boolean_flag(std::string_view key) {
    return key == "help" || key == "h" || key == "summary" || key == "show-state" ||
           key == "verbose" || key == "quiet" || key == "no-jit" ||
           key == "no-registers" || key == "mem-writes" || key == "search";
}

inline Args parse_args(int argc, char** argv) {
    Args args;
    int i = 1;
    if (i < argc && argv[i][0] != '-') {
        args.subcommand = argv[i++];
    }
    while (i < argc) {
        std::string_view arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            args.options["--help"] = "true";
            ++i;
        } else if (arg.substr(0, 2) == "--") {
            std::string key(arg.substr(2));
            if (!is_boolean_flag(key) && i + 1 < argc && argv[i + 1][0] != '-') {
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
            std::string key(1, arg[1]);
            if (!is_boolean_flag(key) && i + 1 < argc && argv[i + 1][0] != '-') {
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
