#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/pin_map.hpp"
#include "vioavr/core/tracing.hpp"
#include "vioavr/core/types.hpp"

#include <span>
#include <vector>

namespace vioavr::core {

class MemoryBus {
public:
    static constexpr u16 kLowIoBase = 0x20U;
    static constexpr u16 kLowIoSize = 0x40U;

    explicit MemoryBus(const DeviceDescriptor& device);

    void reset() noexcept;
    void set_trace_hook(ITraceHook* trace_hook) noexcept;
    void set_pin_map(PinMap* pin_map) noexcept;
    void attach_peripheral(IoPeripheral& peripheral);
    void load_flash(std::span<const u16> words);
    void load_image(const HexImage& image);

    [[nodiscard]] constexpr const DeviceDescriptor& device() const noexcept
    {
        return device_;
    }

    [[nodiscard]] constexpr std::span<const u16> flash_words() const noexcept
    {
        return flash_;
    }

    [[nodiscard]] constexpr std::span<u8> data_space() noexcept
    {
        return data_;
    }

    [[nodiscard]] constexpr u32 flash_size_words() const noexcept
    {
        return static_cast<u32>(flash_.size());
    }

    [[nodiscard]] constexpr std::span<const u8> data_space() const noexcept
    {
        return data_;
    }

    [[nodiscard]] constexpr u16 read_program_word(u32 word_address) const noexcept
    {
        if (flash_rww_busy_ && word_address <= device_.flash_rww_end_word) {
            return 0xFFFFU;
        }
        return word_address < flash_.size() ? flash_[word_address] : 0U;
    }

    [[nodiscard]] constexpr u8 read_program_byte(u32 byte_address) const noexcept
    {
        const u32 word_address = byte_address >> 1U;
        if (flash_rww_busy_ && word_address <= device_.flash_rww_end_word) {
            return 0xFFU;
        }
        if (word_address >= flash_.size()) {
            return 0U;
        }
        const u16 word = flash_[word_address];
        return static_cast<u8>(((byte_address & 0x01U) == 0U) ? (word & 0x00FFU) : (word >> 8U));
    }

    void write_program_word(u32 word_address, u16 value) noexcept;

    [[nodiscard]] constexpr u32 loaded_program_words() const noexcept
    {
        return loaded_program_words_;
    }

    [[nodiscard]] constexpr u32 reset_word_address() const noexcept
    {
        return reset_word_address_;
    }

    [[nodiscard]] static constexpr u16 low_io_address(u8 io_offset) noexcept
    {
        return static_cast<u16>(kLowIoBase + io_offset);
    }

    [[nodiscard]] std::span<IoPeripheral* const> peripherals() const noexcept
    {
        return peripherals_;
    }

    [[nodiscard]] u8 read_data(u16 address) noexcept;
    void write_data(u16 address, u8 value) noexcept;
    void tick_peripherals(u64 elapsed_cycles, u8 active_domains = 0xFFU) noexcept;
    [[nodiscard]] bool consume_pin_change(PinStateChange& change) noexcept;
    void propagate_external_pin_change(u32 external_id, PinLevel level) noexcept;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept;
    void set_flash_rww_busy(bool busy) noexcept { flash_rww_busy_ = busy; }
    [[nodiscard]] bool flash_rww_busy() const noexcept { return flash_rww_busy_; }

    void set_xmem(class Xmem* xmem) noexcept { xmem_ = xmem; }
    [[nodiscard]] class Xmem* xmem() const noexcept { return xmem_; }
    
    [[nodiscard]] u8 get_wait_states(u16 address) const noexcept;

private:
    [[nodiscard]] IoPeripheral* find_peripheral(u16 address) noexcept;
    [[nodiscard]] const IoPeripheral* find_peripheral(u16 address) const noexcept;

    const DeviceDescriptor& device_;
    ITraceHook* trace_hook_ {};
    PinMap* pin_map_ {};
    std::vector<u16> flash_;
    std::vector<u8> data_;
    bool flash_rww_busy_ {false};
    Xmem* xmem_ {nullptr};
    std::vector<IoPeripheral*> peripherals_ {};
    std::vector<IoPeripheral*> dispatch_table_ {};
    u32 loaded_program_words_ {};
    u32 reset_word_address_ {};
};
}  // namespace vioavr::core
