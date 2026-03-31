#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include <array>

namespace vioavr::core {

class AvrCpu;

class DeviceDescriptor;

/**
 * @brief Bridge peripheral that maps CPU internal registers (SREG, SP) into the I/O space.
 */
class CpuControl final : public IoPeripheral {
public:
    explicit CpuControl(AvrCpu& cpu, const DeviceDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

private:
    AvrCpu& cpu_;
    std::array<AddressRange, 1> ranges_;
};

}  // namespace vioavr::core
