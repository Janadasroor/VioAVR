#pragma once

namespace vioavr::core {

struct DigitalThresholdConfig {
    double low_threshold {0.3};
    double high_threshold {0.6};
};

[[nodiscard]] inline bool apply_schmitt_threshold(const bool previous_level,
                                                  const double normalized_voltage,
                                                  const DigitalThresholdConfig config = {}) noexcept
{
    if (normalized_voltage <= config.low_threshold) {
        return false;
    }
    if (normalized_voltage >= config.high_threshold) {
        return true;
    }
    return previous_level;
}

}  // namespace vioavr::core
