#include "vioavr/core/device_catalog.hpp"
#include <iostream>
#include <iomanip>

int main() {
    using namespace vioavr::core;
    const auto* dev = DeviceCatalog::find("atmega328p");
    if (!dev) return 1;
    
    std::cout << "Timer16 count: " << (int)dev->timer16_count << std::endl;
    for(int i=0; i<dev->timer16_count; ++i) {
        auto& t = dev->timers16[i];
        std::cout << "Timer " << i << ":" << std::endl;
        std::cout << "  TCNT: 0x" << std::hex << t.tcnt_address << std::endl;
        std::cout << "  TCCRB: 0x" << std::hex << t.tccrb_address << std::endl;
        std::cout << "  CS Mask: 0x" << std::hex << (int)t.cs_mask << std::endl;
    }
    return 0;
}
