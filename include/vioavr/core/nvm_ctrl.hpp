#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"

namespace vioavr::core {

/**
 * @brief Modern NVM Controller (AVR8X / megaAVR-0)
 */
class NvmCtrl : public IoPeripheral {
public:
    enum class Command : u8 {
        none = 0x00,
        page_write = 0x01,
        page_erase = 0x02,
        page_erase_write = 0x03,
        page_buf_clear = 0x04,
        chip_erase = 0x05,
        ee_erase = 0x06,
        fuse_write = 0x07
    };

    explicit NvmCtrl(const NvmCtrlDescriptor& desc);

    [[nodiscard]] std::string_view name() const noexcept override { return "NVMCTRL"; }

    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    void reset() noexcept override;
    void tick(u64) noexcept override {}
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void set_bus(class MemoryBus& bus) noexcept { bus_ = &bus; }

    [[nodiscard]] Command current_command() const noexcept { return static_cast<Command>(ctrla_ & 0x07U); }
    void clear_command() noexcept { ctrla_ &= ~0x07U; }

    [[nodiscard]] bool busy() const noexcept { return (status_ & 0x03U) != 0; }
    void set_busy(bool flash, bool ee) noexcept {
        status_ = (status_ & ~0x03U) | (flash ? 0x01U : 0x00U) | (ee ? 0x02U : 0x00U);
    }

    [[nodiscard]] u16 address() const noexcept { return addr_; }
    [[nodiscard]] u16 data() const noexcept { return data_; }

private:
    NvmCtrlDescriptor desc_;
    u8 ctrla_{0};
    u8 ctrlb_{0};
    u8 status_{0};
    u8 intctrl_{0};
    u8 intflags_{0};
    u16 addr_{0};
    u16 data_{0};
    class MemoryBus* bus_ {nullptr};
    std::array<AddressRange, 4> ranges_{};
};

} // namespace vioavr::core
