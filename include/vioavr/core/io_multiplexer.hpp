#pragma once

#include <vioavr/core/io_peripheral.hpp>
#include <vector>
#include <string>

namespace vioavr::core {

class IoMultiplexer final : public IoPeripheral {
public:
    explicit IoMultiplexer(std::string_view name) : name_(name) {}

    void add(IoPeripheral* target) {
        if (target) {
            targets_.push_back(target);
        }
    }

    [[nodiscard]] std::string_view name() const noexcept override { return name_; }

    [[nodiscard]] std::span<const AddressRange> mapped_ranges() const noexcept override {
        return {};
    }

    void reset() noexcept override {
        for (auto* target : targets_) {
            target->reset();
        }
    }

    void tick(u64 elapsed_cycles) noexcept override {
        for (auto* target : targets_) {
            target->tick(elapsed_cycles);
        }
    }

    [[nodiscard]] bool wants_tick() const noexcept override {
        for (auto* target : targets_) {
            if (target->wants_tick()) return true;
        }
        return false;
    }

    [[nodiscard]] u8 read(u16 address) noexcept override {
        u8 result = 0;
        for (auto* target : targets_) {
            result |= target->read(address);
        }
        return result;
    }

    void write(u16 address, u8 value) noexcept override {
        for (auto* target : targets_) {
            target->write(address, value);
        }
    }

    [[nodiscard]] bool pending_interrupt_request(InterruptRequest& request) const noexcept override {
        for (auto* target : targets_) {
            if (target->pending_interrupt_request(request)) return true;
        }
        return false;
    }

    [[nodiscard]] bool consume_interrupt_request(InterruptRequest& request) noexcept override {
        for (auto* target : targets_) {
            if (target->consume_interrupt_request(request)) return true;
        }
        return false;
    }

private:
    std::string name_;
    std::vector<IoPeripheral*> targets_;
};

} // namespace vioavr::core
