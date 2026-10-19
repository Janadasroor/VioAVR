#include "vioavr/core/spi.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <algorithm>

namespace vioavr::core {

static void spi_callback(u64 cycle, void* param) {
    static_cast<Spi*>(param)->on_event(cycle);
}

static void spi_edge_callback(u64 cycle, void* param) {
    static_cast<Spi*>(param)->on_edge(cycle);
}

namespace {
constexpr u8 kSpiInterruptFlag = 0x80U;
constexpr u8 kSpiEnable = 0x40U;
constexpr u8 kSpiMaster = 0x10U;
constexpr u8 kSpiInterruptEnable = 0x80U;
constexpr u8 kSpiCpha = 0x04U;
constexpr u8 kSpiCpol = 0x08U;
constexpr u8 kSpiDord = 0x20U;
}

Spi::Spi(std::string_view name, const SpiDescriptor& descriptor, PinMux* pin_mux) noexcept
    : name_(name), desc_(descriptor), pin_mux_(pin_mux)
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
    release_transfer_pins();
    spcr_ = 0U;
    spsr_ = 0U;
    spdr_ = 0U;
    shift_register_ = 0U;
    miso_buffer_ = 0U;
    transfer_cycles_left_ = 0U;
    transfer_in_progress_ = false;
    edges_done_ = 0U;
    rx_byte_ = 0U;
    half_bit_cycles_ = 0U;
    interrupt_pending_ = false;
    spsr_read_since_spif_ = false;
}

void Spi::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
    // Master transfers are stepped by the scheduler; a slave is clocked by
    // SCK edges coming from the pin, so it is serviced here.
    slave_service_pins();
}

void Spi::slave_drive_miso() noexcept {
    if (pin_mux_ == nullptr || desc_.miso_pin_address == 0U) return;
    if (!slave_miso_claimed_) {
        pin_mux_->claim_pin_by_address(desc_.miso_pin_address, desc_.miso_pin_bit,
                                       PinOwner::spi);
        slave_miso_claimed_ = true;
    }
    pin_mux_->update_pin_by_address(desc_.miso_pin_address, desc_.miso_pin_bit,
                                    PinOwner::spi, true, slave_miso_);
}

void Spi::slave_release_miso() noexcept {
    if (!slave_miso_claimed_ || pin_mux_ == nullptr || desc_.miso_pin_address == 0U) return;
    pin_mux_->update_pin_by_address(desc_.miso_pin_address, desc_.miso_pin_bit,
                                    PinOwner::spi, true, true); // idle high
    pin_mux_->release_pin_by_address(desc_.miso_pin_address, desc_.miso_pin_bit,
                                     PinOwner::spi);
    slave_miso_claimed_ = false;
}

void Spi::update_active_state() noexcept {
    if (bus_ == nullptr) return;
    const bool active = (spcr_ & desc_.spe_mask) != 0U &&
                        (spcr_ & desc_.mstr_mask) == 0U;
    bus_->set_peripheral_active(this, active);
}

