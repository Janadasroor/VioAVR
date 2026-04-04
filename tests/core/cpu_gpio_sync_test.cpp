#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/gpio_port.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/sync_engine.hpp"
#include "vioavr/core/devices/atmega328.hpp"

#include <array>
#include <vector>

namespace {

using namespace vioavr::core;

constexpr u16 encode_ldi(const u8 destination, const u8 immediate) {
    return static_cast<u16>(0xE000U | ((static_cast<u16>(immediate & 0xF0U)) << 4U) | (static_cast<u16>(destination - 16U) << 4U) | (immediate & 0x0FU));
}

constexpr u16 encode_out(const u8 io_offset, const u8 source) {
    return static_cast<u16>(0xB800U | ((static_cast<u16>(source) & 0x1FU) << 4U) | ((static_cast<u16>(io_offset) & 0x30U) << 5U) | (io_offset & 0x0FU));
}

constexpr u16 encode_in(const u8 destination, const u8 io_offset) {
    return static_cast<u16>(0xB000U | ((static_cast<u16>(destination) & 0x1FU) << 4U) | ((static_cast<u16>(io_offset) & 0x30U) << 5U) | (io_offset & 0x0FU));
}

class RecordingSyncEngine final : public SyncEngine {
public:
    void on_reset() noexcept override {}
    void on_cycles_advanced(u64, u64) noexcept override {}
    [[nodiscard]] bool should_pause(u64) const noexcept override { return false; }
    void wait_until_resumed() override {}

    void on_pin_state_changed(const PinStateChange& change) noexcept override {
        if (count < changes.size()) {
            changes[count] = change;
            ++count;
        }
    }

    void resume() override {}
    [[nodiscard]] std::vector<PinStateChange> consume_pin_changes() override { return {}; }

    std::array<PinStateChange, 8> changes {};
    std::size_t count {};
};

void step_to(AvrCpu& cpu, u32 target_pc) { while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) { cpu.step(); } }
} // namespace

TEST_CASE("CPU and GPIO Synchronization via SyncEngine Test")
{
    using namespace vioavr::core;
    using namespace vioavr::core::devices;

    constexpr auto pinb_addr = static_cast<u16>(0x23U);
    constexpr auto ddrb_addr = static_cast<u16>(0x24U);
    constexpr auto portb_addr = static_cast<u16>(0x25U);

    MemoryBus bus {atmega328};
    GpioPort port_b {"PORTB", pinb_addr, ddrb_addr, portb_addr};
    bus.attach_peripheral(port_b);
    AvrCpu cpu {bus};
    RecordingSyncEngine sync;
    cpu.set_sync_engine(&sync);

    bus.load_image(HexImage {
        .flash_words = {
            encode_ldi(16U, 0x03U),
            encode_out(0x04U, 16U),                      // 1: DDRB = 0x03 (PB0, PB1 outputs)
            encode_ldi(16U, 0x01U),
            encode_out(0x05U, 16U),                      // 3: PORTB = 0x01 (PB0 High)
            encode_ldi(16U, 0x03U),
            encode_out(0x03U, 16U),                      // 5: PINB = 0x03 (Toggle PB0/PB1) -> PB0 Low, PB1 High
            encode_in(17U, 0x03U),                       // 6: r17 = PINB
            0x0000U                                     // 7
        },
        .entry_word = 0U
    });

    cpu.reset();
    port_b.set_input_levels(0xA0U);

    SUBCASE("Execution and Pin Event Recording") {
        cpu.run(100);
        
        auto s = cpu.snapshot();
        // PINB reflects (Inputs & ~DDR) | (PORT & DDR)
        // PORTB was 0x01, then toggled by 0x03 -> 0x02
        // PINB = (0xA0 & 0xFC) | (0x02 & 0x03) = 0xA0 | 0x02 = 0xA2
        CHECK(s.gpr[17] == 0xA2U);
        CHECK(port_b.port_register() == 0x02U);
        
        // Pin events should have triggered for:
        // 1. PB0 High (at instruction 3)
        // 2. PB0 Low (at instruction 5)
        // 3. PB1 High (at instruction 5)
        CHECK(sync.count == 3U);
        
        // Check PB0 High event
        CHECK(sync.changes[0].bit_index == 0U);
        CHECK(sync.changes[0].level == true);
        
        // Check PB0 Low event
        CHECK(sync.changes[1].bit_index == 0U);
        CHECK(sync.changes[1].level == false);
        
        // Check PB1 High event
        CHECK(sync.changes[2].bit_index == 1U);
        CHECK(sync.changes[2].level == true);
        
        // Cycle stamps should be monotonic
        CHECK(sync.changes[0].cycle_stamp <= sync.changes[1].cycle_stamp);
        CHECK(sync.changes[1].cycle_stamp <= sync.changes[2].cycle_stamp);
    }
}
