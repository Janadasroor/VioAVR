#pragma once

#include "vioavr/core/types.hpp"

#include <string_view>
#include <vector>
#include <algorithm>

namespace vioavr::core {

/**
 * @brief Interface for tracing core execution and state changes.
 * 
 * This hook allows external observers to monitor the CPU and memory bus
 * without introducing dependencies on Qt or specific logging frameworks
 * into the core library.
 */
class ITraceHook {
public:
    virtual ~ITraceHook() = default;

    /**
     * @brief Called when an instruction is about to be executed.
     * @param address Word address in flash.
     * @param opcode Raw 16-bit opcode.
     * @param mnemonic Human-readable instruction mnemonic.
     */
    virtual void on_instruction(u32 address, u16 opcode, std::string_view mnemonic) = 0;

    /**
     * @brief Called when a general-purpose register is modified.
     * @param index Register index (0-31).
     * @param value New 8-bit value.
     */
    virtual void on_register_write(u8 index, u8 value) = 0;

    /**
     * @brief Called when the status register (SREG) is modified.
     * @param value New 8-bit SREG value.
     */
    virtual void on_sreg_write(u8 value) = 0;

    /**
     * @brief Called when a byte is read from the data space.
     * @param address 16-bit data space address.
     * @param value The value read.
     */
    virtual void on_memory_read(u16 address, u8 value) = 0;

    /**
     * @brief Called when a byte is written to the data space.
     * @param address 16-bit data space address.
     * @param value The value written.
     */
    virtual void on_memory_write(u16 address, u8 value) = 0;

    /**
     * @brief Called when an interrupt request is serviced.
     * @param vector The interrupt vector index being dispatched.
     */
    virtual void on_interrupt(u8 vector) = 0;
};

/**
 * @brief Multiplexes multiple ITraceHook instances.
 */
class TraceMultiplexer final : public ITraceHook {
public:
    void add_hook(ITraceHook* hook) {
        if (hook) hooks_.push_back(hook);
    }

    void remove_hook(ITraceHook* hook) {
        hooks_.erase(std::remove(hooks_.begin(), hooks_.end(), hook), hooks_.end());
    }

    void on_instruction(u32 address, u16 opcode, std::string_view mnemonic) override {
        for (auto* h : hooks_) h->on_instruction(address, opcode, mnemonic);
    }

    void on_register_write(u8 index, u8 value) override {
        for (auto* h : hooks_) h->on_register_write(index, value);
    }

    void on_sreg_write(u8 value) override {
        for (auto* h : hooks_) h->on_sreg_write(value);
    }

    void on_memory_read(u16 address, u8 value) override {
        for (auto* h : hooks_) h->on_memory_read(address, value);
    }

    void on_memory_write(u16 address, u8 value) override {
        for (auto* h : hooks_) h->on_memory_write(address, value);
    }

    void on_interrupt(u8 vector) override {
        for (auto* h : hooks_) h->on_interrupt(vector);
    }

private:
    std::vector<ITraceHook*> hooks_;
};

}  // namespace vioavr::core
