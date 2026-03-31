#pragma once

#include "vioavr/core/types.hpp"

namespace vioavr::core {

/**
 * @brief Strategy for deciding when the CPU should yield to the external simulator.
 */
class ISyncPolicy {
public:
    virtual ~ISyncPolicy() = default;

    /**
     * @brief Called when the CPU starts or resets.
     */
    virtual void reset() noexcept = 0;

    /**
     * @brief Determines if the CPU should pause at the current cycle count.
     * @param total_cycles Current total cycles executed by the CPU.
     * @return true if the CPU should enter a paused state and wait.
     */
    [[nodiscard]] virtual bool should_pause(u64 total_cycles) const noexcept = 0;

    /**
     * @brief Called after a pause has been released.
     * @param total_cycles Current total cycles.
     */
    virtual void on_resumed(u64 total_cycles) noexcept = 0;
};

/**
 * @brief A policy that pauses the CPU every N cycles (the "quantum").
 * 
 * This is useful for fixed-step co-simulation where the external simulator
 * and the AVR core exchange data at regular intervals.
 */
class FixedQuantumSyncPolicy final : public ISyncPolicy {
public:
    explicit FixedQuantumSyncPolicy(u64 quantum_cycles) noexcept 
        : quantum_(quantum_cycles) {}

    void reset() noexcept override {
        next_sync_cycle_ = quantum_;
    }

    [[nodiscard]] bool should_pause(u64 total_cycles) const noexcept override {
        return total_cycles >= next_sync_cycle_;
    }

    void on_resumed(u64 total_cycles) noexcept override {
        // Advance to the next quantum boundary
        while (next_sync_cycle_ <= total_cycles) {
            next_sync_cycle_ += quantum_;
        }
    }

private:
    u64 quantum_;
    u64 next_sync_cycle_ {0};
};

/**
 * @brief A policy that pauses the CPU when it reaches an externally defined barrier.
 * 
 * This is useful for simulations where the next sync point is dynamically 
 * determined by the co-simulator based on transient analysis results.
 */
class ExternalBarrierSyncPolicy final : public ISyncPolicy {
public:
    void reset() noexcept override {
        barrier_cycle_ = 0;
    }

    [[nodiscard]] bool should_pause(u64 total_cycles) const noexcept override {
        return total_cycles >= barrier_cycle_;
    }

    void on_resumed(u64 /*total_cycles*/) noexcept override {
        // We stay at the barrier until it is moved forward
    }

    /**
     * @brief Advance the barrier to a new cycle count.
     * @param cycle The absolute cycle count where the next sync should occur.
     */
    void set_barrier(u64 cycle) noexcept {
        barrier_cycle_ = cycle;
    }

private:
    u64 barrier_cycle_ {0};
};

}  // namespace vioavr::core
