#pragma once

#include "vioavr/core/types.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <vector>
#include <functional>

namespace vioavr::core {

class MemoryBus;

/**
 * @brief Event System (EVSYS) for modern AVR devices.
 * Routes hardware signals (generators) to users without CPU.
 */
class EventSystem : public IoPeripheral {
public:
    explicit EventSystem(const EvsysDescriptor& desc);

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;

    [[nodiscard]] std::string_view name() const noexcept override { return "EVSYS"; }
    u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;
    
    // Core Logic
    void trigger_event(u8 generator_id) noexcept;
    
    // Callbacks for users
    using EventCallback = std::function<void()>;
    void register_user_callback(u8 user_index, EventCallback callback);

    [[nodiscard]] u16 users_base() const noexcept { return desc_.users_address; }

private:
    const EvsysDescriptor desc_;

    u8 strobe_ {0x00};
    std::vector<u8> channels_;
    std::vector<u8> users_;

    std::array<AddressRange, 3> ranges_ {};
    
    // Maps user index -> callback
    std::vector<EventCallback> callbacks_;
};

} // namespace vioavr::core
