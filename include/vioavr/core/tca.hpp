#pragma once

#include "vioavr/core/types.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>

namespace vioavr::core {

class MemoryBus;

/**
 * @brief 16-bit Timer/Counter Type A (TCA)
 * Found in modern AVR devices (AVR8X, megaAVR-0, tinyAVR-0/1/2)
 * Supports Single Mode (16-bit) and Split Mode (2x 8-bit)
 */
class Tca : public IoPeripheral {
public:
    explicit Tca(const TcaDescriptor& desc);

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] std::string_view name() const noexcept override { return "TCA"; }
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }

private:
    const TcaDescriptor desc_;
    MemoryBus* bus_ {nullptr};

    // Registers
    u8 ctrla_ {0x00};
    u8 ctrlb_ {0x00};
    u8 ctrlc_ {0x00};
    u8 ctrld_ {0x00};
    u8 evctrl_ {0x00};
    u8 intctrl_ {0x00};
    u8 intflags_ {0x00};
    u8 dbgctrl_ {0x00};
    u8 temp_ {0x00};

    // 16-bit Value Access
    u16 tcnt_ {0x0000};
    u16 period_ {0xFFFF};
    u16 cmp0_ {0x0000};
    u16 cmp1_ {0x0000};
    u16 cmp2_ {0x0000};

    // Buffers and Status
    bool counting_up_ {true};
    std::array<AddressRange, 4> ranges_ {};

    // Internal logic
    void handle_matches();
    void perform_tick();
    bool is_enabled() const noexcept { return ctrla_ & 0x01; }
    bool is_split_mode() const noexcept { return ctrld_ & 0x01; }
};

} // namespace vioavr::core
