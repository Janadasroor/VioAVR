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
    explicit Tca(std::string name, const TcaDescriptor& desc);

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    [[nodiscard]] bool get_wo_level(u8 index) const noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_event_system(EventSystem* evsys) noexcept override;

private:
    std::string name_;
    const TcaDescriptor desc_;
    std::array<AddressRange, 8> ranges_ {};
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

    // Register State
    struct {
        u16 tcnt {0x0000};
        u16 per {0xFFFF};
        u16 cmp0 {0x0000};
        u16 cmp1 {0x0000};
        u16 cmp2 {0x0000};
    } norm_;

    // Buffer Registers
    struct {
        u16 per {0xFFFF};
        u16 cmp0 {0x0000};
        u16 cmp1 {0x0000};
        u16 cmp2 {0x0000};
        bool per_valid {false};
        bool cmp0_valid {false};
        bool cmp1_valid {false};
        bool cmp2_valid {false};
    } buf_;

    // Split Mode accessors (convenience)
    u8& cnt_l() { return reinterpret_cast<u8*>(&norm_.tcnt)[0]; }
    u8& cnt_h() { return reinterpret_cast<u8*>(&norm_.tcnt)[1]; }
    u8& per_l() { return reinterpret_cast<u8*>(&norm_.per)[0]; }
    u8& per_h() { return reinterpret_cast<u8*>(&norm_.per)[1]; }
    u8& cmp0_l() { return reinterpret_cast<u8*>(&norm_.cmp0)[0]; }
    u8& cmp0_h() { return reinterpret_cast<u8*>(&norm_.cmp0)[1]; }
    u8& cmp1_l() { return reinterpret_cast<u8*>(&norm_.cmp1)[0]; }
    u8& cmp1_h() { return reinterpret_cast<u8*>(&norm_.cmp1)[1]; }
    u8& cmp2_l() { return reinterpret_cast<u8*>(&norm_.cmp2)[0]; }
    u8& cmp2_h() { return reinterpret_cast<u8*>(&norm_.cmp2)[1]; }

    // Prescaler
    u32 prescaler_counter_ {0};
    u32 prescaler_limit_ {1};

    // Buffers and Status
    bool counting_up_ {true};

    // Internal logic
    void handle_matches();
    void perform_tick();
    void perform_tick_split();
    void on_event() noexcept;
    
    [[nodiscard]] bool is_enabled() const noexcept { return ctrla_ & 0x01; }
    [[nodiscard]] bool is_split_mode() const noexcept { return ctrld_ & 0x01; }
    void update_prescaler() noexcept;
};

} // namespace vioavr::core
