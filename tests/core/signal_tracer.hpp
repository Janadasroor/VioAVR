#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace vcd {

class SignalTracer {
public:
    explicit SignalTracer(std::string timescale = "1 us")
        : timescale_(std::move(timescale)) {}

    void add_signal(const std::string& name, int width = 1) {
        char id = next_id_++;
        signals_.push_back({name, width, id, std::string(width, 'x'),
                            std::string(width, 'x'), false});
        signal_map_[name] = static_cast<int>(signals_.size()) - 1;
    }

    void record(const std::string& name, uint64_t time, bool value) {
        _record(name, time, value ? "1" : "0");
    }

    void record(const std::string& name, uint64_t time, uint64_t value) {
        auto& s = signals_[signal_map_.at(name)];
        std::string bin(s.width, '0');
        for (int i = 0; i < s.width; ++i)
            if (value & (1ULL << (s.width - 1 - i))) bin[i] = '1';
        _record(name, time, bin);
    }

    bool dump(const std::string& filename) const {
        std::ofstream f(filename);
        if (!f) return false;

        f << "$timescale " << timescale_ << " $end\n";
        f << "$scope module top $end\n";
        for (auto& s : signals_)
            f << "$var wire " << s.width << " " << s.id
              << " " << s.name << " $end\n";
        f << "$upscope $end\n";
        f << "$enddefinitions $end\n";

        f << "$dumpvars\n";
        for (auto& s : signals_) {
            auto& val = s.initialized ? s.initial_value : s.last_value;
            if (s.width == 1)
                f << val << s.id << "\n";
            else
                f << "b" << val << " " << s.id << "\n";
        }
        f << "$end\n";

        std::sort(events_.begin(), events_.end(),
                  [](auto& a, auto& b) { return a.time < b.time; });

        uint64_t cur = 0;
        for (auto& ev : events_) {
            if (ev.time != cur) {
                f << "#" << ev.time << "\n";
                cur = ev.time;
            }
            if (ev.value.size() == 1)
                f << ev.value << ev.id << "\n";
            else
                f << "b" << ev.value << " " << ev.id << "\n";
        }
        return true;
    }

    void clear() {
        events_.clear();
        for (auto& s : signals_) {
            s.last_value = std::string(s.width, 'x');
            s.initialized = false;
        }
    }

private:
    void _record(const std::string& name, uint64_t time,
                 const std::string& value) {
        auto& s = signals_[signal_map_.at(name)];
        if (!s.initialized) {
            s.initialized = true;
            s.initial_value = value;
            s.last_value = value;
            return;  // dumpvars already reflects this
        }
        if (value != s.last_value) {
            s.last_value = value;
            events_.push_back({time, s.id, value});
        }
    }

    struct Signal {
        std::string name;
        int width;
        char id;
        std::string last_value;
        std::string initial_value;
        bool initialized;
    };
    struct Event {
        uint64_t time;
        char id;
        std::string value;
    };

    std::string timescale_;
    std::vector<Signal> signals_;
    std::map<std::string, int> signal_map_;
    mutable std::vector<Event> events_;
    char next_id_ = 33;
};

} // namespace vcd
