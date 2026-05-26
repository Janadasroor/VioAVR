#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <string>

namespace vioavr::core {

class Usi final : public IoPeripheral {
public:
    explicit Usi(std::string_view name, const UsiDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

private:
    void update_interrupt_pending() noexcept;
    bool is_twowire() const noexcept;
    u8 reg_offset(u16 address) const noexcept;

    std::string name_;
    UsiDescriptor desc_;
    std::array<AddressRange, 4> ranges_{};
    u8 range_count_{0};

    u8 usicr_{0};
    u8 usisr_{0};
    u8 usidr_{0};
    u8 usibr_{0};
    bool int_pending_{false};
    bool prev_sda_{false};
    bool prev_scl_{false};
    bool prev_do_{false};
    bool sda_driven_{false};
};

} // namespace vioavr::core
