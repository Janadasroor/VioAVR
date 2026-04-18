#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"

namespace vioavr::core {

/**
 * @brief Modern Interrupt Controller (AVR8X / megaAVR-0)
 */
class CpuInt : public IoPeripheral {
public:
    explicit CpuInt(const CpuIntDescriptor& desc);

    [[nodiscard]] std::string_view name() const noexcept override { return "CPUINT"; }

    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    void reset() noexcept override;
    void tick(u64) noexcept override {}
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    [[nodiscard]] bool is_lvl1_vector(u8 vector_index) const noexcept {
        bool match = (lvl1vec_ & 0x7FU) == vector_index && lvl1vec_ != 0;
        return match;
    }

    [[nodiscard]] u8 lvl0_priority_vector() const noexcept { return lvl0pri_; }
    [[nodiscard]] bool round_robin_enabled() const noexcept { return (ctrla_ & 0x01U) != 0; }
    [[nodiscard]] bool compact_vector_table() const noexcept { return (ctrla_ & 0x20U) != 0; }
    [[nodiscard]] bool ivsel() const noexcept { return (ctrla_ & 0x40U) != 0; }

    void set_executing_lvl0(bool active) noexcept {
        if (active) status_ |= 0x01U; else status_ &= ~0x01U;
    }
    void set_executing_lvl1(bool active) noexcept {
        if (active) status_ |= 0x02U; else status_ &= ~0x02U;
    }

private:
    CpuIntDescriptor desc_;
    u8 ctrla_{0};
    u8 status_{0};
    u8 lvl0pri_{0};
    u8 lvl1vec_{0};
    std::array<AddressRange, 1> ranges_{};
};

} // namespace vioavr::core
