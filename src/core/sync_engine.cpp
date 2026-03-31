#include "vioavr/core/sync_engine.hpp"
#include "vioavr/core/sync_policy.hpp"
#include "vioavr/core/logger.hpp"

#include <mutex>
#include <condition_variable>
#include <memory>
#include <vector>
#include <string>

namespace vioavr::core {

/**
 * @brief Standard implementation of SyncEngine that coordinates with a SyncPolicy.
 */
class DefaultSyncEngine final : public SyncEngine {
public:
    explicit DefaultSyncEngine(std::unique_ptr<ISyncPolicy> policy) noexcept 
        : policy_(std::move(policy)) {}

    void on_reset() noexcept override {
        std::lock_guard lock(mutex_);
        if (policy_) {
            policy_->reset();
        }
        resumed_ = false;
        total_cycles_ = 0;
        pin_changes_.clear();
    }

    void on_cycles_advanced(u64 total_cycles, u64 /*delta_cycles*/) noexcept override {
        std::lock_guard lock(mutex_);
        total_cycles_ = total_cycles;
    }

    [[nodiscard]] bool should_pause(u64 total_cycles) const noexcept override {
        std::lock_guard lock(mutex_);
        return policy_ ? policy_->should_pause(total_cycles) : false;
    }

    void wait_until_resumed() override {
        std::unique_lock lock(mutex_);
        
        if (resumed_) {
            resumed_ = false;
            if (policy_) {
                policy_->on_resumed(total_cycles_);
            }
            return;
        }

        cv_.wait(lock, [this] { return resumed_; });
        resumed_ = false;

        if (policy_) {
            policy_->on_resumed(total_cycles_);
        }
    }

    void on_pin_state_changed(const PinStateChange& change) noexcept override {
        std::lock_guard lock(mutex_);
        pin_changes_.push_back(change);
    }

    void resume() override {
        {
            std::lock_guard lock(mutex_);
            resumed_ = true;
        }
        cv_.notify_one();
    }

    /**
     * @brief Retrieve and clear accumulated pin changes since the last pause.
     */
    [[nodiscard]] std::vector<PinStateChange> consume_pin_changes() override {
        std::lock_guard lock(mutex_);
        std::vector<PinStateChange> changes;
        std::swap(changes, pin_changes_);
        return changes;
    }

private:
    std::unique_ptr<ISyncPolicy> policy_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool resumed_ {true};
    u64 total_cycles_ {0};
    std::vector<PinStateChange> pin_changes_;
};

/**
 * @brief Factory function for creating the default sync engine with a fixed quantum.
 */
std::unique_ptr<SyncEngine> create_fixed_quantum_sync_engine(u64 quantum) {
    return std::make_unique<DefaultSyncEngine>(
        std::make_unique<FixedQuantumSyncPolicy>(quantum)
    );
}

}  // namespace vioavr::core
