#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"

namespace vioavr::core {

/**
 * @brief Operational Amplifier (OPAMP) peripheral.
 */
class Opamp final : public IoPeripheral {
public:
    explicit Opamp(const OpampDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "OPAMP"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

private:
    OpampDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};
    
    u8 ctrla_{0};
    u8 ctrlb_{0};
    u8 resctrl_{0};
    u8 muxctrl_{0};
};

} // namespace vioavr::core
