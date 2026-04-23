#pragma once
#include "vioavr/core/types.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/logger.hpp"
#include <map>
#include <array>
#include <cstdio>

namespace vioavr::core {

class IRoutingObserver {
public:
    virtual ~IRoutingObserver() = default;
    virtual void on_routing_changed() noexcept = 0;
};

class PortMux : public IoPeripheral {
public:
    explicit PortMux(const PortMuxDescriptor& desc) : desc_(desc) {
        if (desc_.tcaroutea_address != 0) ranges_[0] = {desc_.tcaroutea_address, desc_.tcaroutea_address};
        if (desc_.tcbroutea_address != 0) ranges_[1] = {desc_.tcbroutea_address, desc_.tcbroutea_address};
        if (desc_.usartroutea_address != 0) ranges_[2] = {desc_.usartroutea_address, desc_.usartroutea_address};
        if (desc_.twispiroutea_address != 0) ranges_[3] = {desc_.twispiroutea_address, desc_.twispiroutea_address};
        if (desc_.evoutroutea_address != 0) ranges_[4] = {desc_.evoutroutea_address, desc_.evoutroutea_address};
    }

    void set_pin_mux(PinMux* pm) noexcept { 
        pin_mux_ = pm; 
    }

    [[nodiscard]] std::string_view name() const noexcept override { return "PORTMUX"; }

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override {
        size_t count = 0;
        while (count < ranges_.size() && ranges_[count].begin != 0) count++;
        return {ranges_.data(), count};
    }

    u8 read(u16 address) noexcept override {
        if (address == desc_.twispiroutea_address) return twispiroutea_;
        if (address == desc_.usartroutea_address) return usartroutea_;
        if (address == desc_.evoutroutea_address) return evoutroutea_;
        if (address == desc_.tcaroutea_address) return tcaroutea_;
        if (address == desc_.tcbroutea_address) return tcbroutea_;
        return 0;
    }

    void write(u16 address, u8 value) noexcept override {
        u8 old_tca = tcaroutea_;
        u8 old_tcb = tcbroutea_;
        u8 old_usart = usartroutea_;
        u8 old_twispi = twispiroutea_;

        if (address == desc_.twispiroutea_address) twispiroutea_ = value;
        else if (address == desc_.usartroutea_address) usartroutea_ = value;
        else if (address == desc_.evoutroutea_address) evoutroutea_ = value;
        else if (address == desc_.tcaroutea_address) tcaroutea_ = value;
        else if (address == desc_.tcbroutea_address) tcbroutea_ = value;

        if (tcaroutea_ != old_tca || tcbroutea_ != old_tcb || usartroutea_ != old_usart || twispiroutea_ != old_twispi) {
             if (pin_mux_) {
                 // Release TCA0 old pins
                 u8 old_tca_port = old_tca & 0x07;
                 if (old_tca_port < 6) {
                     for (u8 i = 0; i < 6; ++i) pin_mux_->release_pin(old_tca_port, i, PinOwner::timer);
                 }

                 // Release TCB old pins (they are independent, easier to just release all TCB possible pins)
                 // TCB0/1 are PORTA/PORTF, TCB2 is PORTC/PORTB, TCB3 is PORTB/PORTC
                 // For now, simpler to just let observers handle it if they knew, 
                 // but since they don't, we do a targeted release.
                 auto release_tcb = [&](u8 idx) {
                    u8 old_alt = (old_tcb >> idx) & 0x01;
                    u8 p, b;
                    if (idx == 0) { p = (old_alt == 0) ? 0 : 5; b = (old_alt == 0) ? 2: 4; }
                    else if (idx == 1) { p = (old_alt == 0) ? 0 : 5; b = (old_alt == 0) ? 3: 5; }
                    else if (idx == 2) { p = (old_alt == 0) ? 2 : 1; b = (old_alt == 0) ? 0: 4; }
                    else if (idx == 3) { p = (old_alt == 0) ? 1 : 2; b = (old_alt == 0) ? 5: 1; }
                    else return;
                    pin_mux_->release_pin(p, b, PinOwner::timer);
                 };
                  if (tcbroutea_ != old_tcb) {
                      for (u8 i = 0; i < 4; ++i) release_tcb(i);
                  }

                  if (usartroutea_ != old_usart) {
                      for (u8 i = 0; i < 4; ++i) {
                          auto old_txd = get_usart_txd_pin(i, old_usart);
                          auto old_rxd = get_usart_rxd_pin(i, old_usart);
                          if (old_txd.first < 6) pin_mux_->release_pin(old_txd.first, old_txd.second, PinOwner::uart);
                          if (old_rxd.first < 6) pin_mux_->release_pin(old_rxd.first, old_rxd.second, PinOwner::uart);
                      }
                  }
              }

             for (auto* observer : observers_) {
                 observer->on_routing_changed();
             }
        }
    }

