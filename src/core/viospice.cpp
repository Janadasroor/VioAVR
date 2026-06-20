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
#include "vioavr/core/lcd_controller.hpp"
#include "vioavr/core/dac.hpp"
#include "vioavr/core/dac8x.hpp"
#include "vioavr/core/adc8x.hpp"
#include "vioavr/core/adc10b.hpp"
#include "vioavr/core/ac8x.hpp"
#include "vioavr/core/usb.hpp"
#include "vioavr/core/usb8x.hpp"
#include "vioavr/core/port_mux.hpp"
#include "vioavr/core/evsys.hpp"
#include "vioavr/core/tca.hpp"
#include "vioavr/core/tcb.hpp"
#include "vioavr/core/tc.hpp"
#include "vioavr/core/awex.hpp"
#include "vioavr/core/rtc.hpp"
#include "vioavr/core/spi8x.hpp"
#include "vioavr/core/twi8x.hpp"
#include "vioavr/core/nvm_ctrl.hpp"
#include "vioavr/core/cpu_int.hpp"
#include "vioavr/core/wdt8x.hpp"
#include "vioavr/core/crc8x.hpp"
#include "vioavr/core/ccl.hpp"
#include "vioavr/core/tcd.hpp"
#include "vioavr/core/tce.hpp"
#include "vioavr/core/adcea.hpp"
#include "vioavr/core/xmegadc.hpp"
#include "vioavr/core/xmegaac.hpp"
#include "vioavr/core/zcd.hpp"
#include "vioavr/core/opamp.hpp"
#include "vioavr/core/cfd.hpp"
#include "vioavr/core/ptc.hpp"
#include "vioavr/core/lin.hpp"
#include "vioavr/core/xmega_dac.hpp"
#include "vioavr/core/ebi.hpp"
#include "vioavr/core/ircom.hpp"
#include "vioavr/core/dma.hpp"
#include "vioavr/core/rstctrl.hpp"
#include "vioavr/core/slpctrl.hpp"
#include "vioavr/core/syscfg.hpp"
#include "vioavr/core/vref.hpp"
#include "vioavr/core/bodctrl.hpp"
#include "vioavr/core/mvio.hpp"
#include "vioavr/core/clkctrl.hpp"
#include "vioavr/core/xmega_clkctrl.hpp"
#include "vioavr/core/eusart.hpp"
#include "vioavr/core/psc.hpp"
#include "vioavr/core/usi.hpp"
#include "vioavr/core/pll.hpp"
#include "vioavr/core/amplifier.hpp"
#include <map>

