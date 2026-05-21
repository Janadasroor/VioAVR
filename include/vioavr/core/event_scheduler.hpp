#pragma once

#include "vioavr/core/types.hpp"
#include <functional>
#include <vector>
#include <algorithm>

namespace vioavr::core {

using EventCallback = void (*)(u64 current_cycle, void* param);

struct ScheduledEvent {
    u64 target_cycle;
    EventCallback callback;
    void* param;
    u32 priority; // Lower is higher priority

    bool operator>(const ScheduledEvent& other) const {
        if (target_cycle != other.target_cycle) {
            return target_cycle > other.target_cycle;
        }
        return priority > other.priority;
    }
};

/**
 * @brief Manages a priority queue of future events to avoid per-cycle polling.
 */
class EventScheduler {
public:
    EventScheduler() = default;

    void reset() {
        events_.clear();
        current_cycle_ = 0;
    }

    void schedule(u64 delay, EventCallback callback, void* param, u32 priority = 100) {
        u64 target = current_cycle_ + delay;
        events_.push_back({target, callback, param, priority});
        std::push_heap(events_.begin(), events_.end(), std::greater<ScheduledEvent>());
    }

    void cancel(EventCallback callback, void* param) {
        auto it = std::remove_if(events_.begin(), events_.end(), [&](const ScheduledEvent& e) {
            return e.callback == callback && e.param == param;
        });
        if (it != events_.end()) {
            events_.erase(it, events_.end());
            std::make_heap(events_.begin(), events_.end(), std::greater<ScheduledEvent>());
        }
    }

    /**
     * @brief Advance time and process events.
     * @return Cycles until the next event.
     */
    u64 advance_to(u64 target_cycle, const std::function<void(u64)>& pre_event_hook = nullptr) {
        while (!events_.empty() && events_.front().target_cycle <= target_cycle) {
            ScheduledEvent event = events_.front();
            std::pop_heap(events_.begin(), events_.end(), std::greater<ScheduledEvent>());
            events_.pop_back();

            if (pre_event_hook) {
                pre_event_hook(event.target_cycle);
            }

            current_cycle_ = event.target_cycle;
            event.callback(current_cycle_, event.param);
        }
        current_cycle_ = target_cycle;
        return next_event_delay();
    }

    [[nodiscard]] u64 next_event_delay() const {
        if (events_.empty()) return 1000; // Arbitrary large quantum
        if (events_.front().target_cycle <= current_cycle_) return 0;
        return events_.front().target_cycle - current_cycle_;
    }

    [[nodiscard]] u64 current_cycle() const { return current_cycle_; }

private:
    std::vector<ScheduledEvent> events_;
    u64 current_cycle_ {0};
};

} // namespace vioavr::core
