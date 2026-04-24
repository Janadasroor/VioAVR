#include "vioavr/core/twi.hpp"
#include "vioavr/core/memory_bus.hpp"
#include <algorithm>

namespace vioavr::core {

namespace {
// TWI Status Codes
constexpr u8 kStatusStart           = 0x08U;
constexpr u8 kStatusRepStart        = 0x10U;
constexpr u8 kStatusMasterTxAddrAck = 0x18U;
constexpr u8 kStatusMasterTxAddrNack= 0x20U;
constexpr u8 kStatusMasterTxDataAck = 0x28U;
constexpr u8 kStatusMasterTxDataNack= 0x30U;
constexpr u8 kStatusArbitrationLost = 0x38U;
constexpr u8 kStatusMasterRxAddrAck = 0x40U;
constexpr u8 kStatusMasterRxAddrNack= 0x48U;
constexpr u8 kStatusMasterRxDataAck = 0x50U;
constexpr u8 kStatusMasterRxDataNack= 0x58U;

constexpr u8 kStatusSlaveRxAddrAck  = 0x60U;
constexpr u8 kStatusSlaveRxArbLostAck = 0x68U;
constexpr u8 kStatusSlaveRxGenAck   = 0x70U;
constexpr u8 kStatusSlaveRxGenArbLostAck = 0x78U;
constexpr u8 kStatusSlaveRxDataAck  = 0x80U;
constexpr u8 kStatusSlaveRxDataNack = 0x88U;
constexpr u8 kStatusSlaveRxGenDataAck = 0x90U;
constexpr u8 kStatusSlaveRxGenDataNack = 0x98U;
constexpr u8 kStatusSlaveStop       = 0xA0U;

constexpr u8 kStatusSlaveTxAddrAck  = 0xA8U;
constexpr u8 kStatusSlaveTxArbLostAck = 0xB0U;
constexpr u8 kStatusSlaveTxDataAck  = 0xB8U;
constexpr u8 kStatusSlaveTxDataNack = 0xC0U;
constexpr u8 kStatusSlaveTxLastDataAck = 0xC8U;

constexpr u8 kStatusNoInfo          = 0xF8U;
constexpr u8 kStatusBusError       = 0x00U;
}

Twi::Twi(std::string_view name, const TwiDescriptor& descriptor) noexcept
    : name_(name), desc_(descriptor)
{
    const std::array<u16, 6> addrs = {
        desc_.twbr_address, desc_.twsr_address, desc_.twar_address,
        desc_.twdr_address, desc_.twcr_address, desc_.twamr_address
    };
    ranges_[0] = {
        *std::min_element(addrs.begin(), addrs.end()),
        *std::max_element(addrs.begin(), addrs.end())
    };
}

std::string_view Twi::name() const noexcept { return name_; }
std::span<const AddressRange> Twi::mapped_ranges() const noexcept { return ranges_; }

void Twi::reset() noexcept
{
    twbr_ = 0;
    twsr_ = 0xF8U;
    twar_ = 0;
    twdr_ = 0xFFU;
    twcr_ = 0;
    twamr_ = 0;
    mode_ = Mode::idle;
    step_cycles_left_ = 0;
    interrupt_pending_ = false;
    sla_matched_ = false;
    general_call_matched_ = false;
    rx_idx_ = 0;
    rx_buffer_.clear();
    tx_buffer_.clear();
}

void Twi::tick(const u64 elapsed_cycles) noexcept
{
    if (power_reduction_enabled()) return;

    if (step_cycles_left_ > 0) {
        if (elapsed_cycles >= step_cycles_left_) {
            step_cycles_left_ = 0;
            complete_step();
        } else {
            step_cycles_left_ -= static_cast<u32>(elapsed_cycles);
        }
    }
}

u8 Twi::read(const u16 address) noexcept
{
    if (address == desc_.twbr_address) return twbr_;
    if (address == desc_.twsr_address) return twsr_;
    if (address == desc_.twar_address) return twar_;
    if (address == desc_.twdr_address) return twdr_;
    if (address == desc_.twcr_address) {
        u8 val = twcr_;
        if (interrupt_pending_) val |= desc_.twint_mask;
        return val;
    }
    if (address == desc_.twamr_address) return twamr_;
    return 0U;
}

void Twi::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.twbr_address) twbr_ = value;
    else if (address == desc_.twsr_address) twsr_ = (twsr_ & 0xF8U) | (value & 0x03U);
    else if (address == desc_.twar_address) twar_ = value;
    else if (address == desc_.twdr_address) twdr_ = value;
    else if (address == desc_.twcr_address) {
        // TWINT is cleared by writing 1
        if (value & desc_.twint_mask) {
            interrupt_pending_ = false;
            
            // Trigger next step if TWEN is set
            if (value & desc_.twen_mask) {
                // Calculate bit rate timing
                // F_SCL = CPU_CLK / (16 + 2*TWBR * 4^Prescaler)
                u32 prescaler = 1 << (2 * (twsr_ & 0x03U));
                u32 divisor = 16 + 2 * twbr_ * prescaler;
                step_cycles_left_ = divisor; // One TWI cycle
            }
        }
        twcr_ = value & ~desc_.twint_mask; // Clear TWINT internally
    }
    else if (address == desc_.twamr_address) twamr_ = value;
}

