#pragma once

#include "vioavr/core/types.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class EventSystem;

class Dma : public IoPeripheral {
public:
    explicit Dma(const DmaDescriptor& desc);

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] std::string_view name() const noexcept override { return "DMA"; }
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

private:
    struct TransferState {
        u16 beats_remaining{0};
        u16 beats_in_burst{0};
        u8 block_size{1};
        u8 trigact{0};
        bool started{false};
        u64 cycle_accumulator{0};
    };

    struct Channel {
        u8 ctrla{0};
        u8 ctrlb{0};
        u8 addrctrl{0};
        u16 srcaddr{0};
        u16 dstaddr{0};
        u16 cnt{0};
        u16 remaining_count{0};
        u16 descaddr{0};
        u8 trigsrc{0};
        u8 intctrl{0};
        u8 intflags{0};

        bool busy{false};
        bool pending_trigger{false};
        TransferState xfer{};
    };

    const DmaDescriptor desc_;
    MemoryBus* bus_{nullptr};
    EventSystem* evsys_{nullptr};

    std::array<Channel, 4> channels_{};
    std::array<AddressRange, 12> ranges_{};

    u8 ctrla_{0};
    u8 status_{0};
    u8 intctrl_{0};
    u8 intflags_{0};
    u8 dbgctrl_{0};

    bool int_pending_{false};

    void start_transfer(u8 channel_idx) noexcept;
    void process_transfer_beats(u8 channel_idx) noexcept;
    void try_reload_descriptor(u8 channel_idx) noexcept;
    void on_trigger(u8 trigger_id) noexcept;

    [[nodiscard]] bool is_enabled() const noexcept { return ctrla_ & 0x01; }
    [[nodiscard]] bool reload_enabled(u8 ctrlb) const noexcept { return (ctrlb & 0x80U) != 0; }
};

} // namespace vioavr::core