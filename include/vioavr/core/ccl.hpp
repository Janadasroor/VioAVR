#pragma once

#include "vioavr/core/types.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <vector>
#include <array>

namespace vioavr::core {

class MemoryBus;

/**
 * @brief Configurable Custom Logic (CCL) for modern AVR devices.
 * Implements programmable logic LUTs and sequential stages.
 */
class Ccl : public IoPeripheral {
public:
    explicit Ccl(const CclDescriptor& desc);

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] std::string_view name() const noexcept override { return "CCL"; }
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    void set_memory_bus(MemoryBus* bus) noexcept override;
    void set_event_system(class EventSystem* evsys) noexcept;
    void set_pin_mux(class PinMux* pm) noexcept;

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    // Logic Interface
    bool get_lut_output(u8 index) const noexcept { return outputs_[index]; }
    bool get_seq_output(u8 index) const noexcept { return seq_state_[index]; }
    
    // External Input Interface (Pins)
    void set_pin_input(u8 lut_index, u8 input_index, bool level) noexcept;

private:
    const CclDescriptor desc_;
    MemoryBus* bus_ {nullptr};
    class EventSystem* evsys_ {nullptr};
    class PinMux* pin_mux_ {nullptr};

    class Tca* tca0_ {nullptr};
    std::array<class Tcb*, 4> tcbs_ {nullptr, nullptr, nullptr, nullptr};
    class Ac8x* ac0_ {nullptr};

    u8 ctrla_ {0x00};
    std::array<u8, 4> seqctrl_ {};
    std::array<u8, 2> intctrl_ {};
    std::array<u8, 2> intflags_ {};

    struct LutState {
        u8 ctrla {0x00};
        u8 ctrlb {0x00};
        u8 ctrlc {0x00};
        u8 truth {0x00};
        
        // Input state (0=IN0, 1=IN1, 2=IN2)
        std::array<bool, 3> inputs {false, false, false};
    };
    std::array<LutState, 4> luts_ {}; // ATmega4809 has 4 LUTs
    std::array<bool, 4> outputs_ {};
    std::array<bool, 4> prev_outputs_ {};
    std::array<bool, 2> prev_luts_in2_ {};
    
    // State for sequential logic (SEQ0, SEQ1)
    std::array<bool, 2> seq_state_ {};

    std::array<AddressRange, 6> ranges_ {};
    
    void update_logic() noexcept;
    void update_routing() noexcept;
    bool compute_lut(u8 index) const noexcept;
};

} // namespace vioavr::core
