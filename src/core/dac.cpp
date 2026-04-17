#include "vioavr/core/dac.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <algorithm>

namespace vioavr::core {

Dac::Dac(std::string_view name, const DacDescriptor& desc)
    : name_(name), desc_(desc)
{
    std::vector<u16> addrs = {
        desc.dacon_address, desc.dacl_address, desc.dach_address
    };
    std::sort(addrs.begin(), addrs.end());
    for (u16 addr : addrs) {
        if (addr == 0) continue;
        if (!ranges_.empty() && addr == ranges_.back().end + 1) {
            ranges_.back().end = addr;
        } else {
            ranges_.push_back({addr, addr});
        }
    }
}

std::span<const AddressRange> Dac::mapped_ranges() const noexcept {
    return {ranges_.data(), ranges_.size()};
}

void Dac::reset() noexcept {
    dacon_ = 0;
    data_ = 0;
    voltage_ = 0.0;
}

void Dac::tick(u64 elapsed_cycles) noexcept {
    (void)elapsed_cycles;
    // DAC is purely combinatorial for now or updates on register write
}

u8 Dac::read(u16 address) noexcept {
    if (address == desc_.dacon_address) return dacon_;
    if (address == desc_.dacl_address) return data_ & 0xFF;
    if (address == desc_.dach_address) return (data_ >> 8) & 0x03;
    return 0;
}

void Dac::write(u16 address, u8 value) noexcept {
    if (address == desc_.dacon_address) {
        dacon_ = value;
        update_voltage();
    } else if (address == desc_.dacl_address) {
        data_ = (data_ & 0xFF00) | value;
        update_voltage();
    } else if (address == desc_.dach_address) {
        data_ = (data_ & 0x00FF) | (static_cast<u16>(value & 0x03) << 8);
        update_voltage();
    }
}

bool Dac::power_reduction_enabled() const noexcept {
    if (!bus_ || desc_.pr_address == 0 || desc_.pr_bit == 0xFF) return false;
    return (bus_->read_data(desc_.pr_address) & (1 << desc_.pr_bit)) != 0;
}

void Dac::update_voltage() noexcept {
    if (power_reduction_enabled() || !(dacon_ & 0x01)) { // Assuming DAEN is bit 0
        voltage_ = 0.0;
        return;
    }
    // Convert 10-bit data to 0.0-1.0 range
    voltage_ = static_cast<double>(data_) / 1023.0;
}

} // namespace vioavr::core
