#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <vector>

namespace vioavr::core {

class Xmem final : public IoPeripheral {
public:
    explicit Xmem(const DeviceDescriptor& desc) noexcept;
    
    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    
    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool enabled() const noexcept { return enabled_; }
    
    // Memory access interface for external SRAM
    [[nodiscard]] u8 read_external(u16 address) noexcept;
    void write_external(u16 address, u8 value) noexcept;

    // CPU queries this to add wait states
    [[nodiscard]] u8 get_wait_states(u16 address) const noexcept;

private:
    const DeviceDescriptor& desc_;
    std::vector<AddressRange> ranges_;
    
    // Registers
    u8 xmcra_ {0x0U};
    u8 xmcrb_ {0x0U};
    bool enabled_ {false};
    
    // External SRAM data
    std::vector<u8> memory_;
};

} // namespace vioavr::core
