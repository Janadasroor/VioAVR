#include "vioavr/core/machine.hpp"
#include "vioavr/core/logger.hpp"
#include "vioavr/core/device_catalog.hpp"
#include "vioavr/core/adc.hpp"
#include "vioavr/core/amplifier.hpp"
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
#include "vioavr/core/adc.hpp"
#include "vioavr/core/adc8x.hpp"
#include "vioavr/core/adc10b.hpp"
#include "vioavr/core/ac8x.hpp"
#include "vioavr/core/can.hpp"
#include "vioavr/core/psc.hpp"
#include "vioavr/core/dac.hpp"
#include "vioavr/core/dac8x.hpp"
#include "vioavr/core/nvm_ctrl.hpp"
#include "vioavr/core/cpu_int.hpp"
#include "vioavr/core/tca.hpp"
#include "vioavr/core/tcb.hpp"
#include "vioavr/core/tc.hpp"
#include "vioavr/core/awex.hpp"
#include "vioavr/core/rtc.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/ccl.hpp"
#include "vioavr/core/uart8x.hpp"
#include "vioavr/core/spi8x.hpp"
#include "vioavr/core/twi8x.hpp"
#include "vioavr/core/usi.hpp"
#include "vioavr/core/wdt8x.hpp"
#include "vioavr/core/crc8x.hpp"
#include "vioavr/core/dma.hpp"
#include "vioavr/core/rstctrl.hpp"
#include "vioavr/core/slpctrl.hpp"
#include "vioavr/core/syscfg.hpp"
#include "vioavr/core/vref.hpp"
#include "vioavr/core/bodctrl.hpp"
#include "vioavr/core/clkctrl.hpp"
#include "vioavr/core/zcd.hpp"
#include "vioavr/core/lin.hpp"
#include "vioavr/core/opamp.hpp"
#include "vioavr/core/ptc.hpp"
#include "vioavr/core/cfd.hpp"
#include "vioavr/core/tcd.hpp"
#include "vioavr/core/tce.hpp"
#include "vioavr/core/xmega_dac.hpp"
#include "vioavr/core/ebi.hpp"
#include "vioavr/core/ircom.hpp"
#include "vioavr/core/xmem.hpp"
#include "vioavr/core/adcea.hpp"
#include "vioavr/core/xmegadc.hpp"
#include "vioavr/core/xmegaac.hpp"
#include "vioavr/core/lcd_controller.hpp"
#include "vioavr/core/usb.hpp"
#include "vioavr/core/usb8x.hpp"
#include "vioavr/core/mvio.hpp"
#include "vioavr/core/eusart.hpp"
#include "vioavr/core/pll.hpp"