void Spi::slave_service_pins() noexcept {
    const bool slave_mode = (spcr_ & desc_.spe_mask) != 0U &&
                            (spcr_ & desc_.mstr_mask) == 0U;
    if (!slave_mode) {
        slave_release_miso();
        slave_active_ = false;
        return;
    }
    if (pin_mux_ == nullptr || desc_.sck_pin_address == 0U ||
        desc_.mosi_pin_address == 0U) {
        return;
    }

    const PinState sck = pin_mux_->get_state_by_address(desc_.sck_pin_address,
                                                        desc_.sck_pin_bit);
    if (!slave_active_) {
        // Claim MISO and present the first bit while the bus is idle.
        const bool idle = (spcr_ & kSpiCpol) != 0U;
        slave_sck_last_ = sck.drive_level;
        if (sck.is_output && sck.drive_level == idle) {
            // shift_register_ holds the byte written to SPDR (slave path).
            const u8 first = (spcr_ & kSpiDord) != 0U
                ? static_cast<u8>(shift_register_ & 0x01U)
                : static_cast<u8>((shift_register_ & 0x80U) >> 7U);
            slave_miso_ = first != 0U;
            slave_active_ = true;
            slave_bits_ = 0U;
            slave_rx_ = 0U;
            slave_drive_miso();
        } else {
            slave_release_miso();
        }
    }

    if (!slave_active_ || !sck.is_output) return;

    const bool mosi = pin_mux_->get_state_by_address(desc_.mosi_pin_address,
                                                     desc_.mosi_pin_bit).drive_level;
    const bool cpol = (spcr_ & kSpiCpol) != 0U;
    const bool cpha = (spcr_ & kSpiCpha) != 0U;
    const bool level = sck.drive_level;
    if (level == slave_sck_last_) return;

    // CPHA=0 samples on the leading edge (rising when CPOL=0), CPHA=1 on the
    // trailing one (falling when CPOL=0, rising when CPOL=1).
    const bool sample_edge = cpha ? (level == (cpol != 0U)) : (level != (cpol != 0U));
    slave_sck_last_ = level;

    if (sample_edge) {
        // MOSI is shifted in MSB first, or LSB first with DORD.
        if ((spcr_ & kSpiDord) != 0U) {
            slave_rx_ = static_cast<u8>((slave_rx_ >> 1) | (mosi ? 0x80U : 0x00U));
        } else {
            slave_rx_ = static_cast<u8>((slave_rx_ << 1) | (mosi ? 1U : 0U));
        }
        ++slave_bits_;
    }

    // Present the next bit: CPHA=0 changes on the trailing edge (so it is
    // stable before the next sample), CPHA=1 on the leading edge.
    const bool output_edge = cpha ? sample_edge : !sample_edge;
    if (output_edge) {
        const u8 bit_index = static_cast<u8>(slave_bits_ % 8U);
        const u8 next = (spcr_ & kSpiDord) != 0U
            ? static_cast<u8>((shift_register_ >> bit_index) & 0x01U)
            : static_cast<u8>((shift_register_ >> (7U - bit_index)) & 0x01U);
        slave_miso_ = next != 0U;
        slave_drive_miso();
    }

    if (slave_bits_ >= 8U) {
        rx_byte_ = slave_rx_;
        spdr_ = rx_byte_;
        spsr_ |= desc_.spif_mask;
        interrupt_pending_ = true;
        update_interrupt_pending();
        slave_active_ = false;
        slave_bits_ = 0U;
        slave_rx_ = 0U;
    }
}

void Spi::on_event(u64 cycle) noexcept {
    (void)cycle;
    if (transfer_in_progress_) {
        return;  // Stepped transfers run through on_edge().
    }
    transfer_cycles_left_ = 0U;
    complete_transfer();
}

u8 Spi::read(const u16 address) noexcept
{
    if (address == desc_.spcr_address) {
        return spcr_;
    }
    if (address == desc_.spsr_address) {
        if ((spsr_ & desc_.spif_mask) != 0U) {
            spsr_read_since_spif_ = true;
        }
        return spsr_;
    }
    if (address == desc_.spdr_address) {
        if ((spsr_ & desc_.spif_mask) != 0U && spsr_read_since_spif_) {
            spsr_ &= static_cast<u8>(~desc_.spif_mask);
            interrupt_pending_ = false;
            update_interrupt_pending();
        }
        spsr_read_since_spif_ = false;
        return spdr_;
    }
    return 0U;
}

void Spi::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.spcr_address) {
        const bool was_enabled = (spcr_ & desc_.spe_mask) != 0U;
        const bool was_master = (spcr_ & desc_.mstr_mask) != 0U;
        spcr_ = value;
        if (was_enabled && (((value & desc_.spe_mask) == 0U) ||
                            (was_master && ((value & desc_.mstr_mask) == 0U)))) {
            // Disabling mid-transfer (or dropping out of master mode) aborts it.
            abort_transfer();
        }
        // A slave is clocked by SCK edges from the pin, so it has to be in
        // the bus ticking list (which is built at attach time).
        update_active_state();
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
            // Start transfer — clear SPIF so it can be set again on completion
            spsr_ &= ~desc_.spif_mask;
            interrupt_pending_ = false;
            u8 data = value;
            if ((spcr_ & kSpiDord) != 0U) { // DORD
                data = static_cast<u8>(
                    ((data & 0x01U) << 7U) | ((data & 0x02U) << 5U) |
                    ((data & 0x04U) << 3U) | ((data & 0x08U) << 1U) |
                    ((data & 0x10U) >> 1U) | ((data & 0x20U) >> 3U) |
                    ((data & 0x40U) >> 5U) | ((data & 0x80U) >> 7U)
                );
            }
            shift_register_ = data;

            if (pins_mapped()) {
                start_stepped_transfer(data);
            } else {
                const u32 divisor = sck_divisor();
                transfer_cycles_left_ = 8U * divisor;
                if (bus_) {
                    bus_->scheduler().schedule(transfer_cycles_left_, spi_callback, this);
                }
            }
        } else {
            shift_register_ = value;
        }
    }
    update_interrupt_pending();
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
        interrupt_pending_ = false;
        update_interrupt_pending();
        return true;
    }
    return false;
}

