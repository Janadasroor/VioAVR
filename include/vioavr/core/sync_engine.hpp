#pragma once

#include "vioavr/core/types.hpp"
#include <memory>
#include <vector>

namespace vioavr::core {

class SyncEngine {
public:
    virtual ~SyncEngine() = default;

    virtual void on_reset() noexcept = 0;
    virtual void on_cycles_advanced(u64 total_cycles, u64 delta_cycles) noexcept = 0;
    [[nodiscard]] virtual bool should_pause(u64 total_cycles) const noexcept = 0;
    virtual void wait_until_resumed() = 0;
    virtual void on_pin_state_changed(const PinStateChange& change) noexcept = 0;

    /**
     * @brief Signal the CPU to resume from a pause.
     */
    virtual void resume() = 0;

    /**
     * @brief Retrieve and clear accumulated pin changes since the last pause.
     */
    [[nodiscard]] virtual std::vector<PinStateChange> consume_pin_changes() = 0;
};

/**
 * @brief Factory function for creating the default sync engine with a fixed quantum.
 * @param quantum The number of cycles to execute between synchronization points.
 * @return A unique_ptr to the created SyncEngine.
 */
[[nodiscard]] std::unique_ptr<SyncEngine> create_fixed_quantum_sync_engine(u64 quantum);

}  // namespace vioavr::core
