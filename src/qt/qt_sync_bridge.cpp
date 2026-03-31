#include "vioavr/qt/qt_sync_bridge.hpp"

namespace vioavr::qt {

QtSyncBridge::QtSyncBridge(QObject* parent) : QObject(parent) {}

void QtSyncBridge::on_reset() noexcept
{
    {
        std::lock_guard lock(mutex_);
        resumed_ = false;
        pin_changes_.clear();
    }
    emit resetObserved();
}

void QtSyncBridge::on_cycles_advanced(const core::u64 total_cycles, const core::u64 delta_cycles) noexcept
{
    emit cyclesAdvanced(total_cycles, delta_cycles);
}

bool QtSyncBridge::should_pause(const core::u64 total_cycles) const noexcept
{
    (void)total_cycles;
    // By default, the bridge doesn't define its own pause points.
    // An external policy or UI interaction would trigger pauses.
    return false;
}

void QtSyncBridge::wait_until_resumed()
{
    emit pausedChanged(true);
    
    std::unique_lock lock(mutex_);
    if (resumed_) {
        resumed_ = false;
        emit pausedChanged(false);
        return;
    }

    cv_.wait(lock, [this] { return resumed_; });
    resumed_ = false;
    
    emit pausedChanged(false);
}

void QtSyncBridge::on_pin_state_changed(const core::PinStateChange& change) noexcept
{
    {
        std::lock_guard lock(mutex_);
        pin_changes_.push_back(change);
    }
    emit pinStateChanged(change.port_index, change.bit_index, change.level, change.cycle_stamp);
}

void QtSyncBridge::resume()
{
    {
        std::lock_guard lock(mutex_);
        resumed_ = true;
    }
    cv_.notify_one();
}

std::vector<core::PinStateChange> QtSyncBridge::consume_pin_changes()
{
    std::lock_guard lock(mutex_);
    std::vector<core::PinStateChange> changes;
    std::swap(changes, pin_changes_);
    return changes;
}

}  // namespace vioavr::qt
