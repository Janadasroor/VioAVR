#pragma once
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class MemoryBus;
class PortMux;
class PinMux;

class Awex final : public IoPeripheral {
public:
    explicit Awex(std::string name, const AwexDescriptor& desc) noexcept;

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }
    void set_port_mux(PortMux* pm) noexcept { port_mux_ = pm; }
    void set_pin_mux(PinMux* pm) noexcept { pin_mux_ = pm; }
    void set_port_index(u8 idx) noexcept { port_index_ = idx; }

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    /// Called by companion TC to set raw WO levels
    void set_wo_level(u8 channel, bool level) noexcept;

    /// Get AWEX-processed levels (with dead-time applied)
    [[nodiscard]] bool get_dh_level(u8 channel) const noexcept;
    [[nodiscard]] bool get_dl_level(u8 channel) const noexcept;

    /// Drive DH/DL outputs to port pins via PortMux
    void drive_outputs(u8 port_override = 0xFF) noexcept;

    [[nodiscard]] ClockDomain clock_domain() const noexcept override { return ClockDomain::io; }

private:
    [[nodiscard]] bool dt_enabled_for_channel(u8 ch) const noexcept;

    std::string name_;
    const AwexDescriptor desc_;
    std::array<AddressRange, 8> ranges_;
    MemoryBus* bus_ {};
    PortMux* port_mux_ {};
    PinMux* pin_mux_ {};
    u8 port_index_ {0};

    u8 ctrl_ {};
    u8 fdemask_ {};
    u8 fdctrl_ {};
    u8 status_ {};
    u8 dtboth_ {};
    u8 dtbothbuf_ {};
    u8 dtls_ {};
    u8 dths_ {};
    u8 dtlsbuf_ {};
    u8 dthsbuf_ {};
    u8 outoven_ {};

    u8 wo_levels_ {};
    bool dt_pending_[4] {};
    u16 dt_counters_[4] {};
};

} // namespace vioavr::core
