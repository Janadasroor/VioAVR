#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace vioavr::core {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

enum class MemorySpace : u8 {
    flash,
    sram,
    io
};

enum class CpuState : u8 {
    halted,
    running,
    paused,
    sleeping
};

enum class PinLevel : u8 {
    low = 0,
    high = 1
};

enum class SregFlag : u8 {
    carry = 0,
    zero = 1,
    negative = 2,
    overflow = 3,
    sign = 4,
    halfCarry = 5,
    transfer = 6,
    interrupt = 7
};

struct AddressRange {
    u16 begin {};
    u16 end {};

    [[nodiscard]] constexpr bool contains(const u16 address) const noexcept
    {
        return address >= begin && address <= end;
    }
};

struct PinStateChange {
    std::string_view port_name {};
    u8 bit_index {};
    bool level {};
    u64 cycle_stamp {};
};

struct InterruptRequest {
    u8 vector_index {};
    u8 source_id {};
};

struct BusTransaction {
    MemorySpace space {MemorySpace::sram};
    u16 address {};
    u8 value {};
};

inline constexpr std::size_t kRegisterFileSize = 32;
inline constexpr std::size_t kMaxIoRegisters = 0x100;
inline constexpr std::size_t kAtmega328FlashWords = 16 * 1024;
inline constexpr std::size_t kAtmega328SramBytes = 2 * 1024;

}  // namespace vioavr::core
