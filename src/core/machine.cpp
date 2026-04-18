#include "vioavr/core/machine.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/analog_comparator.hpp"
#include "vioavr/core/ext_interrupt.hpp"
#include "vioavr/core/timer8.hpp"
#include "vioavr/core/timer16.hpp"
#include "vioavr/core/timer10.hpp"
#include "vioavr/core/uart.hpp"
#include "vioavr/core/spi.hpp"
#include "vioavr/core/twi.hpp"
#include "vioavr/core/eeprom.hpp"
#include "vioavr/core/watchdog_timer.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/psc.hpp"
#include "vioavr/core/dac.hpp"
#include "vioavr/core/nvm_ctrl.hpp"
#include "vioavr/core/cpu_int.hpp"

namespace vioavr::core {

Machine::Machine(const DeviceDescriptor& device)
    : device_(device),
      bus_(std::make_unique<MemoryBus>(device)),
      cpu_(std::make_unique<AvrCpu>(*bus_)),
      pin_mux_(std::make_unique<PinMux>(static_cast<u8>(device.port_count)))
{
    initialize_peripherals();
    wire_peripherals();
}

Machine::~Machine() = default;

std::unique_ptr<Machine> Machine::create_for_device(std::string_view name)
{
    const auto* device = DeviceCatalog::find(name);
    if (!device) return nullptr;
    return std::make_unique<Machine>(*device);
}

void Machine::reset(ResetCause cause) noexcept
{
    cpu_->reset(cause);
    for (auto& p : owned_peripherals_) {
        p->reset();
    }
}

GpioPort* Machine::get_port(std::string_view name) noexcept
{
    for (auto* port : ports_) {
        if (port->name() == name) return port;
    }
    return nullptr;
}

void Machine::initialize_peripherals()
{
    // 1. GPIO Ports
    for (u8 i = 0; i < device_.port_count; ++i) {
        const auto& desc = device_.ports[i];
        auto port = std::make_unique<GpioPort>(desc.name, desc.pin_address, desc.ddr_address, desc.port_address, *pin_mux_);
        ports_.push_back(port.get());
        pin_mux_->register_port(port->port_address(), i);
        bus_->attach_peripheral(*port);
        owned_peripherals_.push_back(std::move(port));
    }

    // 2. Timers
    for (u8 i = 0; i < device_.timer8_count; ++i) {
        auto timer = std::make_unique<Timer8>("TIMER" + std::to_string(device_.timers8[i].timer_index), device_.timers8[i]);
        timer->set_bus(*bus_);
        bus_->attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    for (u8 i = 0; i < device_.timer16_count; ++i) {
        auto timer = std::make_unique<Timer16>("TIMER" + std::to_string(device_.timers16[i].timer_index), device_.timers16[i]);
        timer->set_bus(*bus_);
        bus_->attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    // 3. UART
    for (u8 i = 0; i < device_.uart_count; ++i) {
        auto uart = std::make_unique<Uart>("UART" + std::to_string(device_.uarts[i].uart_index), device_.uarts[i]);
        bus_->attach_peripheral(*uart);
        owned_peripherals_.push_back(std::move(uart));
    }

    // 4. ADC
    for (u8 i = 0; i < device_.adc_count; ++i) {
        auto adc = std::make_unique<Adc>("ADC", device_.adcs[i], *pin_mux_, i);
        adc->set_bus(*bus_);
        bus_->attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    // 5. SPI
    for (u8 i = 0; i < device_.spi_count; ++i) {
        auto spi = std::make_unique<Spi>("SPI", device_.spis[i]);
        bus_->attach_peripheral(*spi);
        owned_peripherals_.push_back(std::move(spi));
    }

    // 6. PSC
    for (u8 i = 0; i < device_.psc_count; ++i) {
        auto psc = std::make_unique<Psc>("PSC", device_.pscs[i]);
        bus_->attach_peripheral(*psc);
        owned_peripherals_.push_back(std::move(psc));
    }

    // 7. DAC
    for (u8 i = 0; i < device_.dac_count; ++i) {
        auto dac = std::make_unique<Dac>("DAC", device_.dacs[i]);
        bus_->attach_peripheral(*dac);
        owned_peripherals_.push_back(std::move(dac));
    }

    // 6. TWI
    for (u8 i = 0; i < device_.twi_count; ++i) {
        auto twi = std::make_unique<Twi>("TWI", device_.twis[i]);
        bus_->attach_peripheral(*twi);
        owned_peripherals_.push_back(std::move(twi));
    }

    // 7. EEPROM
    for (u8 i = 0; i < device_.eeprom_count; ++i) {
        auto eeprom = std::make_unique<Eeprom>("EEPROM", device_.eeproms[i]);
        bus_->attach_peripheral(*eeprom);
        owned_peripherals_.push_back(std::move(eeprom));
    }

    // 8. Watchdog
    for (u8 i = 0; i < device_.wdt_count; ++i) {
        auto wdt = std::make_unique<WatchdogTimer>("WDT", device_.wdts[i], *cpu_);
        cpu_->set_watchdog_timer(wdt.get());
        bus_->attach_peripheral(*wdt);
        owned_peripherals_.push_back(std::move(wdt));
    }

    // 9. External Interrupts
    if (device_.ext_interrupt_count > 0) {
        auto ext_int = std::make_unique<ExtInterrupt>("EXINT", device_.ext_interrupts[0], *pin_mux_, 0);
        bus_->attach_peripheral(*ext_int);
        owned_peripherals_.push_back(std::move(ext_int));
    }

    // 10. PCINT
    for (u8 i = 0; i < device_.pcint_count; ++i) {
        // Need to find which port this PCINT belongs to. 
        // For now, we assume standard ATmega328P mapping if ports match count.
        GpioPort* target_port = nullptr;
        if (i < ports_.size()) target_port = ports_[i];
        
        if (target_port) {
            auto pcint = std::make_unique<PinChangeInterrupt>("PCINT" + std::to_string(i), 
                device_.pcints[i], *target_port, pcint_shared_state_, i == 0);
            bus_->attach_peripheral(*pcint);
            owned_peripherals_.push_back(std::move(pcint));
        }
    }

    // 11. Analog Comparator
    for (u8 i = 0; i < device_.ac_count; ++i) {
        auto ac = std::make_unique<AnalogComparator>("AC", device_.acs[i], *pin_mux_, i);
        bus_->attach_peripheral(*ac);
        owned_peripherals_.push_back(std::move(ac));
    }

    // 12. NVM Controller
    for (u8 i = 0; i < device_.nvm_ctrl_count; ++i) {
        auto nvm = std::make_unique<NvmCtrl>(device_.nvm_ctrls[i]);
        nvm->set_bus(*bus_);
        if (i == 0) bus_->set_nvm_ctrl(nvm.get());
        bus_->attach_peripheral(*nvm);
        owned_peripherals_.push_back(std::move(nvm));
    }

    // 13. CPUINT (Modern Interrupt Controller)
    for (u8 i = 0; i < device_.cpu_int_count; ++i) {
        auto cpu_int = std::make_unique<CpuInt>(device_.cpu_ints[i]);
        if (i == 0) bus_->set_cpu_int(cpu_int.get());
        bus_->attach_peripheral(*cpu_int);
        owned_peripherals_.push_back(std::move(cpu_int));
    }
}

void Machine::wire_peripherals()
{
    auto resolve_port = [&](u16 pin_address) -> GpioPort* {
        for (auto* p : ports_) {
            if (p->port_address() == pin_address || p->pin_address() == pin_address) return p;
        }
        return nullptr;
    };

    // Wire Timer outputs to GPIO pins
    for (auto& p : owned_peripherals_) {
        if (auto* t8 = dynamic_cast<Timer8*>(p.get())) {
            // We need to find the descriptor for this timer.
            // Simplified: loop over device descriptors and match by address.
            for (u8 i = 0; i < device_.timer8_count; ++i) {
                const auto& desc = device_.timers8[i];
                if (desc.tcnt_address == t8->mapped_ranges()[0].begin) { // Rough match
                    if (auto* port = resolve_port(desc.ocra_pin_address)) {
                        t8->connect_compare_output_a(*port, desc.ocra_pin_bit);
                    }
                    if (auto* port = resolve_port(desc.ocrb_pin_address)) {
                        t8->connect_compare_output_b(*port, desc.ocrb_pin_bit);
                    }
                    break;
                }
            }
        }
    }

    // Wire ADC triggers
    Adc* adc = nullptr;
    for (auto& p : owned_peripherals_) {
        if (auto* a = dynamic_cast<Adc*>(p.get())) {
            adc = a;
            break;
        }
    }

    if (adc) {
        for (auto& p : owned_peripherals_) {
            if (auto* ac = dynamic_cast<AnalogComparator*>(p.get())) {
                adc->connect_comparator_auto_trigger(*ac);
                
                // Also wire AC to PSC for fault protection
                for (auto& p2 : owned_peripherals_) {
                    if (auto* psc = dynamic_cast<Psc*>(p2.get())) {
                        ac->connect_psc_fault(*psc);
                    }
                }
            }
            if (auto* t8 = dynamic_cast<Timer8*>(p.get())) {
                adc->connect_timer_compare_auto_trigger(*t8);
                adc->connect_timer_overflow_auto_trigger(*t8);
            }
            if (auto* psc = dynamic_cast<Psc*>(p.get())) {
                psc->connect_adc_trigger(*adc);
            }
        }
    }
}

} // namespace vioavr::core
