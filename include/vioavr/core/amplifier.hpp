#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <string>

namespace vioavr::core {

class At90Amplifier : public IoPeripheral {
public:
    At90Amplifier(std::string_view name, const AmplifierDescriptor& desc, PinMux& pin_mux) noexcept;

    std::string_view name() const noexcept override { return name_; }
    std::span<const AddressRange> mapped_ranges() const noexcept override;
    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] double voltage_out() const noexcept { return voltage_out_; }
    [[nodiscard]] bool is_enabled() const noexcept { return (ampcsr_ & desc_.ampen_mask) != 0; }

private:
    void evaluate() noexcept;
    void update_pin_ownership() noexcept;

    std::string name_;
    AmplifierDescriptor desc_;
    PinMux* pin_mux_;
    u8 ampcsr_{0};
    double voltage_out_{0.0};
    AddressRange range_;
};

} // namespace vioavr::core
