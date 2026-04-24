#pragma once

#include "vioavr/core/device.hpp"
#include "vioavr/core/io_peripheral.hpp"
#include <vector>
#include <array>
#include <string>
#include <string_view>

namespace vioavr::core {

class CanBus final : public IoPeripheral {
public:
    explicit CanBus(std::string_view name, const CanDescriptor& descriptor) noexcept;

    [[nodiscard]] std::string_view name() const noexcept override;
    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override;

    void reset() noexcept override;
    void tick(u64 elapsed_cycles) noexcept override;
    [[nodiscard]] u8 read(u16 address) noexcept override;
    void write(u16 address, u8 value) noexcept override;
    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override;
    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override;

    // Simulation hooks
    struct CanMessage {
        u32 id {0};
        bool ide {false};
        bool rtr {false};
        std::vector<u8> data;
    };

    void inject_message(const CanMessage& msg) noexcept;
    void simulate_bus_error() noexcept;

private:
    struct MessageObject {
        u8 canstmob {};
        u8 cancdmob {};
        u8 canidtags[4] {};
        u8 canidmasks[4] {};
        u16 canstm {};
        std::array<u8, 8> data {};
    };

    std::string name_;
    CanDescriptor desc_;
    std::vector<AddressRange> ranges_;

    // General Registers
    u8 cangcon_ {};
    u8 cangsta_ {};
    u8 cangit_ {};
    u8 cangie_ {};
    u8 canen1_ {};
    u8 canen2_ {};
    u8 canie1_ {};
    u8 canie2_ {};
    u8 cansit1_ {};
    u8 cansit2_ {};
    u8 canbt1_ {};
    u8 canbt2_ {};
    u8 canbt3_ {};
    u8 cantcon_ {};
    u16 cantim_ {};
    u16 canttc_ {};
    u16 cantec_ {};
    u16 canrec_ {};
    u8 canhpmob_ {};
    u8 canpage_ {};

    std::vector<MessageObject> mobs_;

    bool canit_pending_ {false};
    bool ovrit_pending_ {false};
    u32 timer_prescaler_cycles_ {};
    u8 timer_temp_ {0U};

    u64 tx_wait_cycles_ {0};
    int current_tx_mob_ {-1};

    void evaluate_interrupts() noexcept;
    void evaluate_error_state() noexcept;
    void find_high_priority_mob() noexcept;
    void receive_message(const CanMessage& msg) noexcept;
};

}  // namespace vioavr::core
