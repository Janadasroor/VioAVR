#pragma once

#include "vioavr/core/types.hpp"

#include <array>
#include <algorithm>
#include <atomic>

namespace vioavr::core {

/**
 * @brief A thread-safe collection of analog input signals.
 * 
 * This bank serves as the primary interface for external simulators (e.g., ngspice)
 * to inject analog voltages into the ISS. It uses atomic doubles for lock-free
 * updates from the simulator thread while the ISS thread samples them.
 */
class AnalogSignalBank {
public:
    static constexpr std::size_t kChannelCount = 128;

    /**
     * @brief Set the voltage of a channel.
     * @param channel The channel index (0 to 15).
     * @param normalized_voltage The voltage normalized to [0.0, 1.0].
     */
    void set_voltage(u8 channel, double normalized_voltage) noexcept
    {
        if (channel >= kChannelCount) {
            return;
        }

        voltages_[channel].store(std::clamp(normalized_voltage, 0.0, 1.0), std::memory_order_relaxed);
    }

    /**
     * @brief Get the voltage of a channel.
     * @param channel The channel index.
     * @return The normalized voltage [0.0, 1.0].
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
