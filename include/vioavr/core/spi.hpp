#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <array>
#include <string>

namespace vioavr::core {

class PinMux;

class Spi final : public IoPeripheral {
public:
    explicit Spi(std::string_view name, const SpiDescriptor& descriptor,
                 PinMux* pin_mux = nullptr) noexcept;

    void set_pin_mux(PinMux* pin_mux) noexcept { pin_mux_ = pin_mux; }

    void set_memory_bus(MemoryBus* bus) noexcept override { bus_ = bus; }

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    // A slave is clocked by SCK edges from the pin, so tick() must run.
    [[nodiscard]] bool wants_tick() const noexcept override {
        return (spcr_ & desc_.spe_mask) != 0U && (spcr_ & desc_.mstr_mask) == 0U;
    }
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;
    [[nodiscard]] bool supports_interrupt_mask() const noexcept override { return true; }

    [[nodiscard]] ClockDomain clock_domain() const noexcept override;

    // Host-side API for SPI simulation
    void inject_miso_byte(u8 value) noexcept;
    void trigger_slave_transfer(u32 sck_bit_cycles = 2) noexcept;
    [[nodiscard]] u8 last_transmitted_byte() const noexcept;
    [[nodiscard]] bool busy() const noexcept;
    void on_event(u64 cycle) noexcept;
    void on_edge(u64 cycle) noexcept;
    // SS driven low while master: clears MSTR, sets SPIF (datasheet §18.3.2).
    void on_ss_asserted() noexcept;
    [[nodiscard]] u8 spi_mode() const noexcept { return static_cast<u8>((spcr_ >> 2U) & 0x03U); }

 private:
    void complete_transfer() noexcept;
    void update_interrupt_pending() noexcept;
    // Bit-accurate master transfer helpers (pin shaping).
    [[nodiscard]] bool pins_mapped() const noexcept;
    [[nodiscard]] u32 sck_divisor() const noexcept;
    void start_stepped_transfer(u8 data) noexcept;
    void drive_sck(bool level) noexcept;
    void drive_mosi(bool level) noexcept;
    void drive_ss(bool level) noexcept;
    [[nodiscard]] bool sample_miso_wire(u8 wire_pos) noexcept;
    void claim_transfer_pins() noexcept;
    void release_transfer_pins() noexcept;
    void abort_transfer() noexcept;

    std::string name_;
    SpiDescriptor desc_;
    MemoryBus* bus_ {};
    PinMux* pin_mux_ {};
    std::array<AddressRange, 1> ranges_;

    u8 spcr_ {};
    u8 spsr_ {};
    u8 spdr_ {};
    u8 shift_register_ {};
    u8 miso_buffer_ {};
    
    u32 transfer_cycles_left_ {};
    // Stepped (pin-shaping) master transfer state.
    bool transfer_in_progress_ {};
    u8 edges_done_ {};
    u8 rx_byte_ {};
    u32 half_bit_cycles_ {};

    // Functional slave: shifted on real SCK edges from the pin.
    bool slave_active_ {};
    bool slave_sck_last_ {};
    u8 slave_bits_ {};
    u8 slave_rx_ {};
    u8 slave_tx_ {};
    bool slave_miso_ {true};
    bool slave_miso_claimed_ {};
    bool interrupt_pending_ {};
    bool spsr_read_since_spif_ {};

    void slave_service_pins() noexcept;
    void update_active_state() noexcept;
    void slave_drive_miso() noexcept;
    void slave_release_miso() noexcept;
    [[nodiscard]] bool power_reduction_enabled() const noexcept;
};

}  // namespace vioavr::core
