#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/types.hpp"

#include <span>
#include <string_view>
#include <vector>

namespace vioavr::core {

struct HexImage {
    std::vector<u16> flash_words {};
    u32 entry_word {};
};

class HexImageLoader {
public:
    [[nodiscard]] static HexImage load_file(std::string_view path, const DeviceDescriptor& device);
    [[nodiscard]] static HexImage load_text(std::string_view text, const DeviceDescriptor& device);
    [[nodiscard]] static std::vector<u16> bytes_to_words(std::span<const u8> bytes);
};

}  // namespace vioavr::core
