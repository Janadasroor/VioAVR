#pragma once

#include "vioavr/core/types.hpp"
#include <string_view>
#include <vector>
#include <functional>
#include <unordered_map>

namespace vioavr::core {

/**
 * @brief Identifies which peripheral currently "owns" a physical pin.
 */
enum class PinOwner : u8 {
    gpio = 0,
    adc = 1,
    comparator = 2,
    timer = 3,
    uart = 4,
    spi = 5,
    twi = 6,
    external_clock = 7,
    reset = 8,
    jtag = 9
};

/**
 * @brief Returns the priority of a pin owner. Higher is stronger.
 */
[[nodiscard]] constexpr u8 get_pin_priority(PinOwner owner) noexcept {
    return static_cast<u8>(owner);
}

/**
 * @brief Represents the logical state of a physical pin.
 */
struct PinState {
    PinOwner owner {PinOwner::gpio};
    bool is_output {false};
    bool drive_level {false};
    bool pullup_enabled {false};
    double voltage {0.0}; // Normalized 0.0 to 1.0 (or absolute if needed)
};

/**
 * @brief Callback type for when a pin's logical state changes.
 * This can be used by peripherals to react to pin changes or by a SPICE bridge.
 */
using PinChangeCallback = std::function<void(u8 port_idx, u8 bit_idx, const PinState& state)>;

/**
 * @brief Manages pin ownership and alternate function routing across the entire MCU.
 */
class PinMux {
public:
    explicit PinMux(u8 num_ports) noexcept;

    /**
     * @brief Claims a pin for a specific peripheral.
     * @return true if the pin was successfully claimed or already owned by this owner.
     */
    bool claim_pin(u8 port_idx, u8 bit_idx, PinOwner owner) noexcept;

    /**
     * @brief Releases a pin back to GPIO ownership.
     */
    void release_pin(u8 port_idx, u8 bit_idx, PinOwner owner) noexcept;

    /**
     * @brief Updates the drive level/direction/pullup of a pin.
     * Only the current owner (or GPIO) can change the state.
     */
    void update_pin(u8 port_idx, u8 bit_idx, PinOwner requester, bool is_output, bool level, bool pullup = false) noexcept;

    void update_pullup_suppressed(bool suppressed) noexcept;

    /**
     * @brief Registers a port's base PIN address to an internal port index.
     */
    void register_port(u16 pin_address, u8 port_idx) noexcept;

    /**
     * @brief Claims a pin for a specific peripheral using its I/O address.
     */
    bool claim_pin_by_address(u16 pin_address, u8 bit_index, PinOwner owner) noexcept;
    void release_pin_by_address(u16 pin_address, u8 bit_index, PinOwner owner) noexcept;
    void update_pin_by_address(u16 pin_address, u8 bit_index, PinOwner requester, bool is_output, bool level, bool pullup = false) noexcept;

    [[nodiscard]] PinState get_state(u8 port_idx, u8 bit_idx) const noexcept;
    [[nodiscard]] PinState get_state_by_address(u16 pin_address, u8 bit_index) const noexcept;

    /**
     * @brief Registers a listener for pin changes.
     */
    void set_callback(PinChangeCallback callback) noexcept;

private:
    struct PinEntry {
        PinState state {};
        u32 active_claims {0}; // Bitmask of PinOwner values (1 << static_cast<u8>(owner))
    };

    std::vector<std::vector<PinEntry>> ports_;
    std::unordered_map<u16, u8> addr_to_port_;
    PinChangeCallback callback_ {};

    bool pullup_suppressed_ {false}; // From MCUCR PUD bit

    void reevaluate_ownership(u8 port_idx, u8 bit_idx) noexcept;
};

} // namespace vioavr::core
