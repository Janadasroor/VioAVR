#include "vioavr/core/spi.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <algorithm>

namespace vioavr::core {

namespace {
constexpr u8 kSpiInterruptFlag = 0x80U;
constexpr u8 kSpiEnable = 0x40U;
constexpr u8 kSpiMaster = 0x10U;
constexpr u8 kSpiInterruptEnable = 0x80U;
}

Spi::Spi(std::string_view name, const SpiDescriptor& descriptor) noexcept
    : name_(name), desc_(descriptor)
{
    const std::array<u16, 3> addrs = {desc_.spcr_address, desc_.spsr_address, desc_.spdr_address};
    ranges_[0] = {
        *std::min_element(addrs.begin(), addrs.end()),
        *std::max_element(addrs.begin(), addrs.end())
    };
}

std::string_view Spi::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> Spi::mapped_ranges() const noexcept
{
    return ranges_;
}

ClockDomain Spi::clock_domain() const noexcept
{
    return ClockDomain::io;
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
    if (power_reduction_enabled()) return;

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
        return spsr_;
    }
    if (address == desc_.spdr_address) {
        if ((spsr_ & desc_.spif_mask) != 0U) {
            spsr_ &= static_cast<u8>(~desc_.spif_mask);
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
        spsr_ = static_cast<u8>((spsr_ & ~desc_.sp2x_mask) | (value & desc_.sp2x_mask));
        return;
    }
    if (address == desc_.spdr_address) {
        if ((spcr_ & desc_.spe_mask) == 0U) {
            spdr_ = value;
            return;
        }

        if (transfer_cycles_left_ > 0U) {
            spsr_ |= desc_.wcol_mask;
            return;
        }

        if ((spcr_ & desc_.mstr_mask) != 0U) {
            // Start transfer
            u8 data = value;
            if ((spcr_ & 0x20U) != 0U) { // DORD
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
            const bool spi2x = (spsr_ & desc_.sp2x_mask) != 0U;

            switch (spr) {
            case 0: divisor = spi2x ? 2U : 4U; break;
            case 1: divisor = spi2x ? 8U : 16U; break;
            case 2: divisor = spi2x ? 32U : 64U; break;
            case 3: divisor = spi2x ? 64U : 128U; break;
            }

            transfer_cycles_left_ = 8U * divisor;
        } else {
            shift_register_ = value;
        }
    }
}

bool Spi::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if (interrupt_pending_ && (spcr_ & desc_.spie_mask) != 0U) {
        request = {desc_.vector_index, 0U};
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
    spsr_ |= desc_.spif_mask;
    interrupt_pending_ = true;
}

bool Spi::power_reduction_enabled() const noexcept
{
    if (!bus_ || desc_.pr_address == 0U || desc_.pr_bit == 0xFFU) {
        return false;
    }
    return (bus_->read_data(desc_.pr_address) & (1U << desc_.pr_bit)) != 0;
}

}  // namespace vioavr::core
