#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace vioavr::core {

/**
 * @brief A lightweight VCD (Value Change Dump) writer for VioAVR.
 */
class VcdWriter {
public:
    VcdWriter(const std::string& filename);
    ~VcdWriter();

    void add_signal(const std::string& name, int width, const std::string& id);
    void write_header();
    
    void update_signal(const std::string& id, uint64_t value);
    void next_timestamp(uint64_t ns);

private:
    std::ofstream file_;
    uint64_t current_time_ = 0;
    
    struct Signal {
        std::string name;
        int width;
        uint64_t last_value = 0xFFFFFFFFFFFFFFFFULL;
    };
    
    std::map<std::string, Signal> signals_;
};

} // namespace vioavr::core
