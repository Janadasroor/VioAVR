#include "vioavr/core/crc8x.hpp"

namespace vioavr::core {

Crc8x::Crc8x(const Crc8xDescriptor& descriptor, std::span<const u16> flash) noexcept
    : desc_(descriptor), flash_(flash)
{
    u16 end = desc_.ctrla_address;
    if (desc_.status_address > end) end = desc_.status_address;
    if (desc_.data_address > end) end = desc_.data_address;
    if (desc_.checksum_address > end) end = desc_.checksum_address;
    
    ranges_[0] = AddressRange{desc_.ctrla_address, static_cast<u16>(end)};
}

std::span<const AddressRange> Crc8x::mapped_ranges() const noexcept {
    return ranges_;
}

void Crc8x::reset() noexcept {
    ctrla_ = 0;
    status_ = 0;
    checksum_ = 0;
    busy_ = false;
    remaining_cycles_ = 0;
}

void Crc8x::tick(u64 elapsed_cycles) noexcept {
    if (!busy_) return;

    if (elapsed_cycles >= remaining_cycles_) {
        busy_ = false;
        remaining_cycles_ = 0;
        status_ &= ~0x01U; // Clear BUSY
        status_ |= 0x02U;  // Set OK (for simulation, always OK)
    } else {
        remaining_cycles_ -= elapsed_cycles;
    }
}

u8 Crc8x::read(u16 address) noexcept {
    if (address == desc_.ctrla_address) return ctrla_;
    if (address == desc_.status_address) return status_;
    if (address == desc_.checksum_address) return checksum_ & 0xFFU;
    if (address == desc_.checksum_address + 1) return (checksum_ >> 8) & 0xFFU;
    return 0;
}

void Crc8x::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrla_address) {
        ctrla_ = value;
        if (value & 0x01U) { // ENABLE? (Verify bit in ATDF)
            start_scan();
        }
    }
}

void Crc8x::start_scan() noexcept {
    busy_ = true;
    status_ |= 0x01U; // BUSY
    status_ &= ~0x02U; // Clear OK
    checksum_ = calculate_crc16(flash_);
    
    // Scan takes time proportional to flash size
    remaining_cycles_ = flash_.size(); 
}

u16 Crc8x::calculate_crc16(std::span<const u16> data) noexcept {
    u16 crc = 0xFFFFU;
    for (u16 word : data) {
        // Process Low Byte
        u8 lb = word & 0xFFU;
        crc ^= (u16)lb << 8;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000U) crc = (crc << 1) ^ 0x1021U; else crc <<= 1;
        }
        // Process High Byte
        u8 hb = word >> 8;
        crc ^= (u16)hb << 8;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000U) crc = (crc << 1) ^ 0x1021U; else crc <<= 1;
        }
    }
    return crc;
}
    
void Crc8x::set_event_system(EventSystem* evsys) noexcept {
    evsys_ = evsys;
}

} // namespace vioavr::core
