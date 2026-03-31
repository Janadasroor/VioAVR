#pragma once

#include "vioavr/core/device.hpp"
#include <string_view>
#include <optional>
#include <vector>

namespace vioavr::core {

/**
 * @brief Registry of all supported AVR devices.
 */
class DeviceCatalog {
public:
    /**
     * @brief Find a device descriptor by its name (case-insensitive).
     * @param name The device name (e.g., "ATmega328P")
     * @return Pointer to the descriptor, or nullptr if not found.
     */
    [[nodiscard]] static const DeviceDescriptor* find(std::string_view name) noexcept;

    /**
     * @brief Get a list of all supported device names.
     */
    [[nodiscard]] static std::vector<std::string_view> list_devices() noexcept;
};

} // namespace vioavr::core
