#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace vioavr::core {

class MemoryBus;

class Ebi final : public IoPeripheral {
public:
    explicit Ebi(std::string_view name, const EbiDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] bool wants_tick() const noexcept override { return true; }

    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

    // External memory access via MemoryBus
    [[nodiscard]] u8 external_read(u16 address) noexcept;
    void external_write(u16 address, u8 value) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }

    [[nodiscard]] bool is_cs0_active() const noexcept { return cs0_enabled_; }
    [[nodiscard]] u16 cs0_start() const noexcept { return cs0_start_; }
    [[nodiscard]] u16 cs0_end() const noexcept { return cs0_end_; }
    [[nodiscard]] u8 get_wait_states() const noexcept;

    // Exposed for testing
    [[nodiscard]] std::span<const u8> memory() const noexcept { return memory_; }
    [[nodiscard]] std::span<u8> memory() noexcept { return memory_; }

private:
    void update_ranges();
    void update_cs0_range() noexcept;
    [[nodiscard]] bool is_sdram_enabled() const noexcept;

    std::string name_;
    EbiDescriptor desc_;
    std::array<AddressRange, 4> ranges_{};
    u8 num_ranges_{};

    MemoryBus* bus_{nullptr};

    u8 ctrl_{};
    u8 sdramctrla_{};
    u16 refresh_{};
    u16 initdly_{};
    u8 sdramctrlb_{};
    u8 sdramctrlc_{};
    u8 cs0_ctrla_{};
    u8 cs0_ctrlb_{};
    u16 cs0_baseaddr_{};

    u16 init_counter_{};
    u16 refresh_counter_{};
    bool init_complete_{};
    bool sdram_enabled_{};

    u16 cs0_start_{};
    u16 cs0_end_{};
    bool cs0_enabled_{false};

    std::vector<u8> memory_;
};

} // namespace vioavr::core
