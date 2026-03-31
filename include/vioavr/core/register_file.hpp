#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include <array>

namespace vioavr::core {

class AvrCpu;

/**
 * @brief Bridge peripheral that maps CPU General Purpose Registers (R0-R31) into the data space.
 */
class RegisterFile final : public IoPeripheral {
public:
    explicit RegisterFile(AvrCpu& cpu) noexcept;

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
