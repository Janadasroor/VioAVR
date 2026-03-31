#include "vioavr/core/hex_image.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace vioavr::core {

namespace {

[[nodiscard]] u8 parse_hex_byte(const std::string_view text, const std::size_t offset)
{
    if (offset + 2U > text.size()) {
        throw std::runtime_error("truncated HEX byte");
    }

    unsigned value = 0U;
    const auto result = std::from_chars(text.data() + static_cast<std::ptrdiff_t>(offset),
                                        text.data() + static_cast<std::ptrdiff_t>(offset + 2U),
                                        value,
                                        16);
    if (result.ec != std::errc {} || result.ptr != text.data() + static_cast<std::ptrdiff_t>(offset + 2U)) {
        throw std::runtime_error("invalid HEX byte");
    }

    return static_cast<u8>(value);
}

[[nodiscard]] std::string_view trim_line(const std::string_view line) noexcept
{
    std::size_t begin = 0U;
    std::size_t end = line.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(line[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(line[end - 1U])) != 0) {
        --end;
    }
    return line.substr(begin, end - begin);
}

}  // namespace

HexImage HexImageLoader::load_file(const std::string_view path, const DeviceDescriptor& device)
{
    std::ifstream input {std::string(path)};
    if (!input) {
        throw std::runtime_error("failed to open HEX image");
    }

    const std::string text {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };
    return load_text(text, device);
}

HexImage HexImageLoader::load_text(const std::string_view text, const DeviceDescriptor& device)
{
    std::vector<u8> flash_bytes(device.flash_words * 2U, 0U);
    bool eof_seen = false;
    u32 upper_linear_base = 0U;

    std::size_t cursor = 0U;
    while (cursor <= text.size()) {
        const std::size_t next_newline = text.find('\n', cursor);
        const std::size_t end = next_newline == std::string_view::npos ? text.size() : next_newline;
        std::string_view line = trim_line(text.substr(cursor, end - cursor));
        cursor = next_newline == std::string_view::npos ? text.size() + 1U : next_newline + 1U;

        if (line.empty()) {
            continue;
        }

        if (eof_seen) {
            throw std::runtime_error("HEX data found after EOF record");
        }

        if (line.front() != ':') {
            throw std::runtime_error("HEX line missing ':' prefix");
        }

        line.remove_prefix(1U);
        if (line.size() < 10U || (line.size() % 2U) != 0U) {
            throw std::runtime_error("HEX line has invalid length");
        }

        const u8 byte_count = parse_hex_byte(line, 0U);
        const u16 address = static_cast<u16>((static_cast<u16>(parse_hex_byte(line, 2U)) << 8U) |
                                             parse_hex_byte(line, 4U));
        const u8 record_type = parse_hex_byte(line, 6U);
        const std::size_t expected_length = 10U + (static_cast<std::size_t>(byte_count) * 2U);
        if (line.size() != expected_length) {
            throw std::runtime_error("HEX byte count does not match record length");
        }

        u32 checksum_accumulator = byte_count;
        checksum_accumulator += static_cast<u8>(address >> 8U);
        checksum_accumulator += static_cast<u8>(address & 0xFFU);
        checksum_accumulator += record_type;

        std::vector<u8> record_bytes;
        record_bytes.reserve(byte_count);
        for (std::size_t index = 0; index < byte_count; ++index) {
            const u8 value = parse_hex_byte(line, 8U + (index * 2U));
            record_bytes.push_back(value);
            checksum_accumulator += value;
        }

        const u8 checksum = parse_hex_byte(line, expected_length - 2U);
        const u8 computed_checksum = static_cast<u8>((~checksum_accumulator + 1U) & 0xFFU);
        if (checksum != computed_checksum) {
            throw std::runtime_error("HEX checksum mismatch");
        }

        switch (record_type) {
        case 0x00: {
            const u32 absolute_address = upper_linear_base + address;
            const u32 record_end = absolute_address + byte_count;
            if (record_end > flash_bytes.size()) {
                throw std::out_of_range("HEX image exceeds configured flash capacity");
            }
            std::ranges::copy(record_bytes, flash_bytes.begin() + static_cast<std::ptrdiff_t>(absolute_address));
            break;
        }
        case 0x01:
            if (byte_count != 0U || address != 0U) {
                throw std::runtime_error("invalid EOF record");
            }
            eof_seen = true;
            break;
        case 0x04:
            if (byte_count != 2U || address != 0U) {
                throw std::runtime_error("invalid extended linear address record");
            }
            upper_linear_base = static_cast<u32>(
                ((static_cast<u16>(record_bytes[0]) << 8U) | record_bytes[1]) << 16U
            );
            break;
        default:
            throw std::runtime_error("unsupported HEX record type");
        }
    }

    if (!eof_seen) {
        throw std::runtime_error("HEX image missing EOF record");
    }

    HexImage image;
    image.flash_words = bytes_to_words(flash_bytes);
    while (!image.flash_words.empty() && image.flash_words.back() == 0U) {
        image.flash_words.pop_back();
    }
    image.entry_word = 0U;
    return image;
}

std::vector<u16> HexImageLoader::bytes_to_words(const std::span<const u8> bytes)
{
    std::vector<u16> words;
    words.reserve((bytes.size() + 1U) / 2U);

    for (std::size_t index = 0; index < bytes.size(); index += 2U) {
        const u8 low = bytes[index];
        const u8 high = index + 1U < bytes.size() ? bytes[index + 1U] : 0U;
        words.push_back(static_cast<u16>(low | (static_cast<u16>(high) << 8U)));
    }

    return words;
}

}  // namespace vioavr::core
