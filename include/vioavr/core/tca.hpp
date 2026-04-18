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
    void set_event_system(EventSystem* evsys) noexcept override { evsys_ = evsys; }

private:
    const TcaDescriptor desc_;
    MemoryBus* bus_ {nullptr};
    EventSystem* evsys_ {nullptr};

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

    // 16-bit Value Access (Normal Mode)
    u16 tcnt_ {0x0000};
    u16 period_ {0xFFFF};
    u16 cmp0_ {0x0000};
    u16 cmp1_ {0x0000};
    u16 cmp2_ {0x0000};

    // 8-bit Value Access (Split Mode)
    struct {
        u8 cnt_l {0};
        u8 per_l {0xFF};
        u8 cmp0_l {0};
        u8 cmp1_l {0};
        u8 cmp2_l {0};

        u8 cnt_h {0};
        u8 per_h {0xFF};
        u8 cmp0_h {0};
        u8 cmp1_h {0};
        u8 cmp2_h {0};
    } split_;

    // Prescaler
    u32 prescaler_counter_ {0};
    u32 prescaler_limit_ {1};

    // Buffers and Status
    bool counting_up_ {true};
    std::array<AddressRange, 4> ranges_ {};

    // Internal logic
    void handle_matches();
    void perform_tick();
    void perform_tick_split();
    
    [[nodiscard]] bool is_enabled() const noexcept { return ctrla_ & 0x01; }
    [[nodiscard]] bool is_split_mode() const noexcept { return ctrld_ & 0x01; }
    void update_prescaler() noexcept;
};

} // namespace vioavr::core
