#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>

namespace vioavr::core {

/// Brown-Out Detector / Voltage Level Monitor (AVR8X — BOD)
///
/// Emulates the VLM (Voltage Level Monitor) portion of the BOD peripheral.
/// The actual brown-out reset behaviour is intentionally omitted since the
/// emulator does not model power supply fluctuations.  The VLMCTRLA/INTCTRL
/// registers are emulated fully so firmware that polls INTFLAGS.VLMIF or uses
/// the VLM interrupt vector works correctly.
class BodCtrl : public IoPeripheral {
public:
    // INTCTRL bitmasks
    static constexpr u8 VLMIE_MASK  = 0x01U; ///< VLM Interrupt Enable
    static constexpr u8 VLMCFG_MASK = 0x06U; ///< VLM trigger config [2:1]
    
    // INTFLAGS bitmask
    static constexpr u8 VLMIF_MASK  = 0x01U; ///< VLM Interrupt Flag

    // VLMCTRLA levels
    static constexpr u8 VLMLVL_MASK = 0x03U;
    /// VLM level threshold voltages (fraction of VDD)
    static constexpr double vlm_thresholds[4] = { 0.05, 0.25, 0.45, 0.0 };

    explicit BodCtrl(const BodDescriptor& desc) noexcept : desc_(desc) {
        if (desc_.ctrla_address != 0U) {
            // Registers at ctrla(+0), ctrlb(+1) then vlmctrla(+8), intctrl(+9), intflags(+A), status(+B)
            ranges_[0] = {desc_.ctrla_address, static_cast<u16>(desc_.ctrla_address + 1U)};
            if (desc_.vlmctrla_address != 0U)
                ranges_[1] = {desc_.vlmctrla_address, static_cast<u16>(desc_.vlmctrla_address + 3U)};
        }
    }

    [[nodiscard]] std::string_view name() const noexcept override { return "BOD"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override {
        size_t count = 0;
        while (count < ranges_.size() && ranges_[count].begin != 0U) ++count;
        return {ranges_.data(), count};
    }
    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    void reset() noexcept override {
        ctrla_    = 0x05U; // Reset: SLEEP=01 (ENABLED), ACTIVE=01 (ENABLED), SAMPFREQ=0
        ctrlb_    = 0x00U;
        vlmctrla_ = 0x00U;
        intctrl_  = 0x00U;
        intflags_ = 0x00U;
        status_   = 0x00U;
        pending_  = false;
    }

    void tick(u64) noexcept override {
        // VLM interrupt logic is driven externally via `update_vdd()`
    }

    u8 read(u16 address) noexcept override {
        if (address == desc_.ctrla_address)    return ctrla_;
        if (address == desc_.ctrlb_address)    return ctrlb_;
        if (address == desc_.vlmctrla_address) return vlmctrla_;
        if (address == desc_.intctrl_address)  return intctrl_;
        if (address == desc_.intflags_address) return intflags_;
        if (address == desc_.status_address)   return status_;
        return 0U;
    }

    void write(u16 address, u8 value) noexcept override {
        if (address == desc_.ctrla_address)    { ctrla_ = value; return; }
        if (address == desc_.ctrlb_address)    { ctrlb_ = value; return; }
        if (address == desc_.vlmctrla_address) { vlmctrla_ = value; return; }
        if (address == desc_.intctrl_address)  { intctrl_  = value; return; }
        if (address == desc_.intflags_address) {
            // Clear-by-writing-1
            intflags_ &= ~value;
            pending_ = ((intflags_ & VLMIF_MASK) != 0U) && ((intctrl_ & VLMIE_MASK) != 0U);
            return;
        }
    }

    /// Call each simulation step with the current VDD in Volts (e.g. 5.0V).
    void update_vdd(double vdd, double vdd_nominal = 5.0) noexcept {
        u8 lvl = vlmctrla_ & VLMLVL_MASK;
        double threshold = vdd_nominal * (1.0 - vlm_thresholds[lvl]);
        bool below = (vdd < threshold);

        u8 cfg = (intctrl_ & VLMCFG_MASK) >> 1U;
        // cfg 0=falling(below), 1=rising(above), 2=both
        bool trigger = false;
        if (cfg == 0U)      trigger = below && !prev_below_;
        else if (cfg == 1U) trigger = !below && prev_below_;
        else                trigger = (below != prev_below_);
        prev_below_ = below;

        if (trigger) {
            intflags_ |= VLMIF_MASK;
            // STATUS bit 0 = VLMS (voltage level monitor status — 0=above, 1=below)
            status_ = below ? 0x01U : 0x00U;
            if (intctrl_ & VLMIE_MASK) pending_ = true;
        }
    }

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& req) const noexcept override {
        if (!pending_) return false;
        req = InterruptRequest{ .vector_index = desc_.vlm_vector_index, .source_id = 0U };
        return true;
    }

    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& req) noexcept override {
        if (!pending_) return false;
        req = InterruptRequest{ .vector_index = desc_.vlm_vector_index, .source_id = 0U };
        pending_ = false;
        return true;
    }

private:
    const BodDescriptor desc_;
    std::array<AddressRange, 2> ranges_{};
    u8 ctrla_{0x05U};
    u8 ctrlb_{0x00U};
    u8 vlmctrla_{0x00U};
    u8 intctrl_{0x00U};
    u8 intflags_{0x00U};
    u8 status_{0x00U};
    bool pending_{false};
    bool prev_below_{false};
};

} // namespace vioavr::core
