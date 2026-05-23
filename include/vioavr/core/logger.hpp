#pragma once

#include <string_view>
#include <string>
#include <cstdio>

namespace vioavr::core {

enum class LogLevel {
    debug,
    info,
    warning,
    error
};

/**
 * @brief Simple, decoupled logger for the core library.
 * 
 * In a real application, you might want to redirect this to a file
 * or a UI console. By default, it prints to std::cerr.
 */
class Logger {
public:
    using LogCallback = void (*)(LogLevel level, std::string_view message);

    static void set_callback(LogCallback callback) noexcept
    {
        callback_ = callback;
    }

    static void log(LogLevel level, std::string_view message)
    {
        if (callback_) [[likely]] {
            callback_(level, message);
        }
    }

    static void debug(std::string_view message) { log(LogLevel::debug, message); }
    static void info(std::string_view message) { log(LogLevel::info, message); }
    static void warning(std::string_view message) { log(LogLevel::warning, message); }
    static void error(std::string_view message) { log(LogLevel::error, message); }

    static std::string hex(uint32_t val) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%X", val);
        return std::string(buf);
    }

private:
    inline static LogCallback callback_ = nullptr;
};

}  // namespace vioavr::core