namespace vioavr::core {

// Compute the number of PinMux port slots needed: must cover the highest
// port name letter (e.g. PORTJ → idx 9) since port_count may be smaller.
static u8 compute_pin_mux_port_count(const DeviceDescriptor& device) noexcept
{
    u8 max_idx = 0;
    for (u8 i = 0; i < device.port_count; ++i) {
        const auto& p = device.ports[i];
        if (p.name.size() >= 5) {
            u8 idx = static_cast<u8>(p.name[4] - 'A');
            if (idx > max_idx) max_idx = idx;
        }
    }
    u8 needed = static_cast<u8>(max_idx + 1);
    return (needed > device.port_count) ? needed : device.port_count;
}

Machine::Machine(const DeviceDescriptor& device)
    : device_(device),
      bus_(std::make_unique<MemoryBus>(device)),
      cpu_(std::make_unique<AvrCpu>(*bus_)),
      pin_mux_(std::make_unique<PinMux>(compute_pin_mux_port_count(device)))
{
    bus_->set_pin_mux(pin_mux_.get());
    pin_mux_->set_memory_bus(bus_.get());
    cpu_->set_trace_hook(&trace_mux_);
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
    pin_mux_->reset();
    for (auto& p : owned_peripherals_) {
        p->reset();
    }
}

#ifndef _WIN32
void Machine::enable_gdb(uint16_t port)
{
    if (!gdb_stub_) {
        gdb_stub_ = std::make_unique<GdbStub>(*cpu_, *bus_);
        trace_mux_.add_hook(gdb_stub_.get());
    }
    gdb_stub_->start(port);
}

void Machine::disable_gdb()
{
    if (gdb_stub_) {
        gdb_stub_->stop();
        trace_mux_.remove_hook(gdb_stub_.get());
        gdb_stub_.reset();
    }
}
#endif

void Machine::enable_trace_buffer(size_t capacity)
{
    if (trace_buffer_) {
        disable_trace_buffer();
    }
    trace_buffer_ = std::make_unique<TraceBuffer>(capacity);
    trace_buffer_->set_cpu(cpu_.get());
    trace_mux_.add_hook(trace_buffer_.get());
}

void Machine::disable_trace_buffer()
{
    if (trace_buffer_) {
        trace_mux_.remove_hook(trace_buffer_.get());
        trace_buffer_.reset();
    }
}

std::vector<CpuSnapshot> Machine::trace_history() const
{
    if (trace_buffer_) {
        return trace_buffer_->snapshots();
    }
    return {};
}

EventSystem* Machine::event_system() noexcept
{
    for (auto& p : owned_peripherals_) {
        if (auto* ev = dynamic_cast<EventSystem*>(p.get())) return ev;
    }
    return nullptr;
}

void Machine::add_peripheral(std::unique_ptr<IoPeripheral> peripheral) noexcept
{
    owned_peripherals_.push_back(std::move(peripheral));
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
        auto port = std::make_unique<GpioPort>(desc, *pin_mux_);
        ports_.push_back(port.get());
        bus_->attach_peripheral(*port);
        owned_peripherals_.push_back(std::move(port));
    }

    // 1.5 PortMux (AVR8X) — also created when TCA/TCB/UART8x/SPI8x/TWI8x exist
    // (ATtiny chips have fixed routing, no TCAROUTEA register)
    if (device_.portmux.tcaroutea_address != 0 || device_.tca_count > 0 || device_.tcb_count > 0 ||
        device_.uart8x_count > 0 || device_.spi8x_count > 0 || device_.twi8x_count > 0) {
        auto pm = std::make_unique<PortMux>(device_.portmux);
        port_mux_ = pm.get();
        port_mux_->set_pin_mux(pin_mux_.get());
        // Register ports for signal routing helpers
        for (u8 i = 0; i < ports_.size(); ++i) {
            port_mux_->register_port(ports_[i]->port_address(), i);
        }
        bus_->attach_peripheral(*pm);
        owned_peripherals_.push_back(std::move(pm));
    }

