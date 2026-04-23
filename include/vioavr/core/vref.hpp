#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>

namespace vioavr::core {

/// Voltage Reference (AVR8X — VREF)
/// Controls reference voltage selection for the ADC and Analog Comparator.
/// The emulator provides a `reference_voltage()` query so ADC8x/AC8x can
/// pick up the correct reference without hard-coding voltage values.
class VRef : public IoPeripheral {
public:
    // CTRLA bit layout (ATmega4809)
    static constexpr u8 AC0REFSEL_MASK  = 0x07U; ///< Analog Comparator 0 Vref select [2:0]
    static constexpr u8 ADC0REFSEL_MASK = 0x70U; ///< ADC0 Vref select [6:4]

    // CTRLB bit layout
    static constexpr u8 ADC0REFEN_MASK = 0x02U; ///< ADC0 reference output enable
    static constexpr u8 AC0REFEN_MASK  = 0x01U; ///< AC0 reference output enable

    /// Reference select encoding → voltage in V (ATmega4809 Table 22-3)
    static constexpr double ref_voltages[8] = {
        0.55, 1.1, 1.5, 2.5, 4.3, 5.0/*VDD*/, 5.0, 5.0
    };

    explicit VRef(const VrefDescriptor& desc) noexcept : desc_(desc) {
        if (desc_.ctrla_address != 0U) {
            ranges_[0] = {desc_.ctrla_address, desc_.ctrla_address};
            if (desc_.ctrlb_address == desc_.ctrla_address + 1U)
                ranges_[0].end = desc_.ctrlb_address;
            else if (desc_.ctrlb_address != 0U)
                ranges_[1] = {desc_.ctrlb_address, desc_.ctrlb_address};
        }
    }

    [[nodiscard]] std::string_view name() const noexcept override { return "VREF"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override {
        size_t count = 0;
        while (count < ranges_.size() && ranges_[count].begin != 0U) ++count;
        return {ranges_.data(), count};
    }
    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    void reset() noexcept override { ctrla_ = 0U; ctrlb_ = 0U; }
    void tick(u64) noexcept override {}

    u8 read(u16 address) noexcept override {
        if (address == desc_.ctrla_address) return ctrla_;
        if (address == desc_.ctrlb_address) return ctrlb_;
        return 0U;
    }

    void write(u16 address, u8 value) noexcept override {
        if (address == desc_.ctrla_address) ctrla_ = value;
        else if (address == desc_.ctrlb_address) ctrlb_ = value;
    }

    /// Returns the configured ADC0 reference voltage in Volts.
    [[nodiscard]] double adc0_reference_voltage(double vdd = 5.0) const noexcept {
        u8 sel = (ctrla_ & ADC0REFSEL_MASK) >> 4U;
        if (sel == 5U) return vdd;  // VDD
        return (sel < 8U) ? ref_voltages[sel] : vdd;
    }

    /// Returns the configured AC0 reference voltage in Volts.
    [[nodiscard]] double ac0_reference_voltage(double vdd = 5.0) const noexcept {
        u8 sel = ctrla_ & AC0REFSEL_MASK;
        if (sel == 5U) return vdd;
        return (sel < 8U) ? ref_voltages[sel] : vdd;
    }

    [[nodiscard]] bool adc0_ref_output_enabled() const noexcept { return (ctrlb_ & ADC0REFEN_MASK) != 0U; }
    [[nodiscard]] bool ac0_ref_output_enabled()  const noexcept { return (ctrlb_ & AC0REFEN_MASK)  != 0U; }

private:
    const VrefDescriptor desc_;
    std::array<AddressRange, 2> ranges_{};
    u8 ctrla_{0U};
    u8 ctrlb_{0U};
};

} // namespace vioavr::core
