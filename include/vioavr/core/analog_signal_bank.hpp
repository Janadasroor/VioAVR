#pragma once

#include "vioavr/core/types.hpp"

#include <array>
#include <algorithm>
#include <atomic>

namespace vioavr::core {

/**
 * @brief A thread-safe collection of analog input signals.
 * 
 * This bank serves as the primary interface for external simulators to inject
 * analog voltages into the ISS. It uses atomic doubles for lock-free
 * updates from the simulator thread while the ISS thread samples them.
 * Voltages are stored as absolute values in Volts (not normalized).
 */
class AnalogSignalBank {
public:
    static constexpr std::size_t kChannelCount = 128;

    /**
     * @brief Set the voltage of a channel.
     * @param channel The channel index (0 to 127).
     * @param voltage The voltage in Volts (absolute, e.g., 2.5 for 2.5V).
     */
    void set_voltage(u8 channel, double voltage) noexcept
    {
        if (channel >= kChannelCount) {
            return;
        }

        voltages_[channel].store(voltage < 0.0 ? 0.0 : voltage, std::memory_order_relaxed);
    }

    /**
     * @brief Get the voltage of a channel.
     * @param channel The channel index.
     * @return The voltage in Volts.
     */
    [[nodiscard]] double voltage(u8 channel) const noexcept
    {
        if (channel >= kChannelCount) {
            return 0.0;
        }
        return voltages_[channel].load(std::memory_order_relaxed);
    }

private:
    std::array<std::atomic<double>, kChannelCount> voltages_ {};
};

}  // namespace vioavr::core
