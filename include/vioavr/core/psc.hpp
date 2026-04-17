#pragma once
#include "vioavr/core/io_peripheral.hpp"
#include "vioavr/core/device.hpp"
#include <string_view>
#include <array>
#include <vector>

namespace vioavr::core {

class Psc final : public IoPeripheral {
public:
    Psc(std::string_view name, const PscDescriptor& desc);
    ~Psc() override = default;

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    
    void connect_adc_trigger(class Adc& adc) noexcept { adc_trigger_ = &adc; }

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& req) const noexcept override;
    bool consume_interrupt_request(InterruptRequest& req) noexcept override;

private:
    std::string_view name_;
    const PscDescriptor& desc_;
    std::vector<AddressRange> ranges_;
    
    // 12-bit Counter and Registers
    u16 counter_ {0};
    u8 pctl_ {0};
    u8 psoc_ {0};
    u8 pconf_ {0};
    u8 pim_ {0};
    u8 pifr_ {0};
    u16 picr_ {0};
    
    u16 ocrsa_ {0};
    u16 ocrra_ {0};
    u16 ocrsb_ {0};
    u16 ocrrb_ {0};
    
    u8 pfrc0a_ {0};
    u8 pfrc0b_ {0};

    // Temp for 16-bit access
    u8 temp_ {0};

    class Adc* adc_trigger_ {nullptr};

    void evaluate_interrupts() noexcept;
};

} // namespace vioavr::core
