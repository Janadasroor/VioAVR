#pragma once

#include <string_view>
#include <iostream>

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
        if (callback_) {
            callback_(level, message);
        } else {
            default_log(level, message);
        }
    }

    static void debug(std::string_view message) { log(LogLevel::debug, message); }
    static void info(std::string_view message) { log(LogLevel::info, message); }
    static void warning(std::string_view message) { log(LogLevel::warning, message); }
    static void error(std::string_view message) { log(LogLevel::error, message); }

private:
    static void default_log(LogLevel level, std::string_view message)
    {
        const char* level_str = "INFO";
        switch (level) {
            case LogLevel::debug: level_str = "DEBUG"; break;
            case LogLevel::info: level_str = "INFO"; break;
            case LogLevel::warning: level_str = "WARN"; break;
            case LogLevel::error: level_str = "ERROR"; break;
        }
        std::cerr << "[" << level_str << "] " << message << std::endl;
    }

    inline static LogCallback callback_ = nullptr;
};

}  // namespace vioavr::core