void Spi::update_interrupt_pending() noexcept {
    InterruptRequest req;
    set_interrupt_pending(pending_interrupt_request(req));
}

void Spi::inject_miso_byte(const u8 value) noexcept
{
    miso_buffer_ = value;
}

void Spi::trigger_slave_transfer(const u32 sck_bit_cycles) noexcept
{
    if ((spcr_ & kSpiEnable) != 0U && (spcr_ & kSpiMaster) == 0U) {
        // Slave transfer: 8 SCK cycles, each sck_bit_cycles CPU cycles long
        const u32 total_cycles = 8U * sck_bit_cycles;
        transfer_cycles_left_ = total_cycles;
        if (bus_) {
            bus_->scheduler().cancel(spi_callback, this);
            bus_->scheduler().schedule(total_cycles, spi_callback, this);
        }
    }
}

void Spi::on_ss_asserted() noexcept
{
    // Datasheet §18.3.2: SS low in master mode => MSTR cleared, SPIF set.
    if ((spcr_ & desc_.spe_mask) != 0U && (spcr_ & desc_.mstr_mask) != 0U) {
        spcr_ &= static_cast<u8>(~desc_.mstr_mask);
        // Abort any pending master transfer; the bus now belongs to the other master.
        abort_transfer();
        spsr_ |= desc_.spif_mask;
        interrupt_pending_ = true;
        update_interrupt_pending();
    }
}

bool Spi::pins_mapped() const noexcept
{
    return pin_mux_ != nullptr && desc_.sck_pin_address != 0U &&
           desc_.mosi_pin_address != 0U;
}

u32 Spi::sck_divisor() const noexcept
{
    const u8 spr = static_cast<u8>(spcr_ & 0x03U);
    const bool spi2x = (spsr_ & desc_.sp2x_mask) != 0U;
    switch (spr) {
    case 0: return spi2x ? 2U : 4U;
    case 1: return spi2x ? 8U : 16U;
    case 2: return spi2x ? 32U : 64U;
    default: return spi2x ? 64U : 128U;
    }
}

void Spi::claim_transfer_pins() noexcept
{
    if (!pin_mux_) return;
    if (desc_.sck_pin_address != 0U)
        pin_mux_->claim_pin_by_address(desc_.sck_pin_address, desc_.sck_pin_bit, PinOwner::spi);
    if (desc_.mosi_pin_address != 0U)
        pin_mux_->claim_pin_by_address(desc_.mosi_pin_address, desc_.mosi_pin_bit, PinOwner::spi);
    if (desc_.ss_pin_address != 0U)
        pin_mux_->claim_pin_by_address(desc_.ss_pin_address, desc_.ss_pin_bit, PinOwner::spi);
}

void Spi::release_transfer_pins() noexcept
{
    if (!pin_mux_) return;
    if (desc_.sck_pin_address != 0U)
        pin_mux_->release_pin_by_address(desc_.sck_pin_address, desc_.sck_pin_bit, PinOwner::spi);
    if (desc_.mosi_pin_address != 0U)
        pin_mux_->release_pin_by_address(desc_.mosi_pin_address, desc_.mosi_pin_bit, PinOwner::spi);
    if (desc_.ss_pin_address != 0U)
        pin_mux_->release_pin_by_address(desc_.ss_pin_address, desc_.ss_pin_bit, PinOwner::spi);
}

void Spi::drive_sck(bool level) noexcept
{
    if (!pin_mux_ || desc_.sck_pin_address == 0U) return;
    pin_mux_->update_pin_by_address(desc_.sck_pin_address, desc_.sck_pin_bit,
                                    PinOwner::spi, true, level);
}

void Spi::drive_mosi(bool level) noexcept
{
    if (!pin_mux_ || desc_.mosi_pin_address == 0U) return;
    pin_mux_->update_pin_by_address(desc_.mosi_pin_address, desc_.mosi_pin_bit,
                                    PinOwner::spi, true, level);
}

void Spi::drive_ss(bool level) noexcept
{
    if (!pin_mux_ || desc_.ss_pin_address == 0U) return;
    pin_mux_->update_pin_by_address(desc_.ss_pin_address, desc_.ss_pin_bit,
                                    PinOwner::spi, true, level);
}

bool Spi::sample_miso_wire(u8 wire_pos) noexcept
{
    // Prefer a real external driver (GPIO/testbench) when one drives MISO;
    // otherwise fall back to the host-injected byte (default 0x00).
    if (pin_mux_ && desc_.miso_pin_address != 0U) {
        const PinState state =
            pin_mux_->get_state_by_address(desc_.miso_pin_address, desc_.miso_pin_bit);
        if (state.is_output && state.owner != PinOwner::spi) {
            return state.drive_level;
        }
    }
    return ((miso_buffer_ >> wire_pos) & 0x01U) != 0U;
}

