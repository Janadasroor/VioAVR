#pragma once

#include <string>
#include <string_view>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#ifndef _WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#endif
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define fileno _fileno
#define isatty _isatty
#endif

// ---------------------------------------------------------------------------
// ANSI terminal control — colors, cursor, screen, progress bars
// ---------------------------------------------------------------------------

class Terminal {
public:
    enum class Color { black, red, green, yellow, blue, magenta, cyan, white, reset, bright_black, bright_red, bright_green, bright_yellow, bright_blue, bright_magenta, bright_cyan, bright_white };
    enum class Style { none, bold, dim, italic, underline, blink, reverse, hidden, strikethrough };

    // ── mode ────────────────────────────────────────────────
    enum class ColorMode { auto_detect, always, never };
    static ColorMode color_mode;
    static void set_color_mode(ColorMode m) { color_mode = m; }
    static ColorMode parse_color_mode(std::string_view s) {
        if (s == "always" || s == "on" || s == "yes") return ColorMode::always;
        if (s == "never" || s == "off" || s == "no")  return ColorMode::never;
        return ColorMode::auto_detect;
    }

    // ── detection ───────────────────────────────────────────
    static bool is_tty() {
        static bool cached = []{ return isatty(fileno(stdout)) != 0; }();
        return cached;
    }

    static bool wants_color() {
        if (color_mode == ColorMode::always) return true;
        if (color_mode == ColorMode::never)  return false;
        // auto: enable if TTY and not NO_COLOR
        static bool cached = []{
            if (!is_tty()) return false;
            if (getenv("NO_COLOR") && getenv("NO_COLOR")[0] != '\0') return false;
            // FORCE_COLOR overrides
            if (getenv("FORCE_COLOR") && getenv("FORCE_COLOR")[0] != '\0') return true;
            return true;
        }();
        return cached;
    }

    static bool enabled() { return wants_color(); }

    // ── terminal size ───────────────────────────────────────
    static int width() {
        if (!is_tty()) return 80;
        if (auto e = getenv("COLUMNS")) return std::atoi(e);
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE &&
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
            return csbi.srWindow.Right - csbi.srWindow.Left + 1;
        return 80;
#else
        struct winsize w;
        if (ioctl(fileno(stdout), TIOCGWINSZ, &w) == 0 && w.ws_col > 0) return w.ws_col;
        return 80;
#endif
    }

    static int height() {
        if (!is_tty()) return 24;
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE &&
            GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
            return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return 24;
#else
        struct winsize w;
        if (ioctl(fileno(stdout), TIOCGWINSZ, &w) == 0 && w.ws_row > 0) return w.ws_row;
        return 24;
#endif
    }

    // ── ANSI helpers ────────────────────────────────────────
    static std::string esc(const char* code) {
        if (!enabled()) return {};
        return std::string("\033[") + code + "m";
    }

    static std::string seq(const char* code) {
        if (!enabled()) return {};
        return std::string("\033[") + code;
    }

    // ── styles ──────────────────────────────────────────────
    static std::string fg(Color c) {
        if (!enabled()) return {};
        switch (c) {
            case Color::black:   return esc("30");
            case Color::red:     return esc("31");
            case Color::green:   return esc("32");
            case Color::yellow:  return esc("33");
            case Color::blue:    return esc("34");
            case Color::magenta: return esc("35");
            case Color::cyan:    return esc("36");
            case Color::white:   return esc("37");
            case Color::reset:   return esc("0");
            case Color::bright_black:   return esc("90");
            case Color::bright_red:     return esc("91");
            case Color::bright_green:   return esc("92");
            case Color::bright_yellow:  return esc("93");
            case Color::bright_blue:    return esc("94");
            case Color::bright_magenta: return esc("95");
            case Color::bright_cyan:    return esc("96");
            case Color::bright_white:   return esc("97");
        }
        return {};
    }

    static std::string bg(Color c) {
        if (!enabled()) return {};
        switch (c) {
            case Color::black:   return esc("40");
            case Color::red:     return esc("41");
            case Color::green:   return esc("42");
            case Color::yellow:  return esc("43");
            case Color::blue:    return esc("44");
            case Color::magenta: return esc("45");
            case Color::cyan:    return esc("46");
            case Color::white:   return esc("47");
            case Color::reset:   return esc("0");
            case Color::bright_black:   return esc("100");
            case Color::bright_red:     return esc("101");
            case Color::bright_green:   return esc("102");
            case Color::bright_yellow:  return esc("103");
            case Color::bright_blue:    return esc("104");
            case Color::bright_magenta: return esc("105");
            case Color::bright_cyan:    return esc("106");
            case Color::bright_white:   return esc("107");
        }
        return {};
    }