    void tick(u64) noexcept override {}

    void reset() noexcept override {
        twispiroutea_ = 0;
        usartroutea_ = 0;
        evoutroutea_ = 0;
        tcaroutea_ = 0;
        tcbroutea_ = 0;
    }

    void register_port(u16 port_addr, u8 port_index) {
        port_addrs_[port_index] = port_addr;
        port_indices_[port_addr] = port_index;
    }

    void add_observer(IRoutingObserver* observer) noexcept {
        if (observer) observers_.push_back(observer);
    }

    void drive_tca0_wo(u8 wo_index, bool level, bool enabled) noexcept {
        if (!pin_mux_ || wo_index >= 6) return;
        u8 port_idx = tcaroutea_ & 0x07;
        if (port_idx >= 6) return;

        if (enabled) {
            pin_mux_->claim_pin(port_idx, wo_index, PinOwner::timer);
            pin_mux_->update_pin(port_idx, wo_index, PinOwner::timer, true, level, false);
        } else {
            pin_mux_->release_pin(port_idx, wo_index, PinOwner::timer);
        }
    }

    void drive_usart_tx(u8 usart_index, PinLevel level, bool enabled = true) noexcept {
        if (!pin_mux_ || usart_index >= 4) return;
        auto [port_idx, pin_idx] = get_usart_txd_pin(usart_index, usartroutea_);
        if (port_idx >= 6) return;

        if (enabled) {
            pin_mux_->claim_pin(port_idx, pin_idx, PinOwner::uart);
            pin_mux_->update_pin(port_idx, pin_idx, PinOwner::uart, true, level == PinLevel::high, false);
        } else {
            pin_mux_->release_pin(port_idx, pin_idx, PinOwner::uart);
        }
    }

    [[nodiscard]] PinLevel get_usart_rx_level(u8 usart_index) const noexcept {
        if (!pin_mux_ || usart_index >= 4) return PinLevel::high;
        auto [port_idx, pin_idx] = get_usart_rxd_pin(usart_index, usartroutea_);
        if (port_idx >= 6) return PinLevel::high;

        return pin_mux_->get_state(port_idx, pin_idx).drive_level ? PinLevel::high : PinLevel::low;
    }

    void drive_tcb_wo(u8 tcb_index, bool level, bool enabled) noexcept {
        if (!pin_mux_ || tcb_index >= 4) return;
        auto [port_idx, pin_idx] = get_tcb_pin(tcb_index);
        
        if (enabled) {
            pin_mux_->claim_pin(port_idx, pin_idx, PinOwner::timer);
            pin_mux_->update_pin(port_idx, pin_idx, PinOwner::timer, true, level, false);
        } else {
            pin_mux_->release_pin(port_idx, pin_idx, PinOwner::timer);
        }
    }

    void drive_spi_mosi(u8 spi_index, PinLevel level, bool enabled = true) noexcept {
        if (!pin_mux_ || spi_index >= 2) return;
        auto [port_idx, pin_idx] = get_spi_pin(&SpiRouteGroup::mosi, spi_index);
        if (port_idx >= 6) return;
        if (enabled) {
            pin_mux_->claim_pin(port_idx, pin_idx, PinOwner::spi);
            pin_mux_->update_pin(port_idx, pin_idx, PinOwner::spi, true, level == PinLevel::high, false);
        } else {
            pin_mux_->release_pin(port_idx, pin_idx, PinOwner::spi);
        }
    }