void Spi::start_stepped_transfer(u8 data) noexcept
{
    (void)data;
    const u32 divisor = sck_divisor();
    half_bit_cycles_ = divisor / 2U;
    if (half_bit_cycles_ == 0U) half_bit_cycles_ = 1U;
    transfer_cycles_left_ = 8U * divisor;
    transfer_in_progress_ = true;
    edges_done_ = 0U;
    rx_byte_ = 0U;

    const bool cpol = (spcr_ & kSpiCpol) != 0U;
    const bool cpha = (spcr_ & kSpiCpha) != 0U;
    claim_transfer_pins();
    drive_ss(false);       // Select slave.
    drive_sck(cpol);       // Idle level.
    if (!cpha) {
        // CPHA=0: first MOSI bit is set up when SS falls.
        drive_mosi((shift_register_ & 0x80U) != 0U);
    }
    if (bus_) {
        bus_->scheduler().schedule(half_bit_cycles_, spi_edge_callback, this);
    }
}

void Spi::on_edge(u64 cycle) noexcept
{
    (void)cycle;
    if (!transfer_in_progress_) return;

    const bool cpol = (spcr_ & kSpiCpol) != 0U;
    const bool cpha = (spcr_ & kSpiCpha) != 0U;
    const bool dord = (spcr_ & kSpiDord) != 0U;
    edges_done_++;
    const u8 k = edges_done_;  // 1..16
    const u8 bit = static_cast<u8>((k - 1U) / 2U);  // 0..7
    const bool leading = (k % 2U) == 1U;

    drive_sck(leading ? !cpol : cpol);
    const u8 wire_pos = dord ? bit : static_cast<u8>(7U - bit);
    if (leading) {
        if (!cpha) {
            // CPHA=0: sample MISO on the leading edge.
            if (sample_miso_wire(wire_pos)) {
                rx_byte_ |= static_cast<u8>(1U << wire_pos);
            }
        } else {
            // CPHA=1: set up MOSI on the leading edge.
            drive_mosi(((shift_register_ >> wire_pos) & 0x01U) != 0U);
        }
    } else {
        if (!cpha) {
            // CPHA=0: set up the next MOSI bit on the trailing edge.
            const u8 next = static_cast<u8>(bit + 1U);
            if (next < 8U) {
                const u8 next_pos = dord ? next : static_cast<u8>(7U - next);
                drive_mosi(((shift_register_ >> next_pos) & 0x01U) != 0U);
            }
        } else {
            // CPHA=1: sample MISO on the trailing edge.
            if (sample_miso_wire(wire_pos)) {
                rx_byte_ |= static_cast<u8>(1U << wire_pos);
            }
        }
    }

    if (k >= 16U) {
        transfer_in_progress_ = false;
        transfer_cycles_left_ = 0U;
        drive_sck(cpol);
        drive_ss(true);  // Release slave.
        spdr_ = rx_byte_;
        spsr_ = static_cast<u8>((spsr_ & ~desc_.wcol_mask) | desc_.spif_mask);
        interrupt_pending_ = true;
        update_interrupt_pending();
        release_transfer_pins();
        return;
    }
    if (bus_) {
        bus_->scheduler().schedule(half_bit_cycles_, spi_edge_callback, this);
    }
}

void Spi::abort_transfer() noexcept
{
    if (bus_) {
        bus_->scheduler().cancel(spi_callback, this);
        bus_->scheduler().cancel(spi_edge_callback, this);
    }
    transfer_cycles_left_ = 0U;
    transfer_in_progress_ = false;
    edges_done_ = 0U;
    release_transfer_pins();
}

u8 Spi::last_transmitted_byte() const noexcept
{
    return shift_register_;
}

bool Spi::busy() const noexcept
{
    return transfer_in_progress_ || transfer_cycles_left_ > 0U;
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
    spsr_ = static_cast<u8>((spsr_ & ~desc_.wcol_mask) | desc_.spif_mask);
    interrupt_pending_ = true;
    update_interrupt_pending();
}

bool Spi::power_reduction_enabled() const noexcept
{
    if (!bus_ || desc_.pr_address == 0U || desc_.pr_bit == 0xFFU) {
        return false;
    }
    return (bus_->read_data(desc_.pr_address) & (1U << desc_.pr_bit)) != 0;
}

}  // namespace vioavr::core