bool Twi::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if (interrupt_pending_ && (twcr_ & desc_.twie_mask)) {
        request = {desc_.vector_index, 0U};
        return true;
    }
    return false;
}

bool Twi::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (pending_interrupt_request(request)) return true;
    return false;
}

void Twi::complete_step() noexcept
{
    // The state transition happens when TWINT is cleared.
    // Based on the current mode and bits in TWCR (STA, STO, EA).
    
    if (twcr_ & desc_.twsta_mask) {
        // Master: Send START
        const Mode prev_mode = mode_;
        mode_ = Mode::master_tx;
        twsr_ = (prev_mode == Mode::idle) ? kStatusStart : kStatusRepStart;
        twcr_ &= ~desc_.twsta_mask; // STA is not automatically cleared in hardware until next action? 
        // Actually, STA remains set until SLA+W/R is sent if I recall. 
        // But for simulation, we'll clear it after the start condition is recorded.
    } else if (twcr_ & desc_.twsto_mask) {
        // Master: Send STOP
        mode_ = Mode::idle;
        twsr_ = kStatusNoInfo;
        twcr_ &= ~desc_.twsto_mask;
        return; // STOP does not set TWINT
    } else {
        // Normal data/addr transfer
        switch (mode_) {
            case Mode::master_tx:
                handle_master_step();
                break;
            case Mode::master_rx:
                handle_master_step();
                break;
            case Mode::slave_rx:
            case Mode::slave_tx:
                handle_slave_step();
                break;
            case Mode::idle:
                // Check if we were addressed as slave (Passive monitoring)
                // This is a simplification for simulation.
                break;
        }
    }
    
    interrupt_pending_ = true;
}

void Twi::handle_master_step() noexcept
{
    if (twsr_ == kStatusStart || twsr_ == kStatusRepStart) {
        // SLA+W/R just sent
        const bool is_read = (twdr_ & 0x01U);
        if (is_read) {
            mode_ = Mode::master_rx;
            // Check if slave exists in simulation
            if (rx_idx_ < rx_buffer_.size() || sla_matched_) {
                twsr_ = kStatusMasterRxAddrAck;
            } else {
                twsr_ = kStatusMasterRxAddrNack;
            }
        } else {
            mode_ = Mode::master_tx;
            twsr_ = kStatusMasterTxAddrAck; // Assume success for now
        }
    } else if (mode_ == Mode::master_tx) {
        // Data byte just sent
        twsr_ = kStatusMasterTxDataAck;
        tx_buffer_.push_back(twdr_);
    } else if (mode_ == Mode::master_rx) {
        // Data byte just received
        if (rx_idx_ < rx_buffer_.size()) {
            twdr_ = rx_buffer_[rx_idx_++];
            twsr_ = (twcr_ & desc_.twea_mask) ? kStatusMasterRxDataAck : kStatusMasterRxDataNack;
        } else {
            twsr_ = kStatusMasterRxDataNack;
        }
    }
}

void Twi::handle_slave_step() noexcept
{
    // Slave mode logic using TWAR and TWAMR
    // Twi::check_slave_address is called by the bus simulation to addressed us
}

bool Twi::check_slave_address(u8 address) const noexcept
{
    u8 my_addr = (twar_ >> 1U);
    u8 mask = (twamr_ >> 1U);
    u8 target = (address >> 1U);
    
    // bits set in TWAMR mask out comparison
    if ((target & ~mask) == (my_addr & ~mask)) {
        return true;
    }
    
    // General call?
    if (address == 0 && (twar_ & 0x01U)) {
        return true;
    }
    
    return false;
}

void Twi::set_rx_buffer(const std::vector<u8>& data) noexcept
{
    rx_buffer_ = data;
    rx_idx_ = 0;
}

const std::vector<u8>& Twi::tx_buffer() const noexcept { return tx_buffer_; }
bool Twi::busy() const noexcept { return step_cycles_left_ > 0; }

bool Twi::power_reduction_enabled() const noexcept
{
    if (!bus_ || desc_.pr_address == 0U || desc_.pr_bit == 0xFFU) {
        return false;
    }
    return (bus_->read_data(desc_.pr_address) & (1U << desc_.pr_bit)) != 0;
}

} // namespace vioavr::core
