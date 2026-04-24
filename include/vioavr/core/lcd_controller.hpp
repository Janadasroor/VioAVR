#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <vector>
#include <string>

namespace vioavr::core {

/**
 * @brief Liquid Crystal Display (LCD) Controller
 * 
 * Supports driving external LCD glass with multiple common and segment lines.
 * Found in devices like ATmega169.
 */
class LcdController final : public IoPeripheral {
public:
    LcdController(std::string_view name, const LcdDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

private:
    std::string name_;
    LcdDescriptor desc_;
    std::vector<AddressRange> ranges_;

    u8 lcdcra_ {0U};
    u8 lcdcrb_ {0U};
    u8 lcdfrr_ {0U};
    u8 lcdccr_ {0U};
    std::vector<u8> lcddr_;

    bool interrupt_pending_ {false};
    u64 cycle_accumulator_ {0U};
    
    // LCD Control bits (ATmega169 values)
    static constexpr u8 kLcden  = 0x80U;
    static constexpr u8 kLcdab  = 0x40U;
    static constexpr u8 kLcdif  = 0x10U;
    static constexpr u8 kLcdie  = 0x08U;
    static constexpr u8 kLcdmux = 0x30U;
};

} // namespace vioavr::core