    void drive_spi_sck(u8 spi_index, PinLevel level, bool enabled = true) noexcept {
        if (!pin_mux_ || spi_index >= 2) return;
        auto [port_idx, pin_idx] = get_spi_pin(&SpiRouteGroup::sck, spi_index);
        if (port_idx >= 6) return;
        if (enabled) {
            pin_mux_->claim_pin(port_idx, pin_idx, PinOwner::spi);
            pin_mux_->update_pin(port_idx, pin_idx, PinOwner::spi, true, level == PinLevel::high, false);
        } else {
            pin_mux_->release_pin(port_idx, pin_idx, PinOwner::spi);
        }
    }

    void drive_spi_ss(u8 spi_index, PinLevel level, bool enabled = true) noexcept {
        if (!pin_mux_ || spi_index >= 2) return;
        auto [port_idx, pin_idx] = get_spi_pin(&SpiRouteGroup::ss, spi_index);
        if (port_idx >= 6) return;
        if (enabled) {
            pin_mux_->claim_pin(port_idx, pin_idx, PinOwner::spi);
            pin_mux_->update_pin(port_idx, pin_idx, PinOwner::spi, true, level == PinLevel::high, false);
        } else {
            pin_mux_->release_pin(port_idx, pin_idx, PinOwner::spi);
        }
    }

    [[nodiscard]] PinLevel get_spi_miso_level(u8 spi_index) const noexcept {
        if (!pin_mux_ || spi_index >= 2) return PinLevel::high;
        auto [port_idx, pin_idx] = get_spi_pin(&SpiRouteGroup::miso, spi_index);
        if (port_idx >= 6) return PinLevel::high;
        return pin_mux_->get_state(port_idx, pin_idx).drive_level ? PinLevel::high : PinLevel::low;
    }

    void drive_twi_sda(u8 twi_index, PinLevel level, bool enabled = true) noexcept {
        if (!pin_mux_ || twi_index >= 2) return;
        auto [port_idx, pin_idx] = get_twi_pin(&TwiRouteGroup::sda, twi_index);
        if (port_idx >= 6) return;
        if (enabled) {
            pin_mux_->claim_pin(port_idx, pin_idx, PinOwner::twi);
            pin_mux_->update_pin(port_idx, pin_idx, PinOwner::twi, true, level == PinLevel::high, true); // Wired-AND (Open drain)
        } else {
            pin_mux_->release_pin(port_idx, pin_idx, PinOwner::twi);
        }
    }

    void drive_twi_scl(u8 twi_index, PinLevel level, bool enabled = true) noexcept {
        if (!pin_mux_ || twi_index >= 2) return;
        auto [port_idx, pin_idx] = get_twi_pin(&TwiRouteGroup::scl, twi_index);
        if (port_idx >= 6) return;
        if (enabled) {
            pin_mux_->claim_pin(port_idx, pin_idx, PinOwner::twi);
            pin_mux_->update_pin(port_idx, pin_idx, PinOwner::twi, true, level == PinLevel::high, true); // Wired-AND
        } else {
            pin_mux_->release_pin(port_idx, pin_idx, PinOwner::twi);
        }
    }

    [[nodiscard]] PinLevel get_twi_sda_level(u8 twi_index) const noexcept {
        if (!pin_mux_ || twi_index >= 2) return PinLevel::high;
        auto [port_idx, pin_idx] = get_twi_pin(&TwiRouteGroup::sda, twi_index);
        if (port_idx >= 6) return PinLevel::high;
        return pin_mux_->get_state(port_idx, pin_idx).drive_level ? PinLevel::high : PinLevel::low;
    }