    static std::string style(Style s) {
        if (!enabled()) return {};
        switch (s) {
            case Style::none:         return esc("0");
            case Style::bold:         return esc("1");
            case Style::dim:          return esc("2");
            case Style::italic:       return esc("3");
            case Style::underline:    return esc("4");
            case Style::blink:        return esc("5");
            case Style::reverse:      return esc("7");
            case Style::hidden:       return esc("8");
            case Style::strikethrough: return esc("9");
        }
        return {};
    }

    static std::string reset_all() { return esc("0"); }

    // ── colored text helpers ────────────────────────────────
    static std::string colored(std::string_view text, Color c) {
        if (!enabled()) return std::string(text);
        return fg(c) + std::string(text) + fg(Color::reset);
    }

    static std::string styled(std::string_view text, Style s) {
        if (!enabled()) return std::string(text);
        return style(s) + std::string(text) + style(Style::none);
    }

    static std::string highlighted(std::string_view text, Color fg_c, Color bg_c) {
        if (!enabled()) return std::string(text);
        return fg(fg_c) + bg(bg_c) + std::string(text) + reset_all();
    }

    // ── cursor ──────────────────────────────────────────────
    static std::string cursor_up(int n = 1)    { return seq((std::to_string(n) + "A").c_str()); }
    static std::string cursor_down(int n = 1)  { return seq((std::to_string(n) + "B").c_str()); }
    static std::string cursor_fwd(int n = 1)   { return seq((std::to_string(n) + "C").c_str()); }
    static std::string cursor_back(int n = 1)  { return seq((std::to_string(n) + "D").c_str()); }
    static std::string cursor_pos(int row, int col) {
        return seq((std::to_string(row) + ";" + std::to_string(col) + "H").c_str());
    }
    static std::string cursor_save()           { return seq("s"); }
    static std::string cursor_restore()        { return seq("u"); }
    static std::string cursor_hide()           { return seq("?25l"); }
    static std::string cursor_show()           { return seq("?25h"); }
    static std::string cursor_next_line(int n) { return seq((std::to_string(n) + "E").c_str()); }
    static std::string cursor_prev_line(int n) { return seq((std::to_string(n) + "F").c_str()); }

    // ── screen ──────────────────────────────────────────────
    static std::string clear_screen() { return seq("2J") + cursor_pos(1, 1); }
    static std::string clear_line()   { return seq("2K"); }
    static std::string clear_to_eol() { return seq("K"); }

    // ── progress bar ────────────────────────────────────────
    static std::string progress_bar(double fraction, int bar_width = 40) {
        int filled = static_cast<int>(fraction * bar_width);
        if (filled < 0) filled = 0;
        if (filled > bar_width) filled = bar_width;
        std::string bar;
        bar += fg(Color::cyan);
        bar += '[';
        bar += fg(Color::green);
        for (int i = 0; i < bar_width; ++i) {
            bar += (i < filled) ? "█" : "░";
        }
        bar += fg(Color::cyan);
        bar += ']';
        bar += fg(Color::reset);
        return bar;
    }

    static std::string progress_label(double fraction, const char* label = nullptr) {
        std::string s;
        if (label) {
            s += fg(Color::bright_black);
            s += label;
            s += ' ';
            s += fg(Color::reset);
        }
        s += progress_bar(fraction);
        s += fg(Color::yellow);
        s += " " + std::to_string(static_cast<int>(fraction * 100)) + "%";
        s += fg(Color::reset);
        return s;
    }
};

// ── color mode ──────────────────────────────────────────────
inline Terminal::ColorMode Terminal::color_mode = Terminal::ColorMode::auto_detect;

// ── stream operators ────────────────────────────────────────
inline std::ostream& operator<<(std::ostream& os, Terminal::Color c) {
    if (Terminal::enabled()) os << Terminal::fg(c);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, Terminal::Style s) {
    if (Terminal::enabled()) os << Terminal::style(s);
    return os;
}

// ── convenience formatters ──────────────────────────────────
struct TermColor {
    Terminal::Color c;
    explicit TermColor(Terminal::Color c_) : c(c_) {}
};
inline std::ostream& operator<<(std::ostream& os, TermColor tc) {
    return os << tc.c;
}

inline auto term_red     = TermColor(Terminal::Color::red);
inline auto term_green   = TermColor(Terminal::Color::green);
inline auto term_yellow  = TermColor(Terminal::Color::yellow);
inline auto term_blue    = TermColor(Terminal::Color::blue);
inline auto term_cyan    = TermColor(Terminal::Color::cyan);
inline auto term_magenta = TermColor(Terminal::Color::magenta);
inline auto term_reset   = TermColor(Terminal::Color::reset);
