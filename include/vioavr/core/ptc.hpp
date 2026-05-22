#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <cstdint>

namespace vioavr::core {

class Ptc final : public IoPeripheral {
public:
    explicit Ptc(const PtcDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "PTC"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void set_capacitance(u16 value) noexcept { capacitance_ = value; }

private:
    void update_interrupt_pending() noexcept;
    u8 reg_offset(u16 address) const noexcept;

    PtcDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};

    u8 ctrla_{0};
    u8 ctrlb_{0};
    u8 ctrlc_{0};
    u8 ctrld_{0};
    u8 evctrl_{0};
    u8 intctrl_{0};
    u8 intflags_{0};
    u8 dbgctrl_{0};
    u8 wcompctrl_{0};
    u16 result_{0};
    u8 intstat_{0};

    u64 conversion_cycles_{0};
    u64 conversion_start_cycle_{0};
    bool converting_{false};
    bool int_pending_{false};
    u16 capacitance_{0};
};

} // namespace vioavr::core
