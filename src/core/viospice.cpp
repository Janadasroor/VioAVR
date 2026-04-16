#include "vioavr/core/viospice.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/watchdog_timer.hpp"
#include "vioavr/core/spi.hpp"
#include "vioavr/core/twi.hpp"
#include "vioavr/core/can.hpp"
#include "vioavr/core/xmem.hpp"
#include <map>

namespace vioavr::core {

VioSpice::VioSpice(const DeviceDescriptor& device)
    : pin_mux_(device.port_count), bus_(device), cpu_(bus_)
{
    pin_map_ = std::make_unique<PinMap>();
    bus_.set_pin_map(pin_map_.get());

    // 1. Instantiate and Register Ports
    std::map<u16, GpioPort*> port_by_pin_addr;
    std::vector<GpioPort*> port_list;
    for (u8 i = 0; i < device.port_count; ++i) {
        const auto& desc = device.ports[i];
        if (desc.name.empty()) continue;

        auto port = std::make_unique<GpioPort>(
            desc.name, 
            desc.pin_address, 
            desc.ddr_address, 
            desc.port_address
        );
        port_by_pin_addr[desc.pin_address] = port.get();
        port_list.push_back(port.get());
        pin_mux_.register_port(port->port_address(), i);
        bus_.attach_peripheral(*port.release());
    }

    // 2. Instantiate and Wire 8-bit Timers
    for (u8 i = 0; i < device.timer8_count; ++i) {
        const auto& desc = device.timers8[i];
        auto timer = std::make_unique<Timer8>("TIMER8_" + std::to_string(i), desc);
        timer->set_bus(bus_);

        if (auto it = port_by_pin_addr.find(desc.ocra_pin_address); it != port_by_pin_addr.end()) {
            timer->connect_compare_output_a(*it->second, desc.ocra_pin_bit);
        }
        if (auto it = port_by_pin_addr.find(desc.ocrb_pin_address); it != port_by_pin_addr.end()) {
            timer->connect_compare_output_b(*it->second, desc.ocrb_pin_bit);
        }

        // Identify Timer2 (Async) for external clocking
        if (desc.assr_address != 0U) {
            timer2_ = timer.get();
        }

        bus_.attach_peripheral(*timer.release());
    }

    // 3. Instantiate and Wire 16-bit Timers
    for (u8 i = 0; i < device.timer16_count; ++i) {
        const auto& desc = device.timers16[i];
        auto timer = std::make_unique<Timer16>("TIMER16_" + std::to_string(i), desc);
        
        if (auto it = port_by_pin_addr.find(desc.ocra_pin_address); it != port_by_pin_addr.end()) {
            timer->connect_compare_output_a(*it->second, desc.ocra_pin_bit);
        }
        if (auto it = port_by_pin_addr.find(desc.ocrb_pin_address); it != port_by_pin_addr.end()) {
            timer->connect_compare_output_b(*it->second, desc.ocrb_pin_bit);
        }
        if (auto it = port_by_pin_addr.find(desc.ocrc_pin_address); it != port_by_pin_addr.end() && desc.ocrc_address != 0U) {
            timer->connect_compare_output_c(*it->second, desc.ocrc_pin_bit);
        }

        bus_.attach_peripheral(*timer.release());
    }

    // 4. Instantiate and Register Other Peripherals
    for (u8 i = 0; i < device.uart_count; ++i) {
        auto uart = std::make_unique<Uart>("UART" + std::to_string(i), device.uarts[i]);
        bus_.attach_peripheral(*uart.release());
    }

    for (u8 i = 0; i < device.adc_count; ++i) {
        auto adc = std::make_unique<Adc>("ADC" + std::to_string(i), device.adcs[i], pin_mux_, 0, 15);
        adc->bind_signal_bank(analog_signal_bank_);
        bus_.attach_peripheral(*adc.release());
    }

    for (u8 i = 0; i < device.ac_count; ++i) {
        auto ac = std::make_unique<AnalogComparator>("AC" + std::to_string(i), device.acs[i], pin_mux_, 0);
        ac->bind_signal_bank(analog_signal_bank_, 0, 1);
        bus_.attach_peripheral(*ac.release());
    }

    for (u8 i = 0; i < device.ext_interrupt_count; ++i) {
        auto exint = std::make_unique<ExtInterrupt>("EXINT" + std::to_string(i), device.ext_interrupts[i], pin_mux_, i);
        bus_.attach_peripheral(*exint.release());
    }

    for (u8 i = 0; i < device.pcint_count; ++i) {
        if (i < port_list.size()) {
            auto pci = std::make_unique<PinChangeInterrupt>(
                "PCINT" + std::to_string(i), device.pcints[i], *port_list[i], pcint_shared_state_, i == 0);
            bus_.attach_peripheral(*pci.release());
        }
    }

    for (u8 i = 0; i < device.eeprom_count; ++i) {
        auto eeprom = std::make_unique<Eeprom>("EEPROM" + std::to_string(i), device.eeproms[i]);
        bus_.attach_peripheral(*eeprom.release());
    }

    for (u8 i = 0; i < device.spi_count; ++i) {
        auto spi = std::make_unique<Spi>("SPI" + std::to_string(i), device.spis[i]);
        bus_.attach_peripheral(*spi.release());
    }

    for (u8 i = 0; i < device.twi_count; ++i) {
        auto twi = std::make_unique<Twi>("TWI" + std::to_string(i), device.twis[i]);
        bus_.attach_peripheral(*twi.release());
    }

    for (u8 i = 0; i < device.wdt_count; ++i) {
        auto wdt = std::make_unique<WatchdogTimer>("WDT" + std::to_string(i), device.wdts[i], cpu_);
        bus_.attach_peripheral(*wdt.release());
    }

    for (u8 i = 0; i < device.can_count; ++i) {
        auto can_bus = std::make_unique<CanBus>("CAN" + std::to_string(i), device.cans[i]);
        bus_.attach_peripheral(*can_bus.release());
    }

    if (device.xmcra_address != 0U || device.xmcrb_address != 0U) {
        auto xmem = std::make_unique<Xmem>(device);
        bus_.attach_peripheral(*xmem.release());
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
    const auto& device = bus_.device();
    for (u32 i = 0; i < device.timer8_count; ++i) {
        const auto& t8 = device.timers8[i];
        if (t8.assr_address == 0U) continue;

        for (const auto& port_desc : device.ports) {
            if (port_desc.name.empty()) continue;
            if (port_desc.pin_address == t8.tosc1_pin_address || port_desc.port_address == t8.tosc1_pin_address) {
                if (port_desc.name == port_name && t8.tosc1_pin_bit == bit_index) return true;
            }
            if (port_desc.pin_address == t8.tosc2_pin_address || port_desc.port_address == t8.tosc2_pin_address) {
                if (port_desc.name == port_name && t8.tosc2_pin_bit == bit_index) return true;
            }
        }
    }
    return false;
}

} // namespace vioavr::core
