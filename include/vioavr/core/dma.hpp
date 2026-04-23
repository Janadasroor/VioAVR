#pragma once

#include "vioavr/core/types.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class EventSystem;

/**
 * @brief Direct Memory Access (DMA) Controller.
 * Supports automated data transfers between memory and peripherals.
 * Modeled after the modern AVR (AVR-DA) series DMA architecture.
 */
class Dma : public IoPeripheral {
public:
    explicit Dma(const DmaDescriptor& desc);

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] std::string_view name() const noexcept override { return "DMA"; }
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;

private:
    const DmaDescriptor desc_;
    MemoryBus* bus_{nullptr};
    EventSystem* evsys_{nullptr};

    struct Channel {
        u8 ctrla{0};
        u8 ctrlb{0};
        u16 srcaddr{0};
        u16 dstaddr{0};
        u16 cnt{0};
        u16 remaining_count{0};
        u8 trigsrc{0};
        u8 intctrl{0};
        u8 intflags{0};
        
        bool busy{false};
        bool pending_trigger{false};
    };

    std::array<Channel, 4> channels_{};
    std::array<AddressRange, 8> ranges_{};

    // Global Registers
    u8 ctrla_{0};
    u8 status_{0};
    u8 intctrl_{0};
    u8 intflags_{0};
    u8 dbgctrl_{0};

    void perform_transfer(u8 channel_idx) noexcept;
    void on_trigger(u8 trigger_id) noexcept;
    
    [[nodiscard]] bool is_enabled() const noexcept { return ctrla_ & 0x01; }
};

} // namespace vioavr::core
