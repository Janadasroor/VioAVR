#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"

namespace vioavr::core {

class Tcd final : public IoPeripheral {
public:
    explicit Tcd(const TcdDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "TCD"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

private:
    void update_interrupt_pending() noexcept;
    void run_counter(u64 cycles) noexcept;
    void on_compare(u16 cmp_value, bool is_set) noexcept;
    u8 reg_offset(u16 address) const noexcept;

    TcdDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};

    u8 ctrla_{0};
    u8 ctrlb_{0};
    u8 ctrlc_{0};
    u8 ctrld_{0};
    u8 ctrle_{0};
    u8 evctrla_{0};
    u8 evctrlB_{0};
    u8 intctrl_{0};
    u8 intflags_{0};
    u8 status_{0};
    u8 inputctrla_{0};
    u8 inputctrlb_{0};
    u8 faultctrl_{0};
    u8 dlyctrl_{0};
    u8 dlyval_{0};
    u8 ditctrl_{0};
    u8 ditval_{0};
    u8 dbgctrl_{0};
    u16 capturea_{0};
    u16 captureb_{0};
    u16 cmpaset_{0};
    u16 cmpacl_{0};
    u16 cmpbset_{0};
    u16 cmpbcl_{0};

    u16 counter_{0};
    u64 accumulated_cycles_{0};
    bool enabled_{false};
    bool int_pending_{false};
};

} // namespace vioavr::core
