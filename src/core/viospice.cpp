#include "vioavr/core/viospice.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/uart8x.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/watchdog_timer.hpp"
#include "vioavr/core/spi.hpp"
#include "vioavr/core/twi.hpp"
#include "vioavr/core/can.hpp"
#include "vioavr/core/timer10.hpp"
#include "vioavr/core/xmem.hpp"
#include "vioavr/core/dac.hpp"
#include "vioavr/core/adc8x.hpp"
#include "vioavr/core/ac8x.hpp"
#include <map>

namespace vioavr::core {

VioSpice::VioSpice(const DeviceDescriptor& device)
    : pin_mux_(device.port_count), bus_(device), cpu_(bus_)
{
    pin_map_ = std::make_unique<PinMap>();
    bus_.set_pin_map(pin_map_.get());
    cpu_.set_trace_hook(&trace_mux_);

    // 1. Instantiate and Register Ports
    for (u8 i = 0; i < device.port_count; ++i) {
        const auto& desc = device.ports[i];
        if (desc.name.empty()) continue;

        auto port = std::make_unique<GpioPort>(
            desc.name, 
            desc.pin_address, 
            desc.ddr_address, 
            desc.port_address,
            pin_mux_
        );
        GpioPort* ptr = port.get();
        ports_.push_back(ptr);
        port_map_[std::string(desc.name)] = ptr;
        bus_.attach_peripheral(*port.release());
    }

    // 2. Timers
    for (u8 i = 0; i < device.timer8_count; ++i) {
        const auto& desc = device.timers8[i];
        auto timer = std::make_unique<Timer8>("TIMER8_" + std::to_string(i), desc, &pin_mux_);
        timer->set_bus(bus_);
        if (desc.assr_address != 0U) timer2_ = timer.get();
        bus_.attach_peripheral(*timer.release());
    }

    for (u8 i = 0; i < device.timer16_count; ++i) {
        auto timer = std::make_unique<Timer16>("TIMER16_" + std::to_string(i), device.timers16[i], &pin_mux_);
        bus_.attach_peripheral(*timer.release());
    }

    // 3. Analog
    for (u8 i = 0; i < device.adc_count; ++i) {
        auto adc = std::make_unique<Adc>("ADC" + std::to_string(i), device.adcs[i], pin_mux_, 0, 15);
        adc->bind_signal_bank(analog_signal_bank_);
        bus_.attach_peripheral(*adc.release());
    }

    for (u8 i = 0; i < device.adc8x_count; ++i) {
        auto adc = std::make_unique<Adc8x>(device.adcs8x[i]);
        adc->set_memory_bus(&bus_);
        adc->set_analog_signal_bank(&analog_signal_bank_);
        bus_.attach_peripheral(*adc.release());
    }

    for (u8 i = 0; i < device.dac_count; ++i) {
        auto dac = std::make_unique<Dac>("DAC" + std::to_string(i), device.dacs[i]);
        dacs_.push_back(dac.get());
        bus_.attach_peripheral(*dac.release());
    }

    // 4. Communication (UART)
    for (u8 i = 0; i < device.uart_count; ++i) {
        auto uart = std::make_unique<Uart>("UART" + std::to_string(i), device.uarts[i], pin_mux_);
        bus_.attach_peripheral(*uart.release());
    }

    for (u8 i = 0; i < device.uart8x_count; ++i) {
        auto uart = std::make_unique<Uart8x>(device.uarts8x[i], pin_mux_);
        uart->set_memory_bus(&bus_);
        bus_.attach_peripheral(*uart.release());
    }

    for (u8 i = 0; i < device.eeprom_count; ++i) {
        auto eeprom = std::make_unique<Eeprom>("EEPROM" + std::to_string(i), device.eeproms[i]);
        bus_.attach_peripheral(*eeprom.release());
    }

    set_quantum(1000);
}

bool VioSpice::load_hex(std::string_view path) {
    try {
        HexImage image = HexImageLoader::load_file(path, bus_.device());
        if (image.flash_words.empty()) return false;
        bus_.load_image(image);
        return true;
    } catch (...) {
        return false;
    }
}

void VioSpice::set_pin_map(std::unique_ptr<PinMap> pin_map) {
    pin_map_ = std::move(pin_map);
    bus_.set_pin_map(pin_map_.get());
}

void VioSpice::add_pin_mapping(std::string_view port_name, u8 bit_index, u32 external_id, std::string_view label) {
    if (pin_map_) {
        u16 addr = 0;
        auto it = port_map_.find(std::string(port_name));
        if (it != port_map_.end()) addr = it->second->pin_address();
        pin_map_->add_mapping(port_name, addr, bit_index, external_id, label);
    }
}

void VioSpice::add_trace_hook(ITraceHook* hook) {
    trace_mux_.add_hook(hook);
}

void VioSpice::set_external_pin(u32 external_id, PinLevel level) {
    if (!pin_map_) return;
    auto mapping = pin_map_->get_mapping_by_external(external_id);
    if (mapping) {
        auto it = port_map_.find(mapping->port_name);
        if (it != port_map_.end()) {
            (void)it->second->on_external_pin_change(mapping->bit_index, level);
            
            // Notify Timer2 if it matches TOSC1
            if (timer2_) {
                 timer2_->on_pin_change(it->second->pin_address(), mapping->bit_index, level == PinLevel::high);
            }
        }
    }
}

void VioSpice::set_external_voltage(u8 channel, double normalized_voltage) {
    analog_signal_bank_.set_voltage(channel, normalized_voltage);
}

std::vector<PinStateChange> VioSpice::consume_pin_changes() {
    std::vector<PinStateChange> changes;
    for (auto* port : ports_) {
        PinStateChange change;
        while (port->consume_pin_change(change)) {
            changes.push_back(change);
        }
    }
    return changes;
}

std::optional<u32> VioSpice::get_external_id(std::string_view port_name, u8 bit_index) const {
    if (pin_map_) return pin_map_->get_external_id(port_name, bit_index);
    return std::nullopt;
}

std::vector<double> VioSpice::get_analog_outputs() {
    std::vector<double> results;
    results.reserve(dacs_.size());
    for (auto* dac : dacs_) {
        results.push_back(dac->voltage());
    }
    return results;
}

void VioSpice::step_duration(double seconds) {
    cpu_.run_duration(seconds);
}

void VioSpice::tick_timer2_async(u64 ticks) {
    if (timer2_) timer2_->tick_async(ticks);
}

void VioSpice::reset() {
    cpu_.reset();
}

void VioSpice::set_frequency(double hz) {
    frequency_ = hz;
}

void VioSpice::set_quantum(u64 cycles) {
    quantum_ = cycles;
}

} // namespace vioavr::core
 
