#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>

namespace vioavr::core {

class Usb8x final : public IoPeripheral {
public:
    explicit Usb8x(const Usb8xDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return "USB8x"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    [[nodiscard]] u8 fifo_count() const noexcept { return fifo_count_; }

    // Test helpers
    [[nodiscard]] const u8* setup_packet() const noexcept { return ep0_setup_pkt_; }
    [[nodiscard]] u8 bus_state() const noexcept { return busstate_ & 0x03U; }
    [[nodiscard]] bool ep0_stalled() const noexcept { return ep0_stalled_; }
    [[nodiscard]] u8 ep0_data_dir() const noexcept { return ep0_data_dir_; }
    [[nodiscard]] u16 ep0_wLength() const noexcept { return ep0_wLength_; }

private:
    void update_interrupt_state() noexcept;
    void detect_setup_packet() noexcept;
    void handle_bus_state_change(u8 old_state, u8 new_state) noexcept;

    Usb8xDescriptor desc_;
    std::array<AddressRange, 1> ranges_{};

    u8 ctrla_{0};
    u8 ctrlb_{0};
    u8 busstate_{0};
    u8 addr_{0};
    u8 fifowp_{0};
    u8 fiforp_{0};
    u8 epptr_{0};
    u8 intctrla_{0};
    u8 intctrlb_{0};
    u8 intflagsa_{0};
    u8 intflagsb_{0};
    u8 pllcsr_{0};
    u8 fifo_count_{0};
    u8 fifo_buf_[64]{};
    u8 fifo_wp_{0};
    u8 fifo_rp_{0};

    u64 sof_cycle_counter_{0};
    u64 sof_frame_cycles_{0};

    // EP0 control transfer state
    u8 ep0_setup_pkt_[8]{};
    u8 ep0_setup_count_{0};
    bool ep0_stalled_{false};
    u8 ep0_data_dir_{0}; // 0=OUT, 1=IN
    u16 ep0_wLength_{0};

    // Bus state tracking for edge detection
    u8 prev_bus_state_{0};
};

} // namespace vioavr::core
