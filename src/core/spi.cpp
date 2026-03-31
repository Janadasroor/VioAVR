#include "vioavr/core/spi.hpp"
#include <algorithm>

namespace vioavr::core {

namespace {
constexpr u8 kSpiInterruptFlag = 0x80U;
constexpr u8 kSpiEnable = 0x40U;
constexpr u8 kSpiMaster = 0x10U;
constexpr u8 kSpiInterruptEnable = 0x80U;
}

Spi::Spi(std::string_view name, const DeviceDescriptor& device) noexcept
    : name_(name), desc_(device.spi)
{
    const std::array<u16, 3> addrs = {desc_.spcr_address, desc_.spsr_address, desc_.spdr_address};
    ranges_ = {{
        AddressRange{
            *std::min_element(addrs.begin(), addrs.end()),
            *std::max_element(addrs.begin(), addrs.end())
        }
    }};
}

std::string_view Spi::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> Spi::mapped_ranges() const noexcept
{
    return ranges_;
}

void Spi::reset() noexcept
{
    spcr_ = 0U;
    spsr_ = 0U;
    spdr_ = 0U;
    shift_register_ = 0U;
    miso_buffer_ = 0U;
    transfer_cycles_left_ = 0U;
    interrupt_pending_ = false;
}

void Spi::tick(const u64 elapsed_cycles) noexcept
{
    if (transfer_cycles_left_ == 0U) {
        return;
    }

    if (elapsed_cycles >= transfer_cycles_left_) {
        transfer_cycles_left_ = 0U;
        complete_transfer();
    } else {
        transfer_cycles_left_ -= static_cast<u32>(elapsed_cycles);
    }
}

u8 Spi::read(const u16 address) noexcept
{
    if (address == desc_.spcr_address) {
        return spcr_;
    }
    if (address == desc_.spsr_address) {
        // Note: Reading SPSR with SPIF set, then reading SPDR clears SPIF.
        // We handle this in SPDR read.
        return spsr_;
    }
    if (address == desc_.spdr_address) {
        if ((spsr_ & kSpiInterruptFlag) != 0U) {
            spsr_ &= static_cast<u8>(~kSpiInterruptFlag);
            interrupt_pending_ = false;
        }
        return spdr_;
    }
    return 0U;
}

void Spi::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.spcr_address) {
        spcr_ = value;
        return;
    }
    if (address == desc_.spsr_address) {
        // SPSR is mostly read-only except SPI2X (bit 0) which is R/W.
        spsr_ = static_cast<u8>((spsr_ & 0xFEU) | (value & 0x01U));
        return;
    }
    if (address == desc_.spdr_address) {
        if ((spcr_ & kSpiEnable) == 0U) {
            spdr_ = value;
            return;
        }

        if (transfer_cycles_left_ > 0U) {
            spsr_ |= 0x40U; // WCOL: Write Collision
            return;
        }

        // Only Master starts a transfer on SPDR write.
        // Slave waits for an external clock.
        if ((spcr_ & kSpiMaster) != 0U) {
            // Start transfer
            u8 data = value;
            if ((spcr_ & 0x20U) != 0U) { // DORD: Data Order
                // Reverse bits
                data = static_cast<u8>(
                    ((data & 0x01U) << 7U) | ((data & 0x02U) << 5U) |
                    ((data & 0x04U) << 3U) | ((data & 0x08U) << 1U) |
                    ((data & 0x10U) >> 1U) | ((data & 0x20U) >> 3U) |
                    ((data & 0x40U) >> 5U) | ((data & 0x80U) >> 7U)
                );
            }
            shift_register_ = data;
            
            u32 divisor = 0;
            const u8 spr = static_cast<u8>(spcr_ & 0x03U);
            const bool spi2x = (spsr_ & 0x01U) != 0U;

            switch (spr) {
            case 0: divisor = spi2x ? 2U : 4U; break;
            case 1: divisor = spi2x ? 8U : 16U; break;
            case 2: divisor = spi2x ? 32U : 64U; break;
            case 3: divisor = spi2x ? 64U : 128U; break;
            }

            transfer_cycles_left_ = 8U * divisor;
        } else {
            // Slave: just buffer the value to be shifted out later
            shift_register_ = value;
        }
    }
}

bool Spi::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if (interrupt_pending_ && (spcr_ & kSpiInterruptEnable) != 0U) {
        request = {desc_.vector_index, 0U}; // Source ID 0
        return true;
    }
    return false;
}

bool Spi::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (pending_interrupt_request(request)) {
        // AVR SPI interrupt is cleared by reading SPSR with SPIF set, then reading SPDR.
        // Or by the hardware itself if we enter the ISR? 
        // Actually, the flag stays set until cleared by the read sequence.
        return true;
    }
    return false;
}

void Spi::inject_miso_byte(const u8 value) noexcept
{
    miso_buffer_ = value;
}

void Spi::trigger_slave_transfer() noexcept
{
    if ((spcr_ & kSpiEnable) != 0U && (spcr_ & kSpiMaster) == 0U) {
        // Slave transfer start duration is usually external-clock dependent.
        // We'll just use a fixed 1 cycle here to trigger completion next step.
        transfer_cycles_left_ = 1U;
    }
}

u8 Spi::last_transmitted_byte() const noexcept
{
    return shift_register_;
}

bool Spi::busy() const noexcept
{
    return transfer_cycles_left_ > 0U;
}

void Spi::complete_transfer() noexcept
{
    u8 received = miso_buffer_;
    if ((spcr_ & 0x20U) != 0U) { // DORD
        // Reverse received bits if DORD=1
        received = static_cast<u8>(
            ((received & 0x01U) << 7U) | ((received & 0x02U) << 5U) |
            ((received & 0x04U) << 3U) | ((received & 0x08U) << 1U) |
            ((received & 0x10U) >> 1U) | ((received & 0x20U) >> 3U) |
            ((received & 0x40U) >> 5U) | ((received & 0x80U) >> 7U)
        );
    }
    spdr_ = received;
    spsr_ |= kSpiInterruptFlag;
    interrupt_pending_ = true;
}

}  // namespace vioavr::core
