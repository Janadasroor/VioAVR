#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/logger.hpp"
#include <algorithm>
#include <fstream>

namespace vioavr::core {
    
static void eeprom_master_timeout_callback(u64 cycle, void* param) {
    static_cast<Eeprom*>(param)->on_master_timeout(cycle);
}

static void eeprom_write_complete_callback(u64 cycle, void* param) {
    static_cast<Eeprom*>(param)->on_write_complete(cycle);
}

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

Eeprom::Eeprom(std::string_view name, const EepromDescriptor& descriptor) noexcept
    : name_(name), desc_(descriptor), size_(descriptor.size)
{
    storage_.resize(size_, 0xFFU);
    if (desc_.eecr_address != 0U) {
        ranges_[0] = {desc_.eecr_address, static_cast<u16>(desc_.eearh_address > 0 ? desc_.eearh_address : desc_.eecr_address + 2U)};
        ranges_count_ = 1U;
    } else {
        ranges_count_ = 0U;
    }
    
    if (desc_.mapped_data.size > 0) {
        ranges_[ranges_count_] = {desc_.mapped_data.data_start, static_cast<u16>(desc_.mapped_data.data_start + desc_.mapped_data.size - 1U)};
        ranges_count_++;
    }
}

std::string_view Eeprom::name() const noexcept { return name_; }

std::span<const AddressRange> Eeprom::mapped_ranges() const noexcept
{
    return {ranges_.data(), ranges_count_};
}

void Eeprom::reset() noexcept
{
    eecr_ = 0U;
    eedr_ = 0U;
    eear_ = 0U;
    write_cycles_left_ = 0U;
    master_write_enable_timeout_ = 0U;
    set_interrupt_pending(false);
}

void Eeprom::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
    // Handled by scheduler
}

void Eeprom::on_master_timeout(u64 cycle) noexcept {
    (void)cycle;
    if (master_write_enable_timeout_ > 0U) {
        master_write_enable_timeout_ = 0U;
        eecr_ &= static_cast<u8>(~kEempe);
    }
}

void Eeprom::on_write_complete(u64 cycle) noexcept {
    (void)cycle;
    if (write_cycles_left_ > 0U) {
        write_cycles_left_ = 0U;
        complete_write();
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

    // Unified Data Map Read
    if (desc_.mapped_data.size > 0 && address >= desc_.mapped_data.data_start && address < desc_.mapped_data.data_start + desc_.mapped_data.size) {
        if (bus_) bus_->request_cpu_stall(4U); // Hardware stall for EEPROM read
        const u16 offset = address - desc_.mapped_data.data_start;
        return (offset < storage_.size()) ? storage_[offset] : 0xFFU;
    }

    return 0U;
}

void Eeprom::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.eecr_address) {
        update_eecr(value);
        update_interrupt_pending();
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
    } else if (desc_.mapped_data.size > 0 && address >= desc_.mapped_data.data_start && address < desc_.mapped_data.data_start + desc_.mapped_data.size) {
        // Direct mapped write (AVR8X style) — only if no write in progress
        if (write_cycles_left_ == 0U) {
            if (bus_) bus_->request_cpu_stall(kEepromAtomicCycles);
            const u16 offset = address - desc_.mapped_data.data_start;
            if (offset < storage_.size()) {
                Logger::debug("Eeprom '" + std::string(name_) + "': Direct write at offset " + std::to_string(offset) + " = 0x" + std::to_string(value));
                storage_[offset] = value;
            }
        }
    }
}

bool Eeprom::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    // EERIE triggers if EEPE is zero AND EERIE is set.
    if (write_cycles_left_ == 0U && (eecr_ & kEerie) != 0U) {
        request = {desc_.vector_index, 0U};
        return true;
    }
    return false;
}

void Eeprom::update_interrupt_pending() noexcept {
    InterruptRequest req;
    set_interrupt_pending(pending_interrupt_request(req));
}

bool Eeprom::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (pending_interrupt_request(request)) {
        set_interrupt_pending(false);
        return true;
    }
    return false;
}

void Eeprom::update_eecr(const u8 value) noexcept
{
    // EERE: Read Enable
    if ((value & kEere) != 0U) {
        if (write_cycles_left_ == 0U) {
            const u16 addr = static_cast<u16>(eear_ % size_);
            eedr_ = storage_[addr];
            if (bus_) bus_->request_cpu_stall(4U);
        }
    }

    // EEMPE: Master Write Enable
    if ((value & kEempe) != 0U) {
        if (write_cycles_left_ == 0U) {
            eecr_ |= kEempe;
            master_write_enable_timeout_ = kMasterWriteTimeout;
            if (bus_) {
                bus_->scheduler().cancel(eeprom_master_timeout_callback, this);
                bus_->scheduler().schedule(master_write_enable_timeout_, eeprom_master_timeout_callback, this);
            }
        }
    }

    // EEPE: Write Enable
    if ((value & kEepe) != 0U) {
        if ((eecr_ & kEempe) != 0U && write_cycles_left_ == 0U) {
            start_write();
            // 2-cycle stall per ATmega datasheet (timed via separate step() calls).
            if (bus_) bus_->request_cpu_stall(2U);
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
    eecr_ = static_cast<u8>((eecr_ & ~kEempe) | kEepe);
    master_write_enable_timeout_ = 0;

    eepm_mode_ = (eecr_ >> 4U) & 0x03U; // Latch programming mode at write start
    switch (eepm_mode_) {
        case 0x00U: write_cycles_left_ = kEepromAtomicCycles; break;
        case 0x01U: write_cycles_left_ = kEepromEraseOnlyCycles; break;
        case 0x02U: write_cycles_left_ = kEepromWriteOnlyCycles; break;
        default:    write_cycles_left_ = kEepromAtomicCycles; break;
    }
    if (bus_) {
        bus_->scheduler().cancel(eeprom_write_complete_callback, this);
        bus_->scheduler().schedule(write_cycles_left_, eeprom_write_complete_callback, this);
    }
}

void Eeprom::complete_write() noexcept
{
    const u16 addr = static_cast<u16>(eear_ % size_);
    
    if (eepm_mode_ == 0x01U) { // Erase only
        storage_[addr] = 0xFFU;
    } else if (eepm_mode_ == 0x02U) { // Write only
        storage_[addr] &= eedr_;
    } else { // Atomic
        storage_[addr] = eedr_;
    }
    
    eecr_ &= static_cast<u8>(~kEepe);
    update_interrupt_pending();
}

void Eeprom::commit_page(u32 address, std::span<const u8> data) noexcept
{
    const u16 start = static_cast<u16>(address % size_);
    Logger::debug("Eeprom '" + std::string(name_) + "': Commit page at address 0x" + std::to_string(address) + " (offset " + std::to_string(start) + "), data size " + std::to_string(data.size()));
    for (size_t i = 0; i < data.size() && (start + i) < storage_.size(); ++i) {
        if (i < 8) Logger::debug("  data[" + std::to_string(i) + "] = 0x" + std::to_string(data[i]));
        storage_[start + i] = data[i];
    }
}

void Eeprom::erase_all() noexcept
{
    std::ranges::fill(storage_, 0xFFU);
}

bool Eeprom::load_from_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(storage_.data()), storage_.size());
    return true;
}

bool Eeprom::save_to_file(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(storage_.data()), storage_.size());
    return true;
}

}  // namespace vioavr::core
