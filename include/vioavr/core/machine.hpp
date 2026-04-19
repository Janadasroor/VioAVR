#pragma once

#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/analog_signal_bank.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/pin_mux.hpp"
#include "vioavr/core/pin_change_interrupt.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/cpu_control.hpp"
#include "vioavr/core/port_mux.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace vioavr::core {

/**
 * @brief Represents a complete AVR microcontroller instance with its peripherals.
 */
class Machine {
public:
    explicit Machine(const DeviceDescriptor& device);
    ~Machine();

    // Prevent copying
    Machine(const Machine&) = delete;
    Machine& operator=(const Machine&) = delete;

    /**
     * @brief Factory method to create a machine for a specific device name.
     */
    static std::unique_ptr<Machine> create_for_device(std::string_view name);

    /**
     * @brief Get the underlying CPU.
     */
    [[nodiscard]] AvrCpu& cpu() noexcept { return *cpu_; }
    [[nodiscard]] const AvrCpu& cpu() const noexcept { return *cpu_; }

    /**
     * @brief Get the memory bus.
     */
    [[nodiscard]] MemoryBus& bus() noexcept { return *bus_; }
    [[nodiscard]] const MemoryBus& bus() const noexcept { return *bus_; }

    /**
     * @brief Get the pin mux.
     */
    [[nodiscard]] PinMux& pin_mux() noexcept { return *pin_mux_; }

    /**
     * @brief Get the analog signal bank.
     */
    [[nodiscard]] AnalogSignalBank& analog_signal_bank() noexcept { return analog_signal_bank_; }
    [[nodiscard]] const AnalogSignalBank& analog_signal_bank() const noexcept { return analog_signal_bank_; }

    /**
     * @brief Reset the machine and all its peripherals.
     */
    void reset(ResetCause cause = ResetCause::power_on) noexcept;

    /**
     * @brief Step the simulation by one instruction or one sleep cycle.
     */
    void step() noexcept { cpu_->step(); }

    /**
     * @brief Run the simulation for a number of cycles.
     */
    void run(u64 cycles) noexcept { cpu_->run(cycles); }

    /**
     * @brief Get a GPIO port by name.
     */
    [[nodiscard]] GpioPort* get_port(std::string_view name) noexcept;

    /**
     * @brief Get all peripherals of a specific type.
     */
    template <typename T>
    [[nodiscard]] std::vector<T*> peripherals_of_type() noexcept {
        std::vector<T*> result;
        for (auto& p : owned_peripherals_) {
            if (auto* typed = dynamic_cast<T*>(p.get())) {
                result.push_back(typed);
            }
        }
        return result;
    }

private:
    void initialize_peripherals();
    void wire_peripherals();

    const DeviceDescriptor& device_;
    std::unique_ptr<MemoryBus> bus_;
    std::unique_ptr<AvrCpu> cpu_;
    std::unique_ptr<PinMux> pin_mux_;
    AnalogSignalBank analog_signal_bank_;
    PinChangeInterruptSharedState pcint_shared_state_ {};
    PortMux* port_mux_{nullptr};

    std::vector<std::unique_ptr<IoPeripheral>> owned_peripherals_;
    std::vector<GpioPort*> ports_;
};

} // namespace vioavr::core
