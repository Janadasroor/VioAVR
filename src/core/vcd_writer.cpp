#include "vioavr/core/vcd_writer.hpp"
#include <ctime>
#include <iomanip>
#include <cstdint>

namespace vioavr::core {

VcdWriter::VcdWriter(const std::string& filename) : file_(filename) {
}

VcdWriter::~VcdWriter() {
    if (file_.is_open()) {
        file_ << "$dumpoff\n$end\n";
        file_.close();
    }
}

void VcdWriter::add_signal(const std::string& name, int width, const std::string& id) {
    signals_[id] = {name, width, 0xFFFFFFFFFFFFFFFFULL};
}

void VcdWriter::write_header() {
    std::time_t t = std::time(nullptr);
    file_ << "$date " << std::ctime(&t) << "$end\n";
    file_ << "$version VioAVR 1.0 $end\n";
    file_ << "$timescale 1ns $end\n";
    file_ << "$scope module top $end\n";
    
    for (const auto& [id, sig] : signals_) {
        file_ << "$var wire " << sig.width << " " << id << " " << sig.name << " $end\n";
    }
    
    file_ << "$upscope $end\n";
    file_ << "$enddefinitions $end\n";
    file_ << "$dumpvars\n";
    file_ << "$end\n";
}

void VcdWriter::update_signal(const std::string& id, uint64_t value) {
    auto it = signals_.find(id);
    if (it == signals_.end()) return;
    
    if (it->second.last_value != value) {
        if (it->second.width == 1) {
            file_ << value << id << "\n";
        } else {
            file_ << "b";
            for (int i = it->second.width - 1; i >= 0; i--) {
                file_ << ((value >> i) & 1);
            }
            file_ << " " << id << "\n";
        }
        it->second.last_value = value;
    }
}

void VcdWriter::next_timestamp(uint64_t ns) {
    if (ns > current_time_) {
        current_time_ = ns;
        file_ << "#" << current_time_ << "\n";
    }
}

} // namespace vioavr::core