namespace vioavr::core {

static u8 compute_pin_mux_port_count_viospice(const DeviceDescriptor& device) noexcept
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

VioSpice::VioSpice(const DeviceDescriptor& device)
    : pin_mux_(compute_pin_mux_port_count_viospice(device)), bus_(device), cpu_(bus_)
{
    pin_map_ = std::make_unique<PinMap>();
    bus_.set_pin_map(pin_map_.get());
    bus_.set_pin_mux(&pin_mux_);
    cpu_.set_trace_hook(&trace_mux_);

    // 1. GPIO Ports
    for (u8 i = 0; i < device.port_count; ++i) {
        const auto& desc = device.ports[i];
        if (desc.name.empty()) continue;

        auto port = std::make_unique<GpioPort>(desc, pin_mux_);
        GpioPort* ptr = port.get();
        ports_.push_back(ptr);
        port_map_[std::string(desc.name)] = ptr;
        u8 port_idx = (desc.name.size() >= 5 && desc.name[0] == 'P')
                      ? static_cast<u8>(desc.name[4] - 'A') : i;
        port_idx_to_name_[port_idx] = std::string(desc.name);
        bus_.attach_peripheral(*port);
        owned_peripherals_.push_back(std::move(port));
    }

    // 1.5 PortMux (AVR8X) — also created when TCA/TCB/UART8x/SPI8x/TWI8x exist
    // (ATtiny chips have fixed routing, no TCAROUTEA register)
    if (device.portmux.tcaroutea_address != 0 || device.tca_count > 0 || device.tcb_count > 0 ||
        device.uart8x_count > 0 || device.spi8x_count > 0 || device.twi8x_count > 0) {
        auto pm = std::make_unique<PortMux>(device.portmux);
        port_mux_ = pm.get();
        port_mux_->set_pin_mux(&pin_mux_);
        for (u8 i = 0; i < static_cast<u8>(ports_.size()); ++i) {
            port_mux_->register_port(ports_[i]->port_address(), i);
        }
        bus_.attach_peripheral(*pm);
        owned_peripherals_.push_back(std::move(pm));
    }

    // 1.6 System Control Peripherals (AVR8X)
    if (device.rstctrl.rstfr_address != 0U) {
        auto p = std::make_unique<RstCtrl>(device.rstctrl);
        bus_.attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    if (device.slpctrl.ctrla_address != 0U) {
        auto p = std::make_unique<SlpCtrl>(device.slpctrl);
        bus_.attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    if (device.syscfg.reves_address != 0U) {
        auto p = std::make_unique<SysCfg>(device.syscfg);
        bus_.attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    if (device.vref.ctrla_address != 0U) {
        auto p = std::make_unique<VRef>(device.vref);
        bus_.attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    if (device.bod.ctrla_address != 0U) {
        auto p = std::make_unique<BodCtrl>(device.bod);
        bus_.attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    if (device.mvio.intctrl_address != 0U) {
        auto p = std::make_unique<Mvio>(device.mvio);
        p->set_memory_bus(&bus_);
        bus_.attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    // AVR8X ClkCtrl (MCLKCTRLA/MCLKCTRLB style)
    if (device.clkctrl.ctrla_address != 0U && device.tc_count == 0U) {
        auto p = std::make_unique<ClkCtrl>(device.clkctrl, 20'000'000U);
        bus_.attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }
    // XMEGA ClkCtrl (CLK.CTRL/PSCTRL/LOCK/RTCCTRL style at base 0x60)
    if (device.tc_count > 0U) {
        auto p = std::make_unique<XmegaClkCtrl>(0x60U, 0x61U, 0x63U, 0x64U);
        bus_.attach_peripheral(*p);
        owned_peripherals_.push_back(std::move(p));
    }

    // 1.7 DMA + EventSystem (AVR8X)
    EventSystem* evsys = nullptr;
    if (device.evsys.strobe_address != 0) {
        auto e = std::make_unique<EventSystem>(device.evsys);
        evsys = e.get();
        evsys_ = e.get();
        e->set_memory_bus(&bus_);
        bus_.attach_peripheral(*e);
        owned_peripherals_.push_back(std::move(e));
    }

    Dma* dma = nullptr;
    if (device.dma_count > 0) {
        for (u8 i = 0; i < device.dma_count; ++i) {
            auto d = std::make_unique<Dma>(device.dmas[i]);
            dma = d.get();
            d->set_memory_bus(&bus_);
            bus_.attach_peripheral(*d);
            owned_peripherals_.push_back(std::move(d));
        }
    }
    if (dma && evsys) {
        dma->set_event_system(evsys);
    }

    // Wire EVOUT routing: EVSYS user callbacks drive PortMux pins
    if (evsys && port_mux_ && device.evsys.evout_user_count > 0) {
        u8 evout_start = device.evsys.evout_user_start;
        for (u8 i = 0; i < device.evsys.evout_user_count; ++i) {
            evsys->register_user_callback(evout_start + i, [pm = port_mux_, i](bool level) {
                pm->drive_evout(i, level);
            });
        }
    }

    // 2. Classic Timers
    for (u8 i = 0; i < device.timer8_count; ++i) {
        const auto& desc = device.timers8[i];
        auto timer = std::make_unique<Timer8>("TIMER8_" + std::to_string(i), desc, &pin_mux_);
        timer->set_bus(bus_);
        if (desc.assr_address != 0U) timer2_ = timer.get();

        if (desc.ocra_pin_address != 0) {
            for (auto* port : ports_) {
                if (port->port_address() == desc.ocra_pin_address || port->pin_address() == desc.ocra_pin_address) {
                    timer->connect_compare_output_a(*port, desc.ocra_pin_bit);
                    break;
                }
            }
        }
        if (desc.ocrb_pin_address != 0) {
            for (auto* port : ports_) {
                if (port->port_address() == desc.ocrb_pin_address || port->pin_address() == desc.ocrb_pin_address) {
                    timer->connect_compare_output_b(*port, desc.ocrb_pin_bit);
                    break;
                }
            }
        }

        bus_.attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    for (u8 i = 0; i < device.timer16_count; ++i) {
        const auto& desc16 = device.timers16[i];
        auto timer = std::make_unique<Timer16>("TIMER16_" + std::to_string(i), desc16, &pin_mux_);
        timer->set_bus(bus_);

        if (desc16.ocra_pin_address != 0) {
            for (auto* port : ports_) {
                if (port->port_address() == desc16.ocra_pin_address || port->pin_address() == desc16.ocra_pin_address) {
                    timer->connect_compare_output_a(*port, desc16.ocra_pin_bit);
                    break;
                }
            }
        }
        if (desc16.ocrb_pin_address != 0) {
            for (auto* port : ports_) {
                if (port->port_address() == desc16.ocrb_pin_address || port->pin_address() == desc16.ocrb_pin_address) {
                    timer->connect_compare_output_b(*port, desc16.ocrb_pin_bit);
                    break;
                }
            }
        }
        if (desc16.ocrc_pin_address != 0) {
            for (auto* port : ports_) {
                if (port->port_address() == desc16.ocrc_pin_address || port->pin_address() == desc16.ocrc_pin_address) {
                    timer->connect_compare_output_c(*port, desc16.ocrc_pin_bit);
                    break;
                }
            }
        }
        if (desc16.icp_pin_address != 0) {
            for (auto* port : ports_) {
                if (port->port_address() == desc16.icp_pin_address || port->pin_address() == desc16.icp_pin_address) {
                    timer->connect_input_capture(*port, desc16.icp_pin_bit);
                    break;
                }
            }
        }

        bus_.attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    // 2.5 AVR8X Timers (TCA, TCB, RTC)
    for (u8 i = 0; i < device.tca_count; ++i) {
        auto timer = std::make_unique<Tca>("TCA" + std::to_string(i), device.timers_tca[i]);
        timer->set_memory_bus(&bus_);
        timer->set_event_system(evsys);
        if (port_mux_) {
            timer->set_port_mux(port_mux_);
            port_mux_->add_observer(timer.get());
        }
        bus_.attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    for (u8 i = 0; i < device.tcb_count; ++i) {
        auto timer = std::make_unique<Tcb>("TCB" + std::to_string(i), device.timers_tcb[i]);
        timer->set_memory_bus(&bus_);
        timer->set_event_system(evsys);
        if (port_mux_) {
            timer->set_port_mux(port_mux_);
            port_mux_->add_observer(timer.get());
        }
        bus_.attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    for (u8 i = 0; i < device.rtc_count; ++i) {
        auto timer = std::make_unique<Rtc>(device.timers_rtc[i]);
        timer->set_memory_bus(&bus_);
        timer->set_event_system(evsys);
        bus_.attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    // Timer10
    for (u8 i = 0; i < device.timer10_count; ++i) {
        auto timer = std::make_unique<Timer10>("TIMER10_" + std::to_string(i), device.timers10[i]);
        timer->set_bus(bus_);
        bus_.attach_peripheral(*timer);
        owned_peripherals_.push_back(std::move(timer));
    }

    // 3. Analog
    for (u8 i = 0; i < device.adc_count; ++i) {
        auto adc = std::make_unique<Adc>("ADC" + std::to_string(i), device.adcs[i], pin_mux_, i, 0);
        adc->set_bus(bus_);
        adc->bind_signal_bank(analog_signal_bank_);
        if (pin_map_) {
            adc->bind_pin_map(*pin_map_);
        }
        adc->set_vref(device.operating_voltage_v);
        bus_.attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    for (u8 i = 0; i < device.adc8x_count; ++i) {
        auto adc = std::make_unique<Adc8x>(device.adcs8x[i]);
        adc->set_memory_bus(&bus_);
        adc->set_event_system(evsys);
        adc->set_analog_signal_bank(&analog_signal_bank_);
        adc->set_vdd(5.0);
        bus_.attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    for (u8 i = 0; i < device.adc10b_count; ++i) {
        auto adc = std::make_unique<Adc10b>(device.adcs10b[i]);
        adc->set_memory_bus(&bus_);
        adc->set_analog_signal_bank(&analog_signal_bank_);
        bus_.attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    for (u8 i = 0; i < device.ac8x_count; ++i) {
        auto ac = std::make_unique<Ac8x>("AC" + std::to_string(i), device.acs8x[i]);
        ac->set_memory_bus(&bus_);
        ac->set_event_system(evsys);
        ac->set_analog_signal_bank(&analog_signal_bank_);
        ac->set_vdd(device.operating_voltage_v);
        bus_.attach_peripheral(*ac);
        owned_peripherals_.push_back(std::move(ac));
    }

    for (u8 i = 0; i < device.dac_count; ++i) {
        auto dac = std::make_unique<Dac>("DAC" + std::to_string(i), device.dacs[i]);
        dac->set_memory_bus(&bus_);
        dac->set_analog_signal_bank(&analog_signal_bank_);
        dac->set_vref(device.operating_voltage_v);
        dacs_.push_back(dac.get());
        bus_.attach_peripheral(*dac);
        owned_peripherals_.push_back(std::move(dac));
    }

    for (u8 i = 0; i < device.dac8x_count; ++i) {
        auto dac = std::make_unique<Dac8x>(device.dacs8x[i], i);
        dac->set_analog_signal_bank(&analog_signal_bank_);
        dac->set_vref(device.operating_voltage_v);
        bus_.attach_peripheral(*dac);
        owned_peripherals_.push_back(std::move(dac));
    }

    // 4. Communication
    for (u8 i = 0; i < device.uart_count; ++i) {
        auto u = std::make_unique<Uart>("UART" + std::to_string(i), device.uarts[i], pin_mux_);
        if (i == 0) uart0_ = u.get();
        bus_.attach_peripheral(*u);
        owned_peripherals_.push_back(std::move(u));
    }

    for (u8 i = 0; i < device.uart8x_count; ++i) {
        auto uart = std::make_unique<Uart8x>(device.uarts8x[i], pin_mux_);
        uart->set_memory_bus(&bus_);
        uart->set_event_system(evsys);
        uart->set_port_mux(port_mux_);
        if (port_mux_) port_mux_->add_observer(uart.get());
        if (i == 0) uart8x0_ = uart.get();
        bus_.attach_peripheral(*uart);
        owned_peripherals_.push_back(std::move(uart));
    }

    for (u8 i = 0; i < device.spi_count; ++i) {
        auto spi = std::make_unique<Spi>("SPI" + std::to_string(i), device.spis[i]);
        bus_.attach_peripheral(*spi);
        owned_peripherals_.push_back(std::move(spi));
    }

    for (u8 i = 0; i < device.spi8x_count; ++i) {
        auto spi = std::make_unique<Spi8x>(device.spis8x[i]);
        spi->set_memory_bus(&bus_);
        spi->set_event_system(evsys);
        spi->set_port_mux(port_mux_);
        bus_.attach_peripheral(*spi);
        owned_peripherals_.push_back(std::move(spi));
    }

    for (u8 i = 0; i < device.twi_count; ++i) {
        auto twi = std::make_unique<Twi>("TWI" + std::to_string(i), device.twis[i]);
        bus_.attach_peripheral(*twi);
        owned_peripherals_.push_back(std::move(twi));
    }

    for (u8 i = 0; i < device.twi8x_count; ++i) {
        auto twi = std::make_unique<Twi8x>(device.twis8x[i]);
        twi->set_memory_bus(&bus_);
        twi->set_event_system(evsys);
        twi->set_port_mux(port_mux_);
        bus_.attach_peripheral(*twi);
        owned_peripherals_.push_back(std::move(twi));
    }

    for (u8 i = 0; i < device.usi_count; ++i) {
        auto usi = std::make_unique<Usi>("USI", device.usis[i]);
        bus_.attach_peripheral(*usi);
        owned_peripherals_.push_back(std::move(usi));
    }

    // Wire Timer0 -> USI for USICS=10 clock source
    if (device.usi_count > 0 && device.timer8_count > 0) {
        Usi* usi_ptr = nullptr;
        Timer8* t8_ptr = nullptr;
        for (auto& p : owned_peripherals_) {
            if (auto* u = dynamic_cast<Usi*>(p.get())) {
                usi_ptr = u;
            }
            if (auto* t = dynamic_cast<Timer8*>(p.get())) {
                if (t->name() == "TIMER8_0") t8_ptr = t;
            }
        }
        if (usi_ptr && t8_ptr) {
            t8_ptr->connect_usi_timer0_clock(*usi_ptr);
        }
    }

    for (u8 i = 0; i < device.eusart_count; ++i) {
        auto eusart = std::make_unique<Eusart>("EUSART", device.eusarts[i]);
        bus_.attach_peripheral(*eusart);
        owned_peripherals_.push_back(std::move(eusart));
    }

    for (u8 i = 0; i < device.psc_count; ++i) {
        auto psc = std::make_unique<Psc>("PSC", device.pscs[i], &pin_mux_);
        bus_.attach_peripheral(*psc);
        owned_peripherals_.push_back(std::move(psc));
    }

    // 5. EEPROM
    for (u8 i = 0; i < device.eeprom_count; ++i) {
        auto eeprom = std::make_unique<Eeprom>("EEPROM" + std::to_string(i), device.eeproms[i]);
        eeprom->set_memory_bus(&bus_);
        if (i == 0) bus_.set_eeprom(eeprom.get());
        bus_.attach_peripheral(*eeprom);
        owned_peripherals_.push_back(std::move(eeprom));
    }

    // 6. Watchdog
    for (u8 i = 0; i < device.wdt_count; ++i) {
        auto wdt = std::make_unique<WatchdogTimer>("WDT", device.wdts[i], cpu_);
        cpu_.set_watchdog_timer(wdt.get());
        bus_.attach_peripheral(*wdt);
        owned_peripherals_.push_back(std::move(wdt));
    }

    for (u8 i = 0; i < device.wdt8x_count; ++i) {
        auto wdt = std::make_unique<Wdt8x>(device.wdts8x[i], cpu_);
        if (i == 0) cpu_.set_wdt8x(wdt.get());
        bus_.attach_peripheral(*wdt);
        owned_peripherals_.push_back(std::move(wdt));
    }

    // 7. NVM Controller
    for (u8 i = 0; i < device.nvm_ctrl_count; ++i) {
        auto nvm = std::make_unique<NvmCtrl>(device.nvm_ctrls[i]);
        nvm->set_bus(bus_);
        if (i == 0) bus_.set_nvm_ctrl(nvm.get());
        bus_.attach_peripheral(*nvm);
        owned_peripherals_.push_back(std::move(nvm));
    }

    // 8. CPUINT (Modern Interrupt Controller)
    for (u8 i = 0; i < device.cpu_int_count; ++i) {
        auto cpu_int = std::make_unique<CpuInt>(device.cpu_ints[i]);
        if (i == 0) bus_.set_cpu_int(cpu_int.get());
        bus_.attach_peripheral(*cpu_int);
        owned_peripherals_.push_back(std::move(cpu_int));
    }

    // 9. External Interrupts
    if (device.ext_interrupt_count > 0) {
        auto ext_int = std::make_unique<ExtInterrupt>("EXINT", device.ext_interrupts[0], pin_mux_, 0);
        bus_.attach_peripheral(*ext_int);
        owned_peripherals_.push_back(std::move(ext_int));
    }

    // 10. Pin Change Interrupts
    for (u8 i = 0; i < device.pcint_count; ++i) {
        GpioPort* target_port = nullptr;
        if (i < ports_.size()) target_port = ports_[i];
        if (target_port) {
            auto pcint = std::make_unique<PinChangeInterrupt>("PCINT" + std::to_string(i),
                device.pcints[i], *target_port, pcint_shared_state_, i == 0);
            bus_.attach_peripheral(*pcint);
            owned_peripherals_.push_back(std::move(pcint));
        }
    }

    // 11. Analog Comparator
    for (int i = 0; i < static_cast<int>(device.ac_count); ++i) {
        const auto& desc = device.acs[i];
        auto ac = std::make_unique<AnalogComparator>("AC" + std::to_string(i), desc, pin_mux_, static_cast<u8>(i));
        
        u8 pos_chan = 0;
        u8 neg_chan = 0;
        if (pin_map_) {
            if (auto ext_id = pin_map_->get_external_id_by_addr(desc.aip_pin_address, desc.aip_pin_bit)) {
                pos_chan = *ext_id;
            }
            if (auto ext_id = pin_map_->get_external_id_by_addr(desc.aim_pin_address, desc.aim_pin_bit)) {
                neg_chan = *ext_id;
            }
        }
        ac->bind_signal_bank(analog_signal_bank_, pos_chan, neg_chan);

        for (auto& p : owned_peripherals_) {
            if (auto* t16 = dynamic_cast<Timer16*>(p.get())) {
                if (t16->name() == "TIMER1") t16->connect_analog_comparator(*ac);
            }
        }
        bus_.attach_peripheral(*ac);
        owned_peripherals_.push_back(std::move(ac));
    }

    // Wire ADC auto-trigger sources (AC, Timer8, Timer16, PSC)
    {
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
    }

    // 12. CCL
    if (device.ccl.ctrla_address != 0) {
        auto c = std::make_unique<Ccl>(device.ccl);
        c->set_memory_bus(&bus_);
        c->set_event_system(evsys);
        c->set_pin_mux(&pin_mux_);
        bus_.attach_peripheral(*c);
        owned_peripherals_.push_back(std::move(c));
    }

    // 13. CRC (AVR8X)
    for (u8 i = 0; i < device.crc8x_count; ++i) {
        auto crc = std::make_unique<Crc8x>(device.crcs8x[i], bus_.flash_words());
        bus_.attach_peripheral(*crc);
        owned_peripherals_.push_back(std::move(crc));
    }

    // 14. TCD
    for (u8 i = 0; i < device.tcd_count; ++i) {
        auto tcd = std::make_unique<Tcd>(device.timers_tcd[i]);
        bus_.attach_peripheral(*tcd);
        owned_peripherals_.push_back(std::move(tcd));
    }

    // 14b. TCE (LA family)
    for (u8 i = 0; i < device.tce_count; ++i) {
        auto tce = std::make_unique<Tce>("TCE" + std::to_string(i), device.timers_tce[i]);
        tce->set_memory_bus(&bus_);
        tce->set_event_system(evsys);
        if (port_mux_) {
            tce->set_port_mux(port_mux_);
            port_mux_->add_observer(tce.get());
        }
        bus_.attach_peripheral(*tce);
        owned_peripherals_.push_back(std::move(tce));
    }

    // 14c. TC (XMEGA 16-bit Timer/Counter) — collect raw pointers for AWEX pairing
    std::vector<Tc*> tc_ptrs;
    tc_ptrs.reserve(device.tc_count);
    for (u8 i = 0; i < device.tc_count; ++i) {
        auto tc = std::make_unique<Tc>("TC" + std::to_string(i), device.timers_tc[i]);
        tc->set_memory_bus(&bus_);
        tc->set_event_system(evsys);
        if (port_mux_) {
            tc->set_port_mux(port_mux_);
            port_mux_->add_observer(tc.get());
        }
        tc->set_pin_mux(&pin_mux_);
        {
            char port_letter = static_cast<char>(device.timers_tc[i].port_letter);
            if (!port_letter) {
                u16 tc_base = device.timers_tc[i].ctrla_address;
                if (tc_base >= 0x0800 && tc_base < 0x0900) port_letter = 'C';
                else if (tc_base >= 0x0900 && tc_base < 0x0A00) port_letter = 'D';
                else if (tc_base >= 0x0A00 && tc_base < 0x0B00) port_letter = 'E';
                else if (tc_base >= 0x0B00 && tc_base < 0x0C00) port_letter = 'F';
            }
            if (port_letter) {
                tc->set_wg_mode_in_ctrld(true);
                for (u8 p = 0; p < device.port_count; ++p) {
                    auto& pn = device.ports[p].name;
                    if (pn.size() > 4 && pn[4] == port_letter) {
                        tc->set_port_index(static_cast<u8>(pn[4] - 'A'));
                        break;
                    }
                }
            }
        }
        tc_ptrs.push_back(tc.get());
        bus_.attach_peripheral(*tc);
        owned_peripherals_.push_back(std::move(tc));
    }

    // 14d. AWEX (XMEGA Advanced Waveform Extension) — pair with companion TC
    for (u8 i = 0; i < device.awex_count; ++i) {
        auto awex = std::make_unique<Awex>("AWEX" + std::to_string(i), device.awexs[i]);
        awex->set_memory_bus(&bus_);
        if (port_mux_) {
            awex->set_port_mux(port_mux_);
        }
        awex->set_pin_mux(&pin_mux_);
        if (i < tc_ptrs.size()) {
            tc_ptrs[i]->set_awex(awex.get());
        }
        bus_.attach_peripheral(*awex);
        owned_peripherals_.push_back(std::move(awex));
    }

    // 14e. AdcEa (EA 12-bit diff ADC)
    for (u8 i = 0; i < device.adcea_count; ++i) {
        auto adc = std::make_unique<AdcEa>(device.adceas[i]);
        adc->set_memory_bus(&bus_);
        adc->set_analog_signal_bank(&analog_signal_bank_);
        bus_.attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    // 14f. XMEGA ADC
    for (u8 i = 0; i < device.adc_xmega_count; ++i) {
        auto adc = std::make_unique<XmegaAdc>(device.adcs_xmega[i]);
        adc->set_memory_bus(&bus_);
        adc->set_analog_signal_bank(&analog_signal_bank_);
        bus_.attach_peripheral(*adc);
        owned_peripherals_.push_back(std::move(adc));
    }

    // 14g. XMEGA AC
    for (u8 i = 0; i < device.ac_xmega_count; ++i) {
        auto ac = std::make_unique<XmegaAc>(device.acs_xmega[i]);
        ac->set_memory_bus(&bus_);
        ac->set_analog_signal_bank(&analog_signal_bank_);
        bus_.attach_peripheral(*ac);
        owned_peripherals_.push_back(std::move(ac));
    }

    // 15. USB
    for (u8 i = 0; i < device.usb_count; ++i) {
        auto usb = std::make_unique<Usb>("USB", device.usbs[i]);
        bus_.attach_peripheral(*usb);
        owned_peripherals_.push_back(std::move(usb));
    }

    // USB8x (DU / XMEGA)
    for (u8 i = 0; i < device.usb8x_count; ++i) {
        auto usb = std::make_unique<Usb8x>(device.usbs8x[i]);
        usb->set_memory_bus(&bus_);
        bus_.attach_peripheral(*usb);
        owned_peripherals_.push_back(std::move(usb));
    }

    // XMEGA DAC
    for (u8 i = 0; i < device.xmega_dac_count; ++i) {
        auto dac = std::make_unique<XmegaDac>("XDAC" + std::to_string(i), device.xmega_dacs[i]);
        dac->set_memory_bus(&bus_);
        dac->set_analog_signal_bank(&analog_signal_bank_);
        dac->set_vref(bus_.device().operating_voltage_v);
        bus_.attach_peripheral(*dac);
        xmega_dacs_.push_back(dac.get());
        owned_peripherals_.push_back(std::move(dac));
    }

    // EBI (External Bus Interface)
    for (u8 i = 0; i < device.ebi_count; ++i) {
        auto ebi = std::make_unique<Ebi>("EBI", device.ebis[i]);
        ebi->set_memory_bus(&bus_);
        bus_.attach_peripheral(*ebi);
        bus_.set_ebi(ebi.get());
        owned_peripherals_.push_back(std::move(ebi));
    }

    // IRCOM (IR Communication Module)
    for (u8 i = 0; i < device.ircom_count; ++i) {
        auto ircom = std::make_unique<Ircom>("IRCOM", device.ircoms[i]);
        ircom->set_memory_bus(&bus_);
        ircom->set_pin_mux(&pin_mux_);
        if (device.ircoms[i].pin_address != 0) {
            for (auto* port : ports_) {
                if (port && port->port_address() == device.ircoms[i].pin_address) {
                    u8 port_letter = port->name()[4] - 'A';
                    ircom->set_output_pin(port_letter, device.ircoms[i].pin_bit_index);
                    break;
                }
            }
        } else {
            // Default to first available GPIO port, bit 0
            for (auto* port : ports_) {
                if (port) {
                    ircom->set_output_pin(port->name()[4] - 'A', 0);
                    break;
                }
            }
        }
        bus_.attach_peripheral(*ircom);
        owned_peripherals_.push_back(std::move(ircom));
    }

    // 16. LCD
    for (u8 i = 0; i < device.lcd_count; ++i) {
        auto lcd = std::make_unique<LcdController>("LCD", device.lcds[i], pin_mux_);
        lcd_ = lcd.get();
        bus_.attach_peripheral(*lcd);
        owned_peripherals_.push_back(std::move(lcd));
    }

    // 17. CAN
    for (u8 i = 0; i < device.can_count; ++i) {
        auto can = std::make_unique<CanBus>("CAN" + std::to_string(i), device.cans[i]);
        bus_.attach_peripheral(*can);
        owned_peripherals_.push_back(std::move(can));
    }

    // 18. Amplifiers
    for (u8 i = 0; i < device.amplifier_count; ++i) {
        auto amp = std::make_unique<At90Amplifier>("AMP" + std::to_string(i), device.amplifiers[i], pin_mux_);
        bus_.attach_peripheral(*amp);
        owned_peripherals_.push_back(std::move(amp));
    }

    // 19. ZCD
    for (u8 i = 0; i < device.zcd_count; ++i) {
        auto zcd = std::make_unique<Zcd>(device.zcds[i], i);
        zcd->set_analog_signal_bank(&analog_signal_bank_);
        zcd->set_input_channel(static_cast<u8>(100 + i));
        zcd->set_vdd(device.operating_voltage_v);
        if (i == 0) bus_.set_zcd(zcd.get());
        bus_.attach_peripheral(*zcd);
        owned_peripherals_.push_back(std::move(zcd));
    }

    // 20. OPAMP
    for (u8 i = 0; i < device.opamp_count; ++i) {
        auto op = std::make_unique<Opamp>(device.opamps[i], i);
        op->set_analog_signal_bank(&analog_signal_bank_);
        op->set_vdd(device.operating_voltage_v);
        bus_.attach_peripheral(*op);
        owned_peripherals_.push_back(std::move(op));
    }

    // 21. CFD
    for (u8 i = 0; i < device.cfd_count; ++i) {
        auto cfd = std::make_unique<Cfd>(device.cfds[i]);
        cfd->set_memory_bus(&bus_);
        bus_.attach_peripheral(*cfd);
        owned_peripherals_.push_back(std::move(cfd));
    }

    // 22. PTC
    for (u8 i = 0; i < device.ptc_count; ++i) {
        auto ptc = std::make_unique<Ptc>(device.ptcs[i]);
        bus_.attach_peripheral(*ptc);
        owned_peripherals_.push_back(std::move(ptc));
    }

    // 23. LIN
    for (u8 i = 0; i < device.lin_count; ++i) {
        auto lin = std::make_unique<LinUART>(device.lins[i]);
        lin->set_memory_bus(&bus_);
        lin->set_pin_mux(&pin_mux_);
        bus_.attach_peripheral(*lin);
        owned_peripherals_.push_back(std::move(lin));
    }

    // 24. PLL Controller
    u16 pll_addr = 0;
    if (device.psc_count > 0) pll_addr = device.pscs[0].pllcsr_address;
    else if (device.usb_count > 0) pll_addr = device.usbs[0].pllcsr_address;
    if (pll_addr != 0) {
        auto pll = std::make_unique<PllController>(pll_addr);
        bus_.attach_peripheral(*pll);
        owned_peripherals_.push_back(std::move(pll));
    }

    // 25. XMEM
    if (device.xmem.xmcra_address != 0) {
        auto xmem = std::make_unique<Xmem>(bus_.device(), cpu_.cpu_control());
        bus_.attach_peripheral(*xmem);
        bus_.set_xmem(xmem.get());
        owned_peripherals_.push_back(std::move(xmem));
    }

    // PinMux callback (set after all peripherals created to capture runtime pin changes)
    pin_mux_.set_callback([this](u8 port_idx, u8 bit_idx, const PinState& state) {
        PinStateChange change;
        auto it = port_idx_to_name_.find(port_idx);
        if (it != port_idx_to_name_.end()) {
            change.port_name = it->second;
        } else {
            change.port_name = "PORT?";
        }
        change.bit_index = bit_idx;
        change.level = state.drive_level;
        change.is_output = state.is_output;
        change.wired_and = state.is_wired_and;
        change.cycle_stamp = cpu_.cycles();
        pending_pin_changes_.push_back(std::move(change));
    });

    // Wire AVR8X peripherals to CPU (constructor ran before these were created)
    for (auto& p : owned_peripherals_) {
        if (auto* wdt = dynamic_cast<Wdt8x*>(p.get())) {
            cpu_.set_wdt8x(wdt);
        } else if (auto* rst = dynamic_cast<RstCtrl*>(p.get())) {
            cpu_.set_rst_ctrl(rst);
        } else if (auto* slp = dynamic_cast<SlpCtrl*>(p.get())) {
            cpu_.set_slp_ctrl(slp);
        } else if (auto* clk = dynamic_cast<ClkCtrl*>(p.get())) {
            cpu_.set_clk_ctrl(clk);
        } else if (auto* xclk = dynamic_cast<XmegaClkCtrl*>(p.get())) {
            cpu_.set_xmega_clk_ctrl(xclk);
        }
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

bool VioSpice::load_hex_image(const HexImage& image) {
    bus_.load_image(image);
    return true;
}

void VioSpice::set_pin_map(std::unique_ptr<PinMap> pin_map) {
    pin_map_ = std::move(pin_map);
    bus_.set_pin_map(pin_map_.get());
    if (pin_map_) {
        for (auto& p : owned_peripherals_) {
            if (auto* adc = dynamic_cast<Adc*>(p.get())) {
                adc->bind_pin_map(*pin_map_);
            } else if (auto* ac = dynamic_cast<AnalogComparator*>(p.get())) {
                const auto& desc = ac->descriptor();
                u8 pos_chan = 0;
                u8 neg_chan = 0;
                if (auto ext_id = pin_map_->get_external_id_by_addr(desc.aip_pin_address, desc.aip_pin_bit)) {
                    pos_chan = *ext_id;
                }
                if (auto ext_id = pin_map_->get_external_id_by_addr(desc.aim_pin_address, desc.aim_pin_bit)) {
                    neg_chan = *ext_id;
                }
                ac->bind_signal_bank(analog_signal_bank_, pos_chan, neg_chan);
            }
        }
    }
}

void VioSpice::add_pin_mapping(std::string_view port_name, u8 bit_index, u32 external_id, std::string_view label) {
    if (pin_map_) {
        u16 addr = 0;
        auto it = port_map_.find(std::string(port_name));
        if (it != port_map_.end()) addr = it->second->pin_address();
        pin_map_->add_mapping(port_name, addr, bit_index, external_id, label);

        // Re-bind analog comparators since the pin map has been updated
        for (auto& p : owned_peripherals_) {
            if (auto* ac = dynamic_cast<AnalogComparator*>(p.get())) {
                const auto& desc = ac->descriptor();
                u8 pos_chan = 0;
                u8 neg_chan = 0;
                if (auto ext_id = pin_map_->get_external_id_by_addr(desc.aip_pin_address, desc.aip_pin_bit)) {
                    pos_chan = *ext_id;
                }
                if (auto ext_id = pin_map_->get_external_id_by_addr(desc.aim_pin_address, desc.aim_pin_bit)) {
                    neg_chan = *ext_id;
                }
                ac->bind_signal_bank(analog_signal_bank_, pos_chan, neg_chan);
            }
        }
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
            bus_.propagate_external_pin_change(it->second->pin_address(), mapping->bit_index, level);
        }
    }
}

void VioSpice::set_external_voltage(u8 channel, double voltage_volts) {
    analog_signal_bank_.set_voltage(channel, voltage_volts);
}

void VioSpice::set_external_voltage_to_digital(u32 external_id, double voltage) {
    if (!pin_map_) return;
    auto mapping = pin_map_->get_mapping_by_external(external_id);
    if (!mapping) return;

    const double vcc = bus_.device().operating_voltage_v;
    const double clamped = (voltage < 0.0) ? 0.0 : (voltage > vcc) ? vcc : voltage;
    const double normalized = clamped / vcc;

    auto it = port_map_.find(mapping->port_name);
    if (it != port_map_.end()) {
        it->second->set_input_voltage(mapping->bit_index, normalized);
    }
}

void VioSpice::set_operating_voltage(double vcc) {
    bus_.device().operating_voltage_v = vcc;

    for (auto& p : owned_peripherals_) {
        IoPeripheral* ptr = p.get();
        if (auto* adc8x = dynamic_cast<Adc8x*>(ptr)) {
            adc8x->set_vdd(vcc);
        } else if (auto* ac8x = dynamic_cast<Ac8x*>(ptr)) {
            ac8x->set_vdd(vcc);
        } else if (auto* zcd = dynamic_cast<Zcd*>(ptr)) {
            zcd->set_vdd(vcc);
        } else if (auto* opamp = dynamic_cast<Opamp*>(ptr)) {
            opamp->set_vdd(vcc);
        } else if (auto* dac = dynamic_cast<Dac*>(ptr)) {
            dac->set_vref(vcc);
        } else if (auto* dac8x = dynamic_cast<Dac8x*>(ptr)) {
            dac8x->set_vref(vcc);
        } else if (auto* xdac = dynamic_cast<XmegaDac*>(ptr)) {
            xdac->set_vref(vcc);
        } else if (auto* adc = dynamic_cast<Adc*>(ptr)) {
            adc->set_vref(vcc);
        }
    }
}

std::vector<PinStateChange> VioSpice::consume_pin_changes() {
    std::vector<PinStateChange> changes = std::move(pending_pin_changes_);
    pending_pin_changes_.clear();
    return changes;
}

std::optional<u32> VioSpice::get_external_id(std::string_view port_name, u8 bit_index) const {
    if (pin_map_) return pin_map_->get_external_id(port_name, bit_index);
    return std::nullopt;
}

std::vector<double> VioSpice::get_analog_outputs() {
    std::vector<double> results;
    results.reserve(dacs_.size() + xmega_dacs_.size() * 2);
    for (auto* dac : dacs_) {
        results.push_back(dac->voltage());
    }
    for (auto* dac : xmega_dacs_) {
        results.push_back(dac->ch0_voltage());
        results.push_back(dac->ch1_voltage());
    }
    return results;
}

void VioSpice::step_duration(double seconds) {
    cpu_.run_duration(seconds);
    bridge_hc05();
}

void VioSpice::enable_hc05() {
    hc05_enabled_ = true;
}

void VioSpice::bridge_hc05() {
    if (!hc05_enabled_) return;
    u16 data;
    auto consume = [&](auto* u) -> bool {
        if (!u) return false;
        return u->consume_transmitted_byte(data);
    };
    auto inject = [&](auto* u, u16 d) {
        if (u) u->inject_received_byte(static_cast<u8>(d));
    };
    while (consume(uart0_) || consume(uart8x0_))
        hc05_.rx_byte(static_cast<u8>(data & 0xFF));
    while (hc05_.has_tx_byte()) {
        data = hc05_.read_tx_byte();
        inject(uart0_, data);
        if (!uart0_) inject(uart8x0_, data);
        if (hc05_pty_fd_ >= 0) {
            u8 b = static_cast<u8>(data & 0xFF);
            ::write(hc05_pty_fd_, &b, 1);
        }
    }
    while (hc05_.has_external_data()) {
        data = hc05_.read_injected_byte();
        inject(uart0_, data);
        if (!uart0_) inject(uart8x0_, data);
    }
}

void VioSpice::tick_timer2_async(u64 ticks) {
    if (timer2_) timer2_->tick_async(ticks);
}

void VioSpice::reset() {
    cpu_.reset();
    pin_mux_.reset();
    for (auto& p : owned_peripherals_) {
        p->reset();
    }
}

void VioSpice::set_frequency(double hz) {
    frequency_ = hz;
}

void VioSpice::set_quantum(u64 cycles) {
    quantum_ = cycles;
}

PinLevel VioSpice::get_pin_level(u32 external_id) const noexcept {
    if (!pin_map_) return PinLevel::low;
    auto mapping = pin_map_->get_mapping_by_external(external_id);
    if (!mapping) return PinLevel::low;
    auto state = pin_mux_.get_state_by_address(mapping->pin_address, mapping->bit_index);
    return state.drive_level ? PinLevel::high : PinLevel::low;
}

void VioSpice::set_ircom_output_pin(u32 external_id) {
    if (!pin_map_) return;
    auto mapping = pin_map_->get_mapping_by_external(external_id);
    if (!mapping) return;
    for (auto& owned : owned_peripherals_) {
        if (!owned) continue;
        auto* ircom = dynamic_cast<Ircom*>(owned.get());
        if (!ircom) continue;
        if (mapping->port_name.size() >= 5) {
            u8 port_letter = mapping->port_name[4] - 'A';
            ircom->set_output_pin(port_letter, mapping->bit_index);
        }
        return;
    }
}

} // namespace vioavr::core
