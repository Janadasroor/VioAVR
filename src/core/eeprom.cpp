#include "vioavr/core/eeprom.hpp"
#include <algorithm>
#include <fstream>

namespace vioavr::core {

namespace {
// EECR bits
constexpr u8 kEerie = 0x08U; // EEPROM Ready Interrupt Enable
constexpr u8 kEempe = 0x04U; // EEPROM Master Write Enable
constexpr u8 kEepe  = 0x02U; // EEPROM Write Enable
constexpr u8 kEere  = 0x01U; // EEPROM Read Enable
constexpr u8 kEepm1 = 0x20U; // Programming Mode Bit 1
constexpr u8 kEepm0 = 0x10U; // Programming Mode Bit 0

// 3.4ms at 16MHz is ~54,400 cycles.
// 1.8ms at 16MHz is ~28,800 cycles.
constexpr u32 kEepromAtomicCycles = 54400U;
constexpr u32 kEepromEraseOnlyCycles = 28800U;
constexpr u32 kEepromWriteOnlyCycles = 28800U;
constexpr u8 kMasterWriteTimeout = 4U; 
}

Eeprom::Eeprom(std::string_view name, const DeviceDescriptor& device) noexcept
    : name_(name), desc_(device.eeprom), size_(device.eeprom_bytes)
{
    storage_.resize(size_, 0xFFU);
    const std::array<u16, 4> addrs = {
        desc_.eecr_address, desc_.eedr_address, 
        desc_.eearl_address, desc_.eearh_address
    };
    ranges_ = {{
        AddressRange{
            *std::min_element(addrs.begin(), addrs.end()),
            *std::max_element(addrs.begin(), addrs.end())
        }
    }};
}

std::string_view Eeprom::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> Eeprom::mapped_ranges() const noexcept
{
    return ranges_;
}

void Eeprom::reset() noexcept
{
    eecr_ = 0U;
    eedr_ = 0U;
    eear_ = 0U;
    write_cycles_left_ = 0U;
    master_write_enable_timeout_ = 0U;
    interrupt_pending_ = false;
}

void Eeprom::tick(const u64 elapsed_cycles) noexcept
{
    if (master_write_enable_timeout_ > 0U) {
        if (elapsed_cycles >= master_write_enable_timeout_) {
            master_write_enable_timeout_ = 0U;
            eecr_ &= static_cast<u8>(~kEempe);
        } else {
            master_write_enable_timeout_ -= static_cast<u8>(elapsed_cycles);
        }
    }

    if (write_cycles_left_ > 0U) {
        if (elapsed_cycles >= write_cycles_left_) {
            write_cycles_left_ = 0U;
            complete_write();
        } else {
            write_cycles_left_ -= static_cast<u32>(elapsed_cycles);
        }
    }
}

u8 Eeprom::read(const u16 address) noexcept
{
    if (address == desc_.eecr_address) {
        return static_cast<u8>(eecr_ | (write_cycles_left_ > 0 ? kEepe : 0U));
    }
    if (address == desc_.eedr_address) {
        return eedr_;
    }
    if (address == desc_.eearl_address) {
        return static_cast<u8>(eear_ & 0xFFU);
    }
    if (address == desc_.eearh_address) {
        return static_cast<u8>((eear_ >> 8U) & 0xFFU);
    }
    return 0U;
}

void Eeprom::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.eecr_address) {
        update_eecr(value);
        return;
    }
    
    // According to datasheet, if EEPE is set, writes to EEAR and EEDR are ignored.
    if (write_cycles_left_ > 0U) {
        return;
    }

    if (address == desc_.eedr_address) {
        eedr_ = value;
    } else if (address == desc_.eearl_address) {
        eear_ = static_cast<u16>((eear_ & 0xFF00U) | value);
    } else if (address == desc_.eearh_address) {
        eear_ = static_cast<u16>((static_cast<u16>(value) << 8U) | (eear_ & 0x00FFU));
    }
}

void Eeprom::update_eecr(const u8 value) noexcept
{
    // EERE: Read Enable
    if ((value & kEere) != 0U) {
        // Read is only possible if EEPE is not set
        if (write_cycles_left_ == 0U) {
            const u16 addr = static_cast<u16>(eear_ % size_);
            eedr_ = storage_[addr];
        }
        // EERE is cleared by hardware after one cycle, we just don't store it.
    }

    // EEMPE: Master Write Enable
    if ((value & kEempe) != 0U) {
        if (write_cycles_left_ == 0U) {
            eecr_ |= kEempe;
            master_write_enable_timeout_ = kMasterWriteTimeout;
        }
    }

    // EEPE: Write Enable
    if ((value & kEepe) != 0U) {
        if ((eecr_ & kEempe) != 0U && write_cycles_left_ == 0U) {
            start_write();
        }
    }

    // EERIE: Interrupt Enable
    if ((value & kEerie) != 0U) {
        eecr_ |= kEerie;
    } else {
        eecr_ &= static_cast<u8>(~kEerie);
    }

    // EEPM: Programming Mode
    eecr_ = static_cast<u8>((eecr_ & 0xCFU) | (value & 0x30U));
}

void Eeprom::start_write() noexcept
{
    eecr_ &= static_cast<u8>(~kEempe);
    master_write_enable_timeout_ = 0;
    
    const u8 mode = (eecr_ >> 4U) & 0x03U;
    switch (mode) {
        case 0x00U: write_cycles_left_ = kEepromAtomicCycles; break;
        case 0x01U: write_cycles_left_ = kEepromEraseOnlyCycles; break;
        case 0x02U: write_cycles_left_ = kEepromWriteOnlyCycles; break;
        default:    write_cycles_left_ = kEepromAtomicCycles; break;
    }
}

void Eeprom::complete_write() noexcept
{
    const u16 addr = static_cast<u16>(eear_ % size_);
    const u8 mode = (eecr_ >> 4U) & 0x03U;
    
    if (mode == 0x01U) { // Erase Only
        storage_[addr] = 0xFFU;
    } else if (mode == 0x02U) { // Write Only
        storage_[addr] &= eedr_;
    } else { // Atomic (Erase + Write)
        storage_[addr] = eedr_;
    }
    
    interrupt_pending_ = true;
}

bool Eeprom::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if (interrupt_pending_ && (eecr_ & kEerie) != 0U) {
        request = {desc_.vector_index, 0U};
        return true;
    }
    return false;
}

bool Eeprom::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (pending_interrupt_request(request)) {
        interrupt_pending_ = false;
        return true;
    }
    return false;
}

bool Eeprom::load_from_file(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    file.read(reinterpret_cast<char*>(storage_.data()), static_cast<std::streamsize>(storage_.size()));
    return true;
}

bool Eeprom::save_to_file(const std::string& path) const
{
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(storage_.data()), static_cast<std::streamsize>(storage_.size()));
    return true;
}

}  // namespace vioavr::core
