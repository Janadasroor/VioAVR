#include "vioavr/core/viospice.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/uart0.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/ext_interrupt.hpp"

namespace vioavr::core {

VioSpice::VioSpice(const DeviceDescriptor& device)
    : bus_(device), cpu_(bus_)
{
    pin_map_ = std::make_unique<PinMap>();
    bus_.set_pin_map(pin_map_.get());

    // In a production version, this would be factory-driven from metadata
    auto eeprom = std::make_unique<Eeprom>("EEPROM", bus_.device());
    auto timer0 = std::make_unique<Timer8>("TIMER0", bus_.device().timer0);
    auto timer1 = std::make_unique<Timer16>("TIMER1", bus_.device().timer1);
    
    auto adc = std::make_unique<Adc>("ADC", bus_.device(), 0, 13);
    adc->bind_signal_bank(analog_signal_bank_); // BIND THE BANK
    
    auto uart0 = std::make_unique<Uart0>("UART0", bus_.device());
    auto exint = std::make_unique<ExtInterrupt>("EXINT", bus_.device(), 0);

    // Attach peripherals
    bus_.attach_peripheral(*eeprom.release());
    bus_.attach_peripheral(*timer0.release());
    bus_.attach_peripheral(*timer1.release());
    bus_.attach_peripheral(*adc.release());
    bus_.attach_peripheral(*uart0.release());
    bus_.attach_peripheral(*exint.release());

    // Dynamic GPIO Port initialization from DeviceDescriptor
    for (const auto& port_desc : bus_.device().ports) {
        if (!port_desc.name.empty()) {
            auto port = std::make_unique<GpioPort>(
                port_desc.name, 
                port_desc.pin_address, 
                port_desc.ddr_address, 
                port_desc.port_address
            );
            bus_.attach_peripheral(*port.release());
        }
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

void VioSpice::set_external_pin(u32 external_id, PinLevel level)
{
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

} // namespace vioavr::core
