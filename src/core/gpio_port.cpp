#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <algorithm>

namespace vioavr::core {

GpioPort::GpioPort(const std::string_view name, const u16 base_address, PinMux& pin_mux) noexcept
    : GpioPort(name, base_address, static_cast<u16>(base_address + 1U), static_cast<u16>(base_address + 2U), pin_mux)
{
}

GpioPort::GpioPort(std::string_view name, u16 pin_address, u16 ddr_address, u16 port_address, PinMux& pin_mux) noexcept
    : pin_mux_(&pin_mux),
      name_(name),
      desc_{name, pin_address, ddr_address, port_address},
      ranges_({AddressRange{pin_address, pin_address}, AddressRange{ddr_address, ddr_address}, AddressRange{port_address, port_address}}),
      num_ranges_(3U)
{
    if (name_.length() >= 5 && name_.starts_with("PORT")) {
        port_idx_ = static_cast<u8>(name_[4] - 'A');
        pin_mux_->register_port(pin_address, port_idx_);
        pin_mux_->register_port(ddr_address, port_idx_);
        pin_mux_->register_port(port_address, port_idx_);
    }
}

GpioPort::GpioPort(const PortDescriptor& desc, PinMux& pin_mux) noexcept
    : pin_mux_(&pin_mux),
      name_(desc.name),
      desc_(desc)
{
    // Infer port index from name (e.g., "PORTA" -> 0)
    if (name_.length() >= 5 && name_.starts_with("PORT")) {
        port_idx_ = static_cast<u8>(name_[4] - 'A');
        
        // Find all non-zero addresses in descriptor
        std::vector<u16> addrs = { 
            desc_.pin_address, desc_.ddr_address, desc_.port_address,
            desc_.dirset_address, desc_.dirclr_address, desc_.dirtgl_address,
            desc_.outset_address, desc_.outclr_address, desc_.outtgl_address
        };
        for (u8 i=0; i<8; ++i) {
            if (desc_.pin_ctrl_base) addrs.push_back(desc_.pin_ctrl_base + i);
        }
        if (desc_.vport_base) {
            addrs.push_back(desc_.vport_base + 0); // VPORT DIR
            addrs.push_back(desc_.vport_base + 1); // VPORT OUT
            addrs.push_back(desc_.vport_base + 2); // VPORT IN
            addrs.push_back(desc_.vport_base + 3); // VPORT INTFLAGS
        }
        
        std::sort(addrs.begin(), addrs.end());
        addrs.erase(std::unique(addrs.begin(), addrs.end()), addrs.end());
        
        size_t ri = 0;
        for (u16 addr : addrs) {
            if (addr == 0) continue;
            if (ri > 0 && addr == ranges_[ri-1].end + 1) {
                ranges_[ri-1].end = addr;
            } else if (ri < ranges_.size()) {
                ranges_[ri++] = {addr, addr};
            }
        }
        num_ranges_ = static_cast<u8>(ri);
    }
}

std::string_view GpioPort::name() const noexcept
{
    return name_;
}

std::span<const AddressRange> GpioPort::mapped_ranges() const noexcept
{
    return std::span<const AddressRange>(ranges_.data(), num_ranges_);
}

void GpioPort::reset() noexcept
{
    ddr_ = 0U;
    port_ = 0U;
    pin_ = 0U;
    pin_latched_ = 0U;
    external_levels_ = 0U;
    pending_changes_mask_ = 0U;
}

void GpioPort::tick(const u64 elapsed_cycles) noexcept
{
    (void)elapsed_cycles;
    (void)sample_levels();
}

u8 GpioPort::read(const u16 address) noexcept
{
    if (address == desc_.pin_address) {
        update_pin_latched();
        return pin_latched_;
    }
    if (address == desc_.ddr_address) return ddr_;
    if (address == desc_.port_address) return port_;
    
    // Mega-0 Aliases (DIR and OUT reads)
    if (address == desc_.dirset_address || address == desc_.dirclr_address || address == desc_.dirtgl_address) return ddr_;
    if (address == desc_.outset_address || address == desc_.outclr_address || address == desc_.outtgl_address) return port_;
    
    // VPORT
    if (desc_.vport_base != 0) {
        if (address == desc_.vport_base + 0) return ddr_;
        if (address == desc_.vport_base + 1) return port_;
        if (address == desc_.vport_base + 2) {
            update_pin_latched();
            return pin_latched_;
        }
        if (address == desc_.vport_base + 3) return 0; // INTFLAGS not fully modeled in GPIO right now, handled by PORTINT/PCINT? Actually Mega-0 PORT has an INTFLAGS register per PORT!
    }
    return 0U;
}

void GpioPort::write(const u16 address, const u8 value) noexcept
{
    auto apply_port = [&](u8 new_val) {
        const u8 changed = static_cast<u8>(port_ ^ new_val);
        port_ = new_val;
        pending_changes_mask_ |= (changed & ddr_);
        update_pin_latched();
    };

    if (address == desc_.pin_address) {
        apply_port(port_ ^ value);
        return; // Writing to PIN toggles
    }
    if (address == desc_.ddr_address) { ddr_ = value; update_pin_latched(); return; }
    if (address == desc_.port_address) { apply_port(value); return; }
    
    // Mega-0
    if (address == desc_.dirset_address) { ddr_ |= value; update_pin_latched(); return; }
    if (address == desc_.dirclr_address) { ddr_ &= ~value; update_pin_latched(); return; }
    if (address == desc_.dirtgl_address) { ddr_ ^= value; update_pin_latched(); return; }
    
    if (address == desc_.outset_address) { apply_port(port_ | value); return; }
    if (address == desc_.outclr_address) { apply_port(port_ & ~value); return; }
    if (address == desc_.outtgl_address) { apply_port(port_ ^ value); return; }
    
    // VPORT
    if (desc_.vport_base != 0) {
        if (address == desc_.vport_base + 0) { ddr_ = value; update_pin_latched(); return; }
        if (address == desc_.vport_base + 1) { apply_port(value); return; }
        if (address == desc_.vport_base + 2) { apply_port(port_ ^ value); return; }
    }
}

bool GpioPort::consume_pin_change(PinStateChange& change) noexcept
{
    if (pending_changes_mask_ == 0U) {
        return false;
    }

    for (u8 i = 0; i < 8; ++i) {
        if ((pending_changes_mask_ & (1U << i)) != 0U) {
            change.port_name = name_;
            change.bit_index = i;
            change.level = (port_ & (1U << i)) != 0U;
            pending_changes_mask_ &= static_cast<u8>(~(1U << i));
            return true;
        }
    }

    return false;
}

bool GpioPort::on_external_pin_change(u8 bit_index, PinLevel level) noexcept
{
    if (bit_index >= 8) return false;

    const u8 mask = (1U << bit_index);
    const u8 old_levels = external_levels_;
    
    if (level == PinLevel::high) {
        external_levels_ |= mask;
    } else {
        external_levels_ &= static_cast<u8>(~mask);
    }

    if (old_levels != external_levels_) {
        update_pin_latched();
        return true;
    }
    
    return false;
}

void GpioPort::set_input_levels(const u8 levels) noexcept
{
    external_levels_ = levels;
    update_pin_latched();
}

void GpioPort::update_pin_latched() noexcept
{
    u8 next_pin = 0;
    for (u8 i = 0; i < 8; ++i) {
        bool is_out = (ddr_ & (1 << i));
        bool level = (port_ & (1 << i));
        bool pullup = !is_out && level;

        // Notify PinMux of GPIO's intended state
        if (pin_mux_) {
            pin_mux_->update_pin(port_idx_, i, PinOwner::gpio, is_out, level, pullup);
            
            // Read back effective state
            auto state = pin_mux_->get_state(port_idx_, i);
            if (state.is_output) {
                if (state.drive_level) next_pin |= (1 << i);
            } else {
                // If it's an input, it reads the external level
                // (PinMux should ideally let us set the external level)
                if (external_levels_ & (1 << i)) next_pin |= (1 << i);
            }
        } else {
            // Fallback if no PinMux
            if (is_out) {
                if (level) next_pin |= (1 << i);
            } else {
                if (external_levels_ & (1 << i)) next_pin |= (1 << i);
                else if (pullup) next_pin |= (1 << i);
            }
        }
    }

    if (next_pin != pin_latched_) {
        pin_latched_ = next_pin;
    }
}

void GpioPort::bind_input_signal(const u8 bit_index, const AnalogSignalBank& bank, const u8 channel,
                                 const DigitalThresholdConfig threshold) noexcept
{
    if (bit_index >= 8) return;
    
    pin_bindings_[bit_index] = {
        .bank = &bank,
        .channel = channel,
        .threshold = threshold,
        .active = true
    };
    has_analog_binding_[bit_index] = true;
    (void)sample_levels();
}

void GpioPort::set_input_voltage(const u8 bit_index, const double normalized_voltage) noexcept
{
    if (bit_index >= 8) return;
    pin_voltages_[bit_index] = normalized_voltage;
    has_voltage_input_[bit_index] = true;
    
    apply_pin_voltage(bit_index, normalized_voltage, pin_bindings_[bit_index].threshold);
}

u8 GpioPort::sample_levels() noexcept
{
    for (u8 i = 0; i < 8; ++i) {
        if (has_analog_binding_[i]) {
            const double voltage = pin_bindings_[i].bank->voltage(pin_bindings_[i].channel);
            apply_pin_voltage(i, voltage, pin_bindings_[i].threshold);
        }
    }
    return pin_latched_;
}

void GpioPort::apply_pin_voltage(const u8 bit_index, const double voltage, const DigitalThresholdConfig threshold) noexcept
{
    const bool current_level = (pin_latched_ & (1U << bit_index)) != 0U;
    const bool next_level = apply_schmitt_threshold(current_level, voltage, threshold);
    
    if (current_level != next_level) {
        u8 levels = pin_latched_;
        if (next_level) levels |= (1U << bit_index);
        else levels &= static_cast<u8>(~(1U << bit_index));
        set_input_levels(levels);
    }
}

}  // namespace vioavr::core
