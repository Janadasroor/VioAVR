#include "vioavr/core/ircom.hpp"
#include "vioavr/core/memory_bus.hpp"

namespace vioavr::core {

Ircom::Ircom(std::string_view name, const IrcomDescriptor& desc) noexcept
    : name_(name), desc_(desc)
{
    u16 base = desc_.ctrl_address ? desc_.ctrl_address : desc_.txplctrl_address;
    u16 end = base;
    if (desc_.ctrl_address && desc_.ctrl_address > end) end = desc_.ctrl_address;
    if (desc_.txplctrl_address && desc_.txplctrl_address > end) end = desc_.txplctrl_address;
    if (desc_.rxplctrl_address && desc_.rxplctrl_address > end) end = desc_.rxplctrl_address;
    if (end > base) {
        ranges_[0] = {base, end};
        num_ranges_ = 1;
    } else if (base) {
        ranges_[0] = {base, base};
        num_ranges_ = 1;
    }
}

std::span<const AddressRange> Ircom::mapped_ranges() const noexcept {
    return {ranges_.data(), num_ranges_};
}

void Ircom::reset() noexcept {
    ctrl_ = 0;
    txplctrl_ = 0;
    rxplctrl_ = 0;
    modulator_counter_ = 0;
    modulator_high_ = false;
    last_driven_level_ = false;
    rx_current_cycle_ = 0;
    rx_last_cycle_ = 0;
    rx_last_level_ = false;
    rx_busy_ = false;
    rx_pulse_counter_ = 0;
    rx_shift_reg_ = 0;
    rx_bit_count_ = 0;
}

void Ircom::drive_output() noexcept {
    if (!pin_mux_ || port_idx_ >= 6 || bit_idx_ >= 8) return;
    bool level = modulator_high_;
    if (level == last_driven_level_) return;
    last_driven_level_ = level;
    if ((ctrl_ & 0x01)) {
        pin_mux_->claim_pin(port_idx_, bit_idx_, PinOwner::ircom);
        pin_mux_->update_pin(port_idx_, bit_idx_, PinOwner::ircom, true, level, false);
    } else {
        pin_mux_->release_pin(port_idx_, bit_idx_, PinOwner::ircom);
    }
}

void Ircom::check_rx_pulse() noexcept {
    if (!(ctrl_ & 0x02)) return;
    u64 elapsed = rx_current_cycle_ - rx_last_cycle_;
    rx_last_cycle_ = rx_current_cycle_;

    u8 threshold = rxplctrl_ > 0 ? rxplctrl_ : 1;
    if (elapsed >= static_cast<u64>(threshold)) {
        ++rx_pulse_counter_;
        rx_busy_ = true;
    }
}

bool Ircom::on_external_pin_change(u16 pin_address, u8 bit_index, PinLevel level) noexcept {
    if (!(ctrl_ & 0x02)) return false;
    if (pin_address != desc_.pin_address || bit_index != desc_.pin_bit_index) return false;

    check_rx_pulse();

    rx_last_level_ = (level == PinLevel::high);
    return true;
}

void Ircom::tick(u64 elapsed_cycles) noexcept {
    rx_current_cycle_ += elapsed_cycles;

    bool tx_enabled = (ctrl_ & 0x01) != 0;
    bool rx_enabled = (ctrl_ & 0x02) != 0;

    // TX modulator
    if (tx_enabled) {
        u8 pl = txplctrl_ > 0 ? txplctrl_ : 1;
        u32 half_period = pl;
        for (u64 i = 0; i < elapsed_cycles; ++i) {
            ++modulator_counter_;
            if (modulator_counter_ >= half_period) {
                modulator_counter_ = 0;
                modulator_high_ = !modulator_high_;
                drive_output();
            }
        }
    } else {
        if (modulator_high_) {
            modulator_high_ = false;
            drive_output();
        }
        modulator_counter_ = 0;
    }

    // RX: if no pin change events came in, just track time via tick
    (void)rx_enabled;
}

u8 Ircom::read(u16 address) noexcept {
    if (address == desc_.ctrl_address)     return ctrl_;
    if (address == desc_.txplctrl_address) return txplctrl_;
    if (address == desc_.rxplctrl_address) return rxplctrl_;
    return 0;
}

void Ircom::write(u16 address, u8 value) noexcept {
    if (address == desc_.ctrl_address) {
        bool was_enabled = (ctrl_ & 0x01) != 0;
        bool was_rx = (ctrl_ & 0x02) != 0;
        ctrl_ = value;
        bool now_enabled = (ctrl_ & 0x01) != 0;
        bool now_rx = (ctrl_ & 0x02) != 0;

        if (!now_enabled) {
            modulator_high_ = false;
            modulator_counter_ = 0;
            drive_output();
        } else if (!was_enabled && now_enabled) {
            modulator_counter_ = 0;
            modulator_high_ = false;
        }

        if (was_rx && !now_rx) {
            rx_busy_ = false;
            rx_pulse_counter_ = 0;
        } else if (!was_rx && now_rx) {
            rx_busy_ = false;
            rx_pulse_counter_ = 0;
            rx_bit_count_ = 0;
            rx_shift_reg_ = 0;
            rx_last_cycle_ = rx_current_cycle_;
            rx_last_level_ = false;
        }
    }
    else if (address == desc_.txplctrl_address) {
        txplctrl_ = value;
    }
    else if (address == desc_.rxplctrl_address) {
        rxplctrl_ = value;
    }
}

bool Ircom::pending_interrupt_request(InterruptRequest& request) const noexcept {
    if (desc_.vector_index && rx_busy_) {
        request.vector_index = desc_.vector_index;
        return true;
    }
    return false;
}

bool Ircom::consume_interrupt_request(InterruptRequest& request) noexcept {
    if (!desc_.vector_index || !rx_busy_) return false;
    request.vector_index = desc_.vector_index;
    rx_busy_ = false;
    return true;
}

} // namespace vioavr::core