    [[nodiscard]] PinLevel get_twi_scl_level(u8 twi_index) const noexcept {
        if (!pin_mux_ || twi_index >= 2) return PinLevel::high;
        auto [port_idx, pin_idx] = get_twi_pin(&TwiRouteGroup::scl, twi_index);
        if (port_idx >= 6) return PinLevel::high;
        return pin_mux_->get_state(port_idx, pin_idx).drive_level ? PinLevel::high : PinLevel::low;
    }

private:
    std::pair<u8, u8> get_tcb_pin(u8 tcb_index) const noexcept {
        u8 alt = (tcbroutea_ >> tcb_index) & 0x01;
        if (tcb_index == 0) return (alt == 0) ? std::make_pair(0, 2) : std::make_pair(5, 4);
        if (tcb_index == 1) return (alt == 0) ? std::make_pair(0, 3) : std::make_pair(5, 5);
        if (tcb_index == 2) return (alt == 0) ? std::make_pair(2, 0) : std::make_pair(1, 4);
        if (tcb_index == 3) return (alt == 0) ? std::make_pair(1, 5) : std::make_pair(2, 1);
        return {0, 0};
    }

    std::pair<u8, u8> get_spi_pin(PeripheralRoute SpiRouteGroup::* member, u8 spi_index) const noexcept {
        if (spi_index >= 2) return {0xFF, 0xFF};
        u8 val = (twispiroutea_ >> (spi_index * 2)) & 0x03;
        
        const auto& route = (desc_.spi[spi_index].*member);
        u16 addr = route.pin_addr[val];
        u8 bit = route.pin_bit[val];
        
        if (bit == 0xFFU || addr == 0) return {0xFF, 0xFF};
        auto it = port_indices_.find(addr);
        if (it == port_indices_.end()) return {0xFF, 0xFF};
        return {it->second, bit};
    }

    std::pair<u8, u8> get_twi_pin(PeripheralRoute TwiRouteGroup::* member, u8 twi_index) const noexcept {
        if (twi_index >= 2) return {0xFF, 0xFF};
        u8 val = (twispiroutea_ >> (twi_index * 2 + 4)) & 0x03;
        
        const auto& route = (desc_.twi[twi_index].*member);
        u16 addr = route.pin_addr[val];
        u8 bit = route.pin_bit[val];
        
        if (bit == 0xFFU || addr == 0) return {0xFF, 0xFF};
        auto it = port_indices_.find(addr);
        if (it == port_indices_.end()) return {0xFF, 0xFF};
        return {it->second, bit};
    }

    std::pair<u8, u8> get_usart_txd_pin(u8 usart_index, u8 route) const noexcept {
        if (usart_index >= 4) return {0xFF, 0xFF};
        u8 val = (route >> (usart_index * 2)) & 0x03;
        if (val >= 4) return {0xFF, 0xFF};
        
        u16 addr = desc_.usart[usart_index].txd.pin_addr[val];
        u8 bit = desc_.usart[usart_index].txd.pin_bit[val];
        
        if (bit == 0xFFU || addr == 0) return {0xFF, 0xFF};
        
        auto it = port_indices_.find(addr);
        if (it == port_indices_.end()) return {0xFF, 0xFF};
        return {it->second, bit};
    }

    std::pair<u8, u8> get_usart_rxd_pin(u8 usart_index, u8 route) const noexcept {
        if (usart_index >= 4) return {0xFF, 0xFF};
        u8 val = (route >> (usart_index * 2)) & 0x03;
        if (val >= 4) return {0xFF, 0xFF};
        
        u16 addr = desc_.usart[usart_index].rxd.pin_addr[val];
        u8 bit = desc_.usart[usart_index].rxd.pin_bit[val];
        
        if (bit == 0xFFU || addr == 0) return {0xFF, 0xFF};
        
        auto it = port_indices_.find(addr);
        if (it == port_indices_.end()) return {0xFF, 0xFF};
        return {it->second, bit};
    }

    const PortMuxDescriptor desc_;
    std::array<AddressRange, 6> ranges_ {};
    u8 twispiroutea_{0};
    u8 usartroutea_{0};
    u8 evoutroutea_{0};
    u8 tcaroutea_{0};
    u8 tcbroutea_{0};
    PinMux* pin_mux_{nullptr};
    std::map<u8, u16> port_addrs_;
    std::map<u16, u8> port_indices_;
    std::vector<IRoutingObserver*> observers_;
};

} // namespace vioavr::core