    // 1.6 System Control Peripherals (AVR8X)
    if (device_.rstctrl.rstfr_address != 0U) {
        auto p = std::make_unique<RstCtrl>(device_.rstctrl);
        bus_->attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    if (device_.slpctrl.ctrla_address != 0U) {
        auto p = std::make_unique<SlpCtrl>(device_.slpctrl);
        bus_->attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    if (device_.syscfg.reves_address != 0U) {
        auto p = std::make_unique<SysCfg>(device_.syscfg);
        bus_->attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    if (device_.vref.ctrla_address != 0U) {
        auto p = std::make_unique<VRef>(device_.vref);
        bus_->attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    if (device_.bod.ctrla_address != 0U) {
        auto p = std::make_unique<BodCtrl>(device_.bod);
        bus_->attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    if (device_.clkctrl.ctrla_address != 0U) {
        // Default base = 20 MHz. The prescaler-on reset default divides by 6 → 3.333 MHz.
        auto p = std::make_unique<ClkCtrl>(device_.clkctrl, 20'000'000U);
        bus_->attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }

    // 2b. MVIO (DB/DD)
    if (device_.mvio.intctrl_address != 0U) {
        auto p = std::make_unique<Mvio>(device_.mvio);
        p->set_memory_bus(bus_.get());
        bus_->attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }

    // Timers
    Dma* dma = nullptr;
    if (device_.dma_count > 0) {
        for (u8 i = 0; i < device_.dma_count; ++i) {
            auto d = std::make_unique<Dma>(device_.dmas[i]);
            dma = d.get();
            d->set_memory_bus(bus_.get());
            bus_->attach_peripheral(*d);
            owned_peripherals_.push_back(std::move(d));
        }
    }

    // Event System
    EventSystem* evsys = nullptr;
    if (device_.evsys.strobe_address != 0) {
        auto e = std::make_unique<EventSystem>(device_.evsys);
        evsys = e.get();
        e->set_memory_bus(bus_.get());
        bus_->attach_peripheral(*e);
        owned_peripherals_.push_back(std::move(e));
    }

    if (dma && evsys) {
        dma->set_event_system(evsys);
    }

    // Wire EVOUT routing: EVSYS user callbacks drive PortMux pins
    if (evsys && port_mux_ && device_.evsys.evout_user_count > 0) {
        u8 evout_start = device_.evsys.evout_user_start;
        for (u8 i = 0; i < device_.evsys.evout_user_count; ++i) {
            evsys->register_user_callback(evout_start + i, [pm = port_mux_, i](bool level) {
                pm->drive_evout(i, level);
            });
        }
    }

    for (u8 i = 0; i < device_.timer8_count; ++i) {
        auto timer = std::make_unique<Timer8>("TIMER" + std::to_string(device_.timers8[i].timer_index), device_.timers8[i], pin_mux_.get());
        timer->set_bus(*bus_);
        bus_->attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    for (u8 i = 0; i < device_.timer16_count; ++i) {
        auto timer = std::make_unique<Timer16>("TIMER" + std::to_string(device_.timers16[i].timer_index), device_.timers16[i], pin_mux_.get());
        timer->set_bus(*bus_);
        bus_->attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    // Timer10 (HV devices)
    for (u8 i = 0; i < device_.timer10_count; ++i) {
        auto timer = std::make_unique<Timer10>("TIMER10_" + std::to_string(i), device_.timers10[i]);
        timer->set_bus(*bus_);
        bus_->attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    // Modern TCA
    for (u8 i = 0; i < device_.tca_count; ++i) {
        auto timer = std::make_unique<Tca>("TCA" + std::to_string(i), device_.timers_tca[i]);
        timer->set_memory_bus(bus_.get());
        timer->set_event_system(evsys);
        if (port_mux_) {
            timer->set_port_mux(port_mux_);
            port_mux_->add_observer(timer.get());
        }
        bus_->attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    // Modern TCB
    for (u8 i = 0; i < device_.tcb_count; ++i) {
        auto timer = std::make_unique<Tcb>("TCB" + std::to_string(i), device_.timers_tcb[i]);
        timer->set_memory_bus(bus_.get());
        timer->set_event_system(evsys);
        if (port_mux_) {
            timer->set_port_mux(port_mux_);
            port_mux_->add_observer(timer.get());
        }
        bus_->attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    // Modern RTC
    for (u8 i = 0; i < device_.rtc_count; ++i) {
        auto timer = std::make_unique<Rtc>(device_.timers_rtc[i]);
        timer->set_memory_bus(bus_.get());
        timer->set_event_system(evsys);
        bus_->attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    // 3. UART
    for (u8 i = 0; i < device_.uart_count; ++i) {
        auto uart = std::make_unique<Uart>("UART" + std::to_string(device_.uarts[i].uart_index), device_.uarts[i], *pin_mux_);
        bus_->attach_peripheral(*uart);
        owned_peripherals_.push_back(std::move(uart));
    }

    // Modern UART (AVR8X)
    for (u8 i = 0; i < device_.uart8x_count; ++i) {
        auto uart = std::make_unique<Uart8x>(device_.uarts8x[i], *pin_mux_);
        uart->set_memory_bus(bus_.get());
        uart->set_event_system(evsys);
        uart->set_port_mux(port_mux_);
        if (port_mux_) port_mux_->add_observer(uart.get());
        bus_->attach_peripheral(*uart);
        owned_peripherals_.push_back(std::move(uart));
    }

    // Modern SPI (AVR8X)
    for (u8 i = 0; i < device_.spi8x_count; ++i) {
        auto spi = std::make_unique<Spi8x>(device_.spis8x[i]);
        spi->set_memory_bus(bus_.get());
        spi->set_event_system(evsys);
        spi->set_port_mux(port_mux_);
        bus_->attach_peripheral(*spi);
        owned_peripherals_.push_back(std::move(spi));
    }

    // Modern TWI (AVR8X)
    for (u8 i = 0; i < device_.twi8x_count; ++i) {
        auto twi = std::make_unique<Twi8x>(device_.twis8x[i]);
        twi->set_memory_bus(bus_.get());
        twi->set_event_system(evsys);
        twi->set_port_mux(port_mux_);
        bus_->attach_peripheral(*twi);
        owned_peripherals_.push_back(std::move(twi));
    }

    // 4. ADC
    for (u8 i = 0; i < device_.adc_count; ++i) {
        auto adc = std::make_unique<Adc>("ADC", device_.adcs[i], *pin_mux_, i);
        adc->set_bus(*bus_);
        bus_->attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    // Modern ADC (AVR8X)
    for (u8 i = 0; i < device_.adc8x_count; ++i) {
        auto adc = std::make_unique<Adc8x>(device_.adcs8x[i]);
        adc->set_memory_bus(bus_.get());
        adc->set_event_system(evsys);
        adc->set_analog_signal_bank(&analog_signal_bank_);
        bus_->attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    // DU 10-bit ADC
    for (u8 i = 0; i < device_.adc10b_count; ++i) {
        auto adc = std::make_unique<Adc10b>(device_.adcs10b[i]);
        adc->set_memory_bus(bus_.get());
        adc->set_analog_signal_bank(&analog_signal_bank_);
        bus_->attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    // Modern AC (AVR8X)
    for (u8 i = 0; i < device_.ac8x_count; ++i) {
        auto ac = std::make_unique<Ac8x>("AC" + std::to_string(i), device_.acs8x[i]);
        ac->set_memory_bus(bus_.get());
        ac->set_event_system(evsys);
        ac->set_analog_signal_bank(&analog_signal_bank_);
        ac->set_vdd(device_.operating_voltage_v);
        bus_->attach_peripheral(*ac);
        owned_peripherals_.push_back(std::move(ac));
    }

    // 5. SPI
    for (u8 i = 0; i < device_.spi_count; ++i) {
        auto spi = std::make_unique<Spi>("SPI", device_.spis[i]);
        bus_->attach_peripheral(*spi);
        owned_peripherals_.push_back(std::move(spi));
    }

    // USI (Universal Serial Interface)
    for (u8 i = 0; i < device_.usi_count; ++i) {
        auto usi = std::make_unique<Usi>("USI", device_.usis[i]);
        bus_->attach_peripheral(*usi);
        owned_peripherals_.push_back(std::move(usi));
    }

    // 6. PSC
    for (u8 i = 0; i < device_.psc_count; ++i) {
        auto psc = std::make_unique<Psc>("PSC", device_.pscs[i], pin_mux_.get());
        bus_->attach_peripheral(*psc);
        owned_peripherals_.push_back(std::move(psc));
    }

    // 6.5 EUSART
    for (u8 i = 0; i < device_.eusart_count; ++i) {
        auto eusart = std::make_unique<Eusart>("EUSART", device_.eusarts[i]);
        bus_->attach_peripheral(*eusart);
        owned_peripherals_.push_back(std::move(eusart));
    }

    // 7. DAC
    for (u8 i = 0; i < device_.dac_count; ++i) {
        auto dac = std::make_unique<Dac>("DAC", device_.dacs[i]);
        dac->set_memory_bus(bus_.get());
        bus_->attach_peripheral(*dac);
        owned_peripherals_.push_back(std::move(dac));
    }

    // 7b. DAC8x (AVR-Dx style)
    for (u8 i = 0; i < device_.dac8x_count; ++i) {
        auto dac = std::make_unique<Dac8x>(device_.dacs8x[i], i);
        dac->set_analog_signal_bank(&analog_signal_bank_);
        dac->set_vref(device_.operating_voltage_v);
        bus_->attach_peripheral(*dac);
        owned_peripherals_.push_back(std::move(dac));
    }

    // Amplifiers
    for (u8 i = 0; i < device_.amplifier_count; ++i) {
        auto amp = std::make_unique<At90Amplifier>("AMP" + std::to_string(i), device_.amplifiers[i], *pin_mux_);
        bus_->attach_peripheral(*amp);
        owned_peripherals_.push_back(std::move(amp));
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
        eeprom->set_memory_bus(bus_.get());
        if (i == 0) bus_->set_eeprom(eeprom.get());
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

    // Modern Watchdog (AVR8X)
    for (u8 i = 0; i < device_.wdt8x_count; ++i) {
        auto wdt = std::make_unique<Wdt8x>(device_.wdts8x[i], *cpu_);
        if (i == 0) cpu_->set_wdt8x(wdt.get());
        bus_->attach_peripheral(*wdt);
        owned_peripherals_.push_back(std::move(wdt));
    }

    // 8.5 PLL Controller
    u16 pll_addr = 0;
    if (device_.psc_count > 0) pll_addr = device_.pscs[0].pllcsr_address;
    else if (device_.usb_count > 0) pll_addr = device_.usbs[0].pllcsr_address;
    
    if (pll_addr != 0) {
        auto pll = std::make_unique<PllController>(pll_addr);
        bus_->attach_peripheral(*pll);
        owned_peripherals_.push_back(std::move(pll));
    }

    // Modern CRC (AVR8X)
    for (u8 i = 0; i < device_.crc8x_count; ++i) {
        auto crc = std::make_unique<Crc8x>(device_.crcs8x[i], bus_->flash_words());
        bus_->attach_peripheral(*crc);
        owned_peripherals_.push_back(std::move(crc));
    }

    // 9. LCD Controller
    for (u8 i = 0; i < device_.lcd_count; ++i) {
        Logger::debug("Machine init LCD " + std::to_string(i) + " SEG0 port: 0x" + Logger::hex(device_.lcds[i].segment_pins[0].port_address));
        auto lcd = std::make_unique<LcdController>("LCD", device_.lcds[i], *pin_mux_);
        bus_->attach_peripheral(*lcd);
        owned_peripherals_.push_back(std::move(lcd));
    }

    // 10. External Interrupts
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
    for (int i = 0; i < (int)device_.ac_count; ++i) {
        auto ac = std::make_unique<AnalogComparator>("AC" + std::to_string(i), device_.acs[i], *pin_mux_, (u8)i);
        // Wire to Timer1 (index 1) if present
        for (auto& p : owned_peripherals_) {
            if (auto* t16 = dynamic_cast<Timer16*>(p.get())) {
                if (t16->name() == "TIMER1") t16->connect_analog_comparator(*ac);
            }
        }
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

    // 14. ZCD
    for (u8 i = 0; i < device_.zcd_count; ++i) {
        auto zcd = std::make_unique<Zcd>(device_.zcds[i], i);
        zcd->set_analog_signal_bank(&analog_signal_bank_);
        zcd->set_input_channel(static_cast<u8>(100 + i));
        zcd->set_vdd(device_.operating_voltage_v);
        if (i == 0) bus_->set_zcd(zcd.get());
        bus_->attach_peripheral(*zcd);
        owned_peripherals_.push_back(std::move(zcd));
    }

    // 15. LIN
    for (u8 i = 0; i < device_.lin_count; ++i) {
        auto lin = std::make_unique<LinUART>(device_.lins[i]);
        lin->set_memory_bus(bus_.get());
        lin->set_pin_mux(pin_mux_.get());
        bus_->attach_peripheral(*lin);
        owned_peripherals_.push_back(std::move(lin));
    }

    // 16. OPAMP
    for (u8 i = 0; i < device_.opamp_count; ++i) {
        auto op = std::make_unique<Opamp>(device_.opamps[i], i);
        op->set_analog_signal_bank(&analog_signal_bank_);
        op->set_vdd(device_.operating_voltage_v);
        bus_->attach_peripheral(*op);
        owned_peripherals_.push_back(std::move(op));
    }

    // 17a. CFD (Clock Failure Detection)
    for (u8 i = 0; i < device_.cfd_count; ++i) {
        auto cfd = std::make_unique<Cfd>(device_.cfds[i]);
        cfd->set_memory_bus(bus_.get());
        bus_->attach_peripheral(*cfd);
        owned_peripherals_.push_back(std::move(cfd));
    }

    // 17. PTC (Peripheral Touch Controller)
    for (u8 i = 0; i < device_.ptc_count; ++i) {
        auto ptc = std::make_unique<Ptc>(device_.ptcs[i]);
        bus_->attach_peripheral(*ptc);
        owned_peripherals_.push_back(std::move(ptc));
    }

    // TCD (Timer/Counter Type D)
    for (u8 i = 0; i < device_.tcd_count; ++i) {
        auto tcd = std::make_unique<Tcd>(device_.timers_tcd[i]);
        bus_->attach_peripheral(*tcd);
        owned_peripherals_.push_back(std::move(tcd));
    }

    // TCE (Timer/Counter Type E — LA family)
    for (u8 i = 0; i < device_.tce_count; ++i) {
        auto tce = std::make_unique<Tce>("TCE" + std::to_string(i), device_.timers_tce[i]);
        tce->set_event_system(evsys);
        if (port_mux_) {
            tce->set_port_mux(port_mux_);
            port_mux_->add_observer(tce.get());
        }
        bus_->attach_peripheral(*tce);
        owned_peripherals_.push_back(std::move(tce));
    }

    // TC (XMEGA 16-bit Timer/Counter) — collect raw pointers for AWEX pairing
    std::vector<Tc*> tc_ptrs;
    tc_ptrs.reserve(device_.tc_count);
    for (u8 i = 0; i < device_.tc_count; ++i) {
        auto tc = std::make_unique<Tc>("TC" + std::to_string(i), device_.timers_tc[i]);
        tc->set_event_system(evsys);
        if (port_mux_) {
            tc->set_port_mux(port_mux_);
            port_mux_->add_observer(tc.get());
        } else {
            tc->set_pin_mux(pin_mux_.get());
        }
        {
            char port_letter = static_cast<char>(device_.timers_tc[i].port_letter);
            if (!port_letter) {
                u16 tc_base = device_.timers_tc[i].ctrla_address;
                if (tc_base >= 0x0800 && tc_base < 0x0900) port_letter = 'C';
                else if (tc_base >= 0x0900 && tc_base < 0x0A00) port_letter = 'D';
                else if (tc_base >= 0x0A00 && tc_base < 0x0B00) port_letter = 'E';
                else if (tc_base >= 0x0B00 && tc_base < 0x0C00) port_letter = 'F';
            }
            if (port_letter) {
                tc->set_wg_mode_in_ctrld(true);
                for (u8 p = 0; p < device_.port_count; ++p) {
                    auto& pn = device_.ports[p].name;
                    if (pn.size() > 4 && pn[4] == port_letter) {
                        tc->set_port_index(static_cast<u8>(pn[4] - 'A'));
                        break;
                    }
                }
            }
        }
        tc_ptrs.push_back(tc.get());
        bus_->attach_peripheral(*tc);
        owned_peripherals_.push_back(std::move(tc));
    }

    // AWEX (XMEGA Advanced Waveform Extension) — pair with companion TC
    for (u8 i = 0; i < device_.awex_count; ++i) {
        auto awex = std::make_unique<Awex>("AWEX" + std::to_string(i), device_.awexs[i]);
        awex->set_memory_bus(bus_.get());
        if (port_mux_) {
            awex->set_port_mux(port_mux_);
        } else {
            awex->set_pin_mux(pin_mux_.get());
        }
        // Match AWEX to companion TC by index (same order: AWEXC→TCC0, AWEXE→TCE0, etc.)
        // Tc::set_awex() also propagates port_index_ to the AWEX.
        if (i < tc_ptrs.size()) {
            tc_ptrs[i]->set_awex(awex.get());
        }
        bus_->attach_peripheral(*awex);
        owned_peripherals_.push_back(std::move(awex));
    }

    // AdcEa (EA 12-bit diff ADC)
    for (u8 i = 0; i < device_.adcea_count; ++i) {
        auto adc = std::make_unique<AdcEa>(device_.adceas[i]);
        adc->set_memory_bus(bus_.get());
        adc->set_analog_signal_bank(&analog_signal_bank_);
        bus_->attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    // XMEGA ADC
    for (u8 i = 0; i < device_.adc_xmega_count; ++i) {
        auto adc = std::make_unique<XmegaAdc>(device_.adcs_xmega[i]);
        adc->set_memory_bus(bus_.get());
        adc->set_analog_signal_bank(&analog_signal_bank_);
        bus_->attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    // XMEGA AC
    for (u8 i = 0; i < device_.ac_xmega_count; ++i) {
        auto ac = std::make_unique<XmegaAc>(device_.acs_xmega[i]);
        ac->set_memory_bus(bus_.get());
        ac->set_analog_signal_bank(&analog_signal_bank_);
        bus_->attach_peripheral(*ac);
        owned_peripherals_.push_back(std::move(ac));
    }

    // 18. CCL (Moved to end to ensure all other peripherals are on the bus)
    if (device_.ccl.ctrla_address != 0) {
        auto c = std::make_unique<Ccl>(device_.ccl);
        c->set_memory_bus(bus_.get());
        c->set_event_system(evsys);
        c->set_pin_mux(pin_mux_.get());
        bus_->attach_peripheral(*c);
        owned_peripherals_.push_back(std::move(c));
    }

    // 17b. CAN
    for (u8 i = 0; i < device_.can_count; ++i) {
        auto can = std::make_unique<CanBus>("CAN" + std::to_string(i), device_.cans[i]);
        bus_->attach_peripheral(*can);
        owned_peripherals_.push_back(std::move(can));
    }

    // 18. USB
    for (u8 i = 0; i < device_.usb_count; ++i) {
        auto usb = std::make_unique<Usb>("USB", device_.usbs[i]);
        bus_->attach_peripheral(*usb);
        owned_peripherals_.push_back(std::move(usb));
    }

    // 18b. USB8x (DU / XMEGA)
    for (u8 i = 0; i < device_.usb8x_count; ++i) {
        auto usb = std::make_unique<Usb8x>(device_.usbs8x[i]);
        bus_->attach_peripheral(*usb);
        owned_peripherals_.push_back(std::move(usb));
    }

    // XMEGA DAC
    for (u8 i = 0; i < device_.xmega_dac_count; ++i) {
        auto dac = std::make_unique<XmegaDac>("XDAC" + std::to_string(i), device_.xmega_dacs[i]);
        dac->set_memory_bus(bus_.get());
        dac->set_analog_signal_bank(&analog_signal_bank_);
        bus_->attach_peripheral(*dac);
        owned_peripherals_.push_back(std::move(dac));
    }

    // XMEM (External Memory)
    if (device_.xmem.xmcra_address != 0) {
        auto xmem = std::make_unique<Xmem>(bus_->device(), cpu_->cpu_control());
        bus_->attach_peripheral(*xmem);
        bus_->set_xmem(xmem.get());
        owned_peripherals_.push_back(std::move(xmem));
    }

    // EBI (External Bus Interface)
    for (u8 i = 0; i < device_.ebi_count; ++i) {
        auto ebi = std::make_unique<Ebi>("EBI", device_.ebis[i]);
        ebi->set_memory_bus(bus_.get());
        bus_->attach_peripheral(*ebi);
        bus_->set_ebi(ebi.get());
        owned_peripherals_.push_back(std::move(ebi));
    }

    // IRCOM (IR Communication Module)
    for (u8 i = 0; i < device_.ircom_count; ++i) {
        auto ircom = std::make_unique<Ircom>("IRCOM", device_.ircoms[i]);
        ircom->set_memory_bus(bus_.get());
        ircom->set_pin_mux(pin_mux_.get());
        if (device_.ircoms[i].pin_address != 0) {
            for (auto* port : ports_) {
                if (port && port->port_address() == device_.ircoms[i].pin_address) {
                    u8 port_letter = port->name()[4] - 'A';
                    ircom->set_output_pin(port_letter, device_.ircoms[i].pin_bit_index);
                    break;
                }
            }
        }
        bus_->attach_peripheral(*ircom);
        owned_peripherals_.push_back(std::move(ircom));
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
            for (u8 i = 0; i < device_.timer8_count; ++i) {
                const auto& desc = device_.timers8[i];
                if (t8->name() == "TIMER" + std::to_string(desc.timer_index)) {
                    if (auto* port = resolve_port(desc.ocra_pin_address)) {
                        t8->connect_compare_output_a(*port, desc.ocra_pin_bit);
                    }
                    if (auto* port = resolve_port(desc.ocrb_pin_address)) {
                        t8->connect_compare_output_b(*port, desc.ocrb_pin_bit);
                    }
                    break;
                }
            }
        } else if (auto* t16 = dynamic_cast<Timer16*>(p.get())) {
            for (u8 i = 0; i < device_.timer16_count; ++i) {
                const auto& desc = device_.timers16[i];
                if (t16->name() == "TIMER" + std::to_string(desc.timer_index)) {
                    if (auto* port = resolve_port(desc.ocra_pin_address)) {
                        t16->connect_compare_output_a(*port, desc.ocra_pin_bit);
                    }
                    if (auto* port = resolve_port(desc.ocrb_pin_address)) {
                        t16->connect_compare_output_b(*port, desc.ocrb_pin_bit);
                    }
                    if (auto* port = resolve_port(desc.ocrc_pin_address)) {
                        t16->connect_compare_output_c(*port, desc.ocrc_pin_bit);
                    }
                    if (auto* port = resolve_port(desc.icp_pin_address)) {
                        t16->connect_input_capture(*port, desc.icp_pin_bit);
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
                
                // Wire AC to PSC based on index for AT90PWM series
                u8 psc_idx = ac->name().back() - '0';
                for (auto& p2 : owned_peripherals_) {
                    if (auto* psc = dynamic_cast<Psc*>(p2.get())) {
                        std::string psc_target = "PSC" + std::to_string(psc_idx);
                        if (psc->name() == psc_target || psc->name() == "PSC") {
                            ac->connect_psc_fault(*psc);
                        }
                    }
                }
            }
            if (auto* t8 = dynamic_cast<Timer8*>(p.get())) {
                adc->connect_timer_compare_auto_trigger(*t8);
                adc->connect_timer_overflow_auto_trigger(*t8);
            }
            if (auto* t16 = dynamic_cast<Timer16*>(p.get())) {
                t16->connect_adc(*adc);
            }
            if (auto* ext = dynamic_cast<ExtInterrupt*>(p.get())) {
                adc->connect_external_interrupt_0_auto_trigger(*ext);
            }
            if (auto* psc = dynamic_cast<Psc*>(p.get())) {
                psc->connect_adc_trigger(*adc);
            }
        }
    }

    // Wire Timer0 compare match -> USI (for USICS=10 clock source)
    if (device_.usi_count > 0) {
        Usi* usi = nullptr;
        for (auto& p : owned_peripherals_) {
            if (auto* u = dynamic_cast<Usi*>(p.get())) {
                usi = u;
                break;
            }
        }
        if (usi) {
            for (auto& p : owned_peripherals_) {
                if (auto* t8 = dynamic_cast<Timer8*>(p.get())) {
                    if (t8->name() == "TIMER0") {
                        t8->connect_usi_timer0_clock(*usi);
                        break;
                    }
                }
            }
        }
    }

    // Re-connect AVR8X peripherals that the AvrCpu constructor missed
    // (constructor ran before initialize_peripherals created them)
    for (auto& p : owned_peripherals_) {
        if (auto* wdt = dynamic_cast<Wdt8x*>(p.get())) {
            cpu_->set_wdt8x(wdt);
        } else if (auto* rst = dynamic_cast<RstCtrl*>(p.get())) {
            cpu_->set_rst_ctrl(rst);
        }
    }
}

} // namespace vioavr::core
