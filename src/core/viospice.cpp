#include "vioavr/core/viospice.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/uart0.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"

namespace vioavr::core {

VioSpice::VioSpice(const DeviceDescriptor& device)
    : pin_mux_(8), bus_(device), cpu_(bus_)
{
    pin_map_ = std::make_unique<PinMap>();
    bus_.set_pin_map(pin_map_.get());

    auto eeprom = std::make_unique<Eeprom>("EEPROM", bus_.device());
    auto timer0 = std::make_unique<Timer8>("TIMER0", bus_.device().timer0);
    auto timer2 = std::make_unique<Timer8>("TIMER2", bus_.device().timer2);
    auto timer1 = std::make_unique<Timer16>("TIMER1", bus_.device().timer1);
    
    // Pass pin_mux_ to Adc and AnalogComparator
    auto adc = std::make_unique<Adc>("ADC", bus_.device().adc, pin_mux_, 0, 13);
    adc->bind_signal_bank(analog_signal_bank_);
    
    auto ac = std::make_unique<AnalogComparator>("AC", bus_.device().ac, pin_mux_, 0);
    ac->bind_signal_bank(analog_signal_bank_, 0, 1);
    
    auto uart0 = std::make_unique<Uart0>("UART0", bus_.device());
    auto exint = std::make_unique<ExtInterrupt>("EXINT", bus_.device().ext_interrupt, pin_mux_, 0);

    timer0->set_bus(bus_);
    timer2->set_bus(bus_);
    timer2_ = timer2.get();

    // Port/Pin wiring
    GpioPort* port_b = nullptr;
    GpioPort* port_c = nullptr;
    GpioPort* port_d = nullptr;
    for (const auto& port_desc : bus_.device().ports) {
        if (!port_desc.name.empty()) {
            auto port = std::make_unique<GpioPort>(
                port_desc.name, 
                port_desc.pin_address, 
                port_desc.ddr_address, 
                port_desc.port_address
            );
            if (port_desc.name == "PORTB") port_b = port.get();
            if (port_desc.name == "PORTC") port_c = port.get();
            if (port_desc.name == "PORTD") port_d = port.get();
            
            // Port index is assumed from sequence (A=0, B=1...)
            // In a better version we'd use a more robust mapping.
            static u8 next_port_idx = 0;
            pin_mux_.register_port(port->pin_address(), next_port_idx++);
            
            bus_.attach_peripheral(*port.release());
        }
    }

    auto bind_timer8_outputs = [](Timer8& timer, const Timer8Descriptor& desc,
                                   GpioPort* port_b, GpioPort* port_c, GpioPort* port_d) {
        auto resolve_port = [&](const u16 pin_address) -> GpioPort* {
            if (port_b != nullptr && pin_address == port_b->pin_address()) return port_b;
            if (port_c != nullptr && pin_address == port_c->pin_address()) return port_c;
            if (port_d != nullptr && pin_address == port_d->pin_address()) return port_d;
            return nullptr;
        };

        if (auto* port = resolve_port(desc.ocra_pin_address); port != nullptr) {
            timer.connect_compare_output_a(*port, desc.ocra_pin_bit);
        }
        if (auto* port = resolve_port(desc.ocrb_pin_address); port != nullptr) {
            timer.connect_compare_output_b(*port, desc.ocrb_pin_bit);
        }
    };

    bind_timer8_outputs(*timer0, bus_.device().timer0, port_b, port_c, port_d);
    bind_timer8_outputs(*timer2, bus_.device().timer2, port_b, port_c, port_d);

    bus_.attach_peripheral(*eeprom.release());
    bus_.attach_peripheral(*timer0.release());
    bus_.attach_peripheral(*timer2.release());
    bus_.attach_peripheral(*timer1.release());
    bus_.attach_peripheral(*adc.release());
    bus_.attach_peripheral(*ac.release());
    bus_.attach_peripheral(*uart0.release());
    bus_.attach_peripheral(*exint.release());

    if (port_b != nullptr && bus_.device().pin_change_interrupt_0.pcmsk_address != 0U) {
        auto pci0 = std::make_unique<PinChangeInterrupt>(
            "PCINT0", bus_.device().pin_change_interrupt_0, *port_b, pcint_shared_state_, true);
        bus_.attach_peripheral(*pci0.release());
    }
    if (port_c != nullptr && bus_.device().pin_change_interrupt_1.pcmsk_address != 0U) {
        auto pci1 = std::make_unique<PinChangeInterrupt>(
            "PCINT1", bus_.device().pin_change_interrupt_1, *port_c, pcint_shared_state_, false);
        bus_.attach_peripheral(*pci1.release());
    }
    if (port_d != nullptr && bus_.device().pin_change_interrupt_2.pcmsk_address != 0U) {
        auto pci2 = std::make_unique<PinChangeInterrupt>(
            "PCINT2", bus_.device().pin_change_interrupt_2, *port_d, pcint_shared_state_, false);
        bus_.attach_peripheral(*pci2.release());
    }

    set_quantum(1000);
}

void VioSpice::set_pin_map(std::unique_ptr<PinMap> pin_map)
{
    pin_map_ = std::move(pin_map);
    bus_.set_pin_map(pin_map_.get());
}

void VioSpice::add_pin_mapping(std::string_view port_name, u8 bit_index, u32 external_id, std::string_view label)
{
    if (pin_map_) {
        pin_map_->add_mapping(port_name, bit_index, external_id, label);
    }
}

void VioSpice::set_quantum(u64 cycles)
{
    quantum_ = cycles;
    sync_ = create_fixed_quantum_sync_engine(quantum_);
    cpu_.set_sync_engine(sync_.get());
}

bool VioSpice::load_hex(std::string_view path)
{
    try {
        const auto image = HexImageLoader::load_file(path, bus_.device());
        bus_.load_image(image);
        reset();
        return true;
    } catch (const std::exception& e) {
        Logger::error("VioSpice: Failed to load HEX: " + std::string(e.what()));
        return false;
    }
}

void VioSpice::reset()
{
    cpu_.reset();
}

void VioSpice::step_duration(double seconds)
{
    cpu_.run_duration(seconds);
}

void VioSpice::tick_timer2_async(const u64 ticks)
{
    if (timer2_ != nullptr) {
        timer2_->tick_async(ticks);
    }
}

void VioSpice::set_external_pin(u32 external_id, PinLevel level)
{
    if (pin_map_ != nullptr) {
        if (auto mapping = pin_map_->get_mapping_by_external(external_id); mapping &&
            is_timer2_async_input(mapping->port_name, mapping->bit_index)) {
            const bool next_high = (level == PinLevel::high);
            if (!timer2_async_input_high_ && next_high) {
                tick_timer2_async(1U);
            }
            timer2_async_input_high_ = next_high;
        }
    }

    bus_.propagate_external_pin_change(external_id, level);
}

void VioSpice::set_external_voltage(u8 channel, double normalized_voltage)
{
    analog_signal_bank_.set_voltage(channel, normalized_voltage);
}

std::vector<PinStateChange> VioSpice::consume_pin_changes()
{
    if (!sync_) return {};
    auto changes = sync_->consume_pin_changes();
    sync_->resume();
    return changes;
}

std::optional<u32> VioSpice::get_external_id(std::string_view port_name, u8 bit_index) const
{
    if (!pin_map_) return std::nullopt;
    return pin_map_->get_external_id(port_name, bit_index);
}

bool VioSpice::is_timer2_async_input(const std::string_view port_name, const u8 bit_index) const noexcept
{
    if (bus_.device().timer2.tosc1_pin_address == 0U) {
        return false;
    }

    for (const auto& port_desc : bus_.device().ports) {
        if (port_desc.name.empty()) {
            continue;
        }
        if (port_desc.pin_address == bus_.device().timer2.tosc1_pin_address) {
            return port_desc.name == port_name && bus_.device().timer2.tosc1_pin_bit == bit_index;
        }
    }

    return false;
}

} // namespace vioavr::core
