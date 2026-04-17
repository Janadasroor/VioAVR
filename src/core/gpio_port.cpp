#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/pin_mux.hpp"
#include <algorithm>

namespace vioavr::core {

GpioPort::GpioPort(const std::string_view name, const u16 base_address, PinMux& pin_mux) noexcept
    : GpioPort(name, base_address, static_cast<u16>(base_address + 1U), static_cast<u16>(base_address + 2U), pin_mux)
{
}

GpioPort::GpioPort(std::string_view name, u16 pin_address, u16 ddr_address, u16 port_address, PinMux& pin_mux) noexcept
    : name_(name),
      ranges_({AddressRange{pin_address, pin_address}, AddressRange{ddr_address, ddr_address}, AddressRange{port_address, port_address}}),
      pin_addr_(pin_address),
      ddr_addr_(ddr_address),
      port_addr_(port_address),
      num_ranges_(3U),
      pin_mux_(&pin_mux)
{
    // Infer port index from name (e.g., "PORTA" -> 0)
    if (name.length() >= 5 && name.starts_with("PORT")) {
        port_idx_ = static_cast<u8>(name[4] - 'A');
        pin_mux_->register_port(pin_address, port_idx_);
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
    sample_levels();
}

u8 GpioPort::read(const u16 address) noexcept
{
    if (address == pin_addr_) {
        // PIN register read: returns the current value of the pins
        // If it's an output, it typically returns the port value
        // If it's an input, it returns the external level
        return pin_latched_;
    }
    if (address == ddr_addr_) {
        return ddr_;
    }
    if (address == port_addr_) {
        return port_;
    }
    return 0U;
}

void GpioPort::write(const u16 address, const u8 value) noexcept
{
    if (address == pin_addr_) {
        // Writing to PIN register toggles bits in PORT register
        write(port_addr_, static_cast<u8>(port_ ^ value));
        update_pin_latched();
        return;
    }

    if (address == ddr_addr_) {
        ddr_ = value;
        update_pin_latched();
        return;
    }

    if (address == port_addr_) {
        const u8 changed = static_cast<u8>(port_ ^ value);
        port_ = value;
        // Any output pin that changed its value needs to publish it
        pending_changes_mask_ |= (changed & ddr_);
        update_pin_latched();
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
    sample_levels();
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
