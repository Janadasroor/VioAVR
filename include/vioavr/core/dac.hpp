#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <string_view>
#include <array>
#include <vector>

namespace vioavr::core {

class Dac final : public IoPeripheral {
public:
    Dac(std::string_view name, const DacDescriptor& desc);
    ~Dac() override = default;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    
    // DAC outputs a voltage [0.0, 1.0] relative to reference
    [[nodiscard]] double output_voltage() const noexcept { return voltage_; }

private:
    std::string_view name_;
    const DacDescriptor& desc_;
    std::vector<AddressRange> ranges_;
    MemoryBus* bus_ {nullptr};

    u8 dacon_ {0};
    u16 data_ {0};
    double voltage_ {0.0};

    [[nodiscard]] bool power_reduction_enabled() const noexcept;
    void update_voltage() noexcept;
};

} // namespace vioavr::core
