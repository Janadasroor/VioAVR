#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class AnalogSignalBank;

class XmegaAc final : public IoPeripheral {
public:
    explicit XmegaAc(const XmegaAcDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_analog_signal_bank(AnalogSignalBank* bank) noexcept { signal_bank_ = bank; }

    [[nodiscard]] std::string_view name() const noexcept override { return "AC"; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override { return {ranges_.data(), ranges_.size()}; }

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

private:
    struct CompRegs {
        u8 ctrl {};
        u8 mux {};
    };

    void evaluate(u8 comp_index) noexcept;
    void update_window() noexcept;
    void update_interrupt_state() noexcept;
    [[nodiscard]] double comp_input_positive(const CompRegs& regs) const noexcept;
    [[nodiscard]] double comp_input_negative(const CompRegs& regs) const noexcept;

    XmegaAcDescriptor desc_;
    MemoryBus* bus_ {};
    AnalogSignalBank* signal_bank_ {};
    std::array<AddressRange, 1> ranges_ {};

    CompRegs comps_[2] {};
    u8 ctrla_ {};
    u8 winctrl_ {};
    u8 status_ {};

    bool prev_comp_out_[2] {};
    u8 prev_wstate_ {};
    bool pending_int_ {};
};

} // namespace vioavr::core
