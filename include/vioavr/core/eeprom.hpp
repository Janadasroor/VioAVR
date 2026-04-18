#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <vector>
#include <string>
#include <array>

namespace vioavr::core {

class Eeprom final : public IoPeripheral {
public:
    explicit Eeprom(std::string_view name, const EepromDescriptor& descriptor) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    // Persistence API
    bool load_from_file(const std::string& path);
    bool save_to_file(const std::string& path) const;

    void commit_page(u32 address, std::span<const u8> data) noexcept;
    void erase_all() noexcept;

private:
    void update_eecr(u8 value) noexcept;
    void start_write() noexcept;
    void complete_write() noexcept;

    std::string_view name_;
    EepromDescriptor desc_;
    std::array<AddressRange, 2> ranges_ {};
    u8 ranges_count_ {1U};
    u16 size_;

    std::vector<u8> storage_;
    
    u8 eecr_ {};
    u8 eedr_ {};
    u16 eear_ {};
    
    u32 write_cycles_left_ {};
    u8 master_write_enable_timeout_ {};
    bool interrupt_pending_ {};
};

}  // namespace vioavr::core
