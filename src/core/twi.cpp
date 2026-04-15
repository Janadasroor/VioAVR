#include "vioavr/core/twi.hpp"
#include <algorithm>

namespace vioavr::core {

namespace {
constexpr u8 kTwInt = 0x80U;
constexpr u8 kTwEa  = 0x40U;
constexpr u8 kTwSta = 0x20U;
constexpr u8 kTwSto = 0x10U;
constexpr u8 kTwWc  = 0x08U;
constexpr u8 kTwEn  = 0x04U;
constexpr u8 kTwIntEnable = 0x01U;

// Status codes
constexpr u8 kStatusStart          = 0x08U;
constexpr u8 kStatusRepStart       = 0x10U;
constexpr u8 kStatusSlaW_Ack       = 0x18U;
constexpr u8 kStatusSlaW_Nack      = 0x20U;
constexpr u8 kStatusDataW_Ack      = 0x28U;
constexpr u8 kStatusDataW_Nack     = 0x30U;
constexpr u8 kStatusArbitrationLost = 0x38U;
constexpr u8 kStatusSlaR_Ack       = 0x40U;
constexpr u8 kStatusSlaR_Nack      = 0x48U;
constexpr u8 kStatusDataR_Ack      = 0x50U;
constexpr u8 kStatusDataR_Nack     = 0x58U;

// Slave Receiver
constexpr u8 kStatusSlaveSlaW_Ack  = 0x60U;
constexpr u8 kStatusSlaveArbLostW  = 0x68U;
constexpr u8 kStatusSlaveGen_Ack   = 0x70U;
constexpr u8 kStatusSlaveArbLostGen = 0x78U;
constexpr u8 kStatusSlaveDataW_Ack = 0x80U;
constexpr u8 kStatusSlaveDataW_Nack = 0x88U;
constexpr u8 kStatusSlaveGenData_Ack = 0x90U;
constexpr u8 kStatusSlaveGenData_Nack = 0x98U;
constexpr u8 kStatusSlaveStop      = 0xA0U;

// Slave Transmitter
constexpr u8 kStatusSlaveSlaR_Ack  = 0xA8U;
constexpr u8 kStatusSlaveArbLostR  = 0xB0U;
constexpr u8 kStatusSlaveDataR_Ack = 0xB8U;
constexpr u8 kStatusSlaveDataR_Nack = 0xC0U;
constexpr u8 kStatusSlaveLastData_Ack = 0xC8U;

constexpr u8 kStatusNone           = 0xF8U;
constexpr u8 kStatusBusError       = 0x00U;
}

Twi::Twi(std::string_view name, const DeviceDescriptor& device) noexcept
    : name_(name), desc_(device.twis[0])
{
    std::vector<u16> addrs;
    auto add_if_valid = [&](u16 addr) { if (addr != 0U) addrs.push_back(addr); };
    add_if_valid(desc_.twbr_address);
    add_if_valid(desc_.twsr_address);
    add_if_valid(desc_.twar_address);
    add_if_valid(desc_.twdr_address);
    add_if_valid(desc_.twcr_address);
    add_if_valid(desc_.twamr_address);

    ranges_ = {{
        AddressRange{
            *std::min_element(addrs.begin(), addrs.end()),
            *std::max_element(addrs.begin(), addrs.end())
        }
    }};
}

std::string_view Twi::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> Twi::mapped_ranges() const noexcept
{
    return ranges_;
}

void Twi::reset() noexcept
{
    twbr_ = 0U;
    twsr_ = kStatusNone;
    twar_ = 0U;
    twdr_ = 0xFFU;
    twcr_ = 0U;
    twamr_ = 0U;
    mode_ = Mode::idle;
    step_cycles_left_ = 0U;
    interrupt_pending_ = false;
    sla_matched_ = false;
    general_call_matched_ = false;
    rx_idx_ = 0;
    tx_buffer_.clear();
}

void Twi::tick(const u64 elapsed_cycles) noexcept
{
    if (step_cycles_left_ == 0U) {
        return;
    }

    if (elapsed_cycles >= step_cycles_left_) {
        step_cycles_left_ = 0U;
        complete_step();
    } else {
        step_cycles_left_ -= static_cast<u32>(elapsed_cycles);
    }
}

u8 Twi::read(const u16 address) noexcept
{
    if (address == desc_.twbr_address) return twbr_;
    if (address == desc_.twsr_address) return twsr_;
    if (address == desc_.twar_address) return twar_;
    if (address == desc_.twdr_address) return twdr_;
    if (address == desc_.twcr_address) {
        return static_cast<u8>((twcr_ & ~desc_.twint_mask) | (interrupt_pending_ ? desc_.twint_mask : 0U));
    }
    if (address == desc_.twamr_address) return twamr_;
    return 0U;
}

void Twi::write(const u16 address, const u8 value) noexcept
{
    if (address == desc_.twbr_address) {
        twbr_ = value;
        return;
    }
    if (address == desc_.twsr_address) {
        twsr_ = static_cast<u8>((twsr_ & 0xF8U) | (value & 0x03U));
        return;
    }
    if (address == desc_.twar_address) {
        twar_ = value;
        return;
    }
    if (address == desc_.twdr_address) {
        twdr_ = value;
        return;
    }
    if (address == desc_.twcr_address) {
        twcr_ = value & ~desc_.twint_mask;

        if ((twcr_ & desc_.twen_mask) == 0U) {
            reset();
            return;
        }

        if ((value & desc_.twint_mask) != 0U) {
            interrupt_pending_ = false;
            
            const u8 prescaler_idx = static_cast<u8>(twsr_ & 0x03U);
            static const u32 prescalers[] = {1, 4, 16, 64};
            const u32 scl_divisor = 16U + 2U * twbr_ * prescalers[prescaler_idx];
            
            step_cycles_left_ = 9U * std::max(1U, scl_divisor);
        }

        if ((value & desc_.twsto_mask) != 0U && (value & desc_.twint_mask) == 0U) {
            step_cycles_left_ = 100U;
        }
    }
    if (address == desc_.twamr_address) {
        twamr_ = value;
        return;
    }
}

bool Twi::pending_interrupt_request(InterruptRequest& request) const noexcept
{
    if (interrupt_pending_ && (twcr_ & desc_.twie_mask) != 0U) {
        request = {desc_.vector_index, 0U};
        return true;
    }
    return false;
}

bool Twi::consume_interrupt_request(InterruptRequest& request) noexcept
{
    if (pending_interrupt_request(request)) {
        return true;
    }
    return false;
}

void Twi::set_rx_buffer(const std::vector<u8>& data) noexcept
{
    rx_buffer_ = data;
    rx_idx_ = 0;
}

const std::vector<u8>& Twi::tx_buffer() const noexcept
{
    return tx_buffer_;
}

bool Twi::busy() const noexcept
{
    return step_cycles_left_ > 0U;
}

bool Twi::check_slave_address(u8 address) const noexcept
{
    const u8 own_addr = (twar_ >> 1U);
    const u8 target_addr = (address >> 1U);
    const u8 mask = (twamr_ >> 1U);
    
    // Match own address with mask
    if ((own_addr & ~mask) == (target_addr & ~mask)) {
        return true;
    }
    
    // Check general call if enabled (TWGCE bit 0 of TWAR)
    if ((twar_ & 0x01U) != 0U && target_addr == 0U) {
        return true;
    }
    
    return false;
}

void Twi::complete_step() noexcept
{
    if ((twcr_ & desc_.twsto_mask) != 0U) {
        // STOP completed
        twsr_ = kStatusNone;
        twcr_ &= static_cast<u8>(~desc_.twsto_mask);
        mode_ = Mode::idle;
        interrupt_pending_ = true;
        return;
    }

    if ((twcr_ & desc_.twsta_mask) != 0U) {
        // START or Repeated START
        const u8 status = (twsr_ & 0xF8U);
        if (status == kStatusNone || status == kStatusSlaveStop) {
             twsr_ = kStatusStart;
        } else {
             twsr_ = kStatusRepStart;
        }
        twcr_ &= static_cast<u8>(~desc_.twsta_mask);
        mode_ = Mode::idle; // Mode will be determined by SLA+W/R
        interrupt_pending_ = true;
        return;
    }

    switch (mode_) {
    case Mode::master_tx:
    case Mode::master_rx:
    case Mode::idle:
        handle_master_step();
        break;
    case Mode::slave_tx:
    case Mode::slave_rx:
        handle_slave_step();
        break;
    }

    interrupt_pending_ = true;
}

void Twi::handle_master_step() noexcept
{
    const u8 status = (twsr_ & 0xF8U);
    
    switch (status) {
    case kStatusStart:
    case kStatusRepStart:
        // SLA+W/R sent
        if ((twdr_ & 0x01U) == 0U) { // W
            mode_ = Mode::master_tx;
            twsr_ = kStatusSlaW_Ack; // Assuming ACK
        } else { // R
            mode_ = Mode::master_rx;
            twsr_ = kStatusSlaR_Ack; // Assuming ACK
        }
        break;
        
    case kStatusSlaW_Ack:
    case kStatusDataW_Ack:
        // DATA sent
        tx_buffer_.push_back(twdr_);
        twsr_ = kStatusDataW_Ack;
        break;
        
    case kStatusSlaR_Ack:
    case kStatusDataR_Ack:
        // DATA received
        if (rx_idx_ < rx_buffer_.size()) {
            twdr_ = rx_buffer_[rx_idx_++];
            if ((twcr_ & desc_.twea_mask) != 0U) {
                twsr_ = kStatusDataR_Ack;
            } else {
                twsr_ = kStatusDataR_Nack;
            }
        } else {
            twdr_ = 0xFFU;
            twsr_ = kStatusDataR_Nack;
        }
        break;
        
    default:
        twsr_ = kStatusNone;
        break;
    }
}

void Twi::handle_slave_step() noexcept
{
    // Basic slave skeleton - real slave matching happens when master sends SLA+W/R
    // In a real I2C bus simulation, we'd need a way to "see" master traffic.
    // For now, we'll keep it Master-centric but prepared for Slave.
}

}  // namespace vioavr::core
