#pragma once

#include "vioavr/core/io_peripheral.hpp"
#include <vector>

namespace vioavr::core {

class AvrCpu;

class DeviceDescriptor;

/**
 * @brief MCU Status Register flags (MCUSR) for ATmega328P.
 *
 * Per datasheet Table 9-2:
 * - PORF (bit 0): Power-On Reset Flag
 * - EXTRF (bit 1): External Reset Flag
 * - BORF (bit 2): Brown-Out Reset Flag
 * - WDRF (bit 3): Watchdog System Reset Flag
 */
enum McusrFlags : u8 {
    MCUSR_PORF  = 0x01U,
    MCUSR_EXTRF = 0x02U,
    MCUSR_BORF  = 0x04U,
    MCUSR_WDRF  = 0x08U
};

/**
 * @brief Reset cause for ATmega328P.
 */
enum class ResetCause : u8 {
    power_on = 0,
    external = 1,
    brown_out = 2,
    watchdog = 3
};

/**
 * @brief Sleep modes supported by ATmega328P.
 *
 * Per datasheet Table 10-1:
 * - Idle: CPU clock stops, peripherals run, all interrupts can wake
 * - ADC Noise Reduction: Same as Idle but only ADC/Timer2/TWI/WDT can wake
 * - Power Down: External oscillator stops, only INTx/PCINT/TWI/T2/ADC/WDT can wake
 * - Power Save: Same as Power Down but Timer2 runs async (requires TOSC1/2)
 * - Standby: Same as Power Down but oscillator keeps running
 * - Extended Standby: Same as Power Save but oscillator keeps running
 */
enum class SleepMode : u8 {
    idle = 0,
    adc_noise_reduction = 1,
    power_down = 2,
    power_save = 3,
    reserved4 = 4,
    reserved5 = 5,
    standby = 6,
    extended_standby = 7
};

/**
 * @brief Bridge peripheral that maps CPU internal registers (SREG, SP) and SMCR into the I/O space.
 */
class CpuControl final : public IoPeripheral {
public:
    explicit CpuControl(AvrCpu& cpu, const DeviceDescriptor& desc) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] SleepMode current_sleep_mode() const noexcept { return sleep_mode_; }
    [[nodiscard]] bool sleep_enabled() const noexcept { return sleep_enabled_; }
    void enter_sleep() noexcept { sleeping_ = true; }
    void exit_sleep() noexcept { sleeping_ = false; }
    [[nodiscard]] bool is_sleeping() const noexcept { return sleeping_; }

    [[nodiscard]] u8 mcusr() const noexcept { return mcusr_; }
    [[nodiscard]] u8 mcucr() const noexcept { return mcucr_; }
    void set_reset_cause(ResetCause cause) noexcept;
    void clear_mcusr() noexcept { mcusr_ = 0U; }

private:
    u8 spmcsr_ {};
    u8 smcr_ {};
    u8 mcusr_ {};
    u8 mcucr_ {};
    u8 prr_ {};
    u8 prr0_ {};
    u8 prr1_ {};
    SleepMode sleep_mode_ {SleepMode::idle};
    bool sleep_enabled_ {false};
    bool sleeping_ {false};
    AvrCpu& cpu_;
    std::vector<AddressRange> ranges_;
};

}  // namespace vioavr::core
