#pragma once

#include "vioavr/core/sync_engine.hpp"

#include <QObject>
#include <mutex>
#include <condition_variable>
#include <vector>

namespace vioavr::qt {

class QtSyncBridge final : public QObject, public core::SyncEngine {
    Q_OBJECT

public:
    explicit QtSyncBridge(QObject* parent = nullptr);

    void on_reset() noexcept override;
    void on_cycles_advanced(core::u64 total_cycles, core::u64 delta_cycles) noexcept override;
    [[nodiscard]] bool should_pause(core::u64 total_cycles) const noexcept override;
    void wait_until_resumed() override;
    void on_pin_state_changed(const core::PinStateChange& change) noexcept override;

    void resume() override;
    [[nodiscard]] std::vector<core::PinStateChange> consume_pin_changes() override;

signals:
    void cyclesAdvanced(qulonglong totalCycles, qulonglong deltaCycles);
    void pinStateChanged(quint8 portIndex, quint8 bitIndex, bool level, qulonglong cycleStamp);
    void resetObserved();
    void pausedChanged(bool paused);

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool resumed_ {false};
    std::vector<core::PinStateChange> pin_changes_;
};

}  // namespace vioavr::qt
