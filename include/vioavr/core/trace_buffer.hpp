#pragma once

#include "vioavr/core/avr_cpu.hpp"
#include <vector>
#include <mutex>
#include <atomic>

namespace vioavr::core {

/**
 * @brief Circular backbuffer for CPU execution history.
 */
class TraceBuffer final : public ITraceHook {
public:
    explicit TraceBuffer(size_t capacity = 10000);
    ~TraceBuffer() override = default;

    /**
     * @brief Get a copy of the buffer.
     */
    [[nodiscard]] std::vector<CpuSnapshot> snapshots() const;
    
    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] size_t capacity() const { return capacity_; }

    void clear();

    // ITraceHook implementation
    void on_instruction(u32 address, u16 opcode, std::string_view mnemonic) override;
    void on_register_write([[maybe_unused]] u8 index, [[maybe_unused]] u8 value) override {}
    void on_sreg_write([[maybe_unused]] u8 value) override {}
    void on_memory_read([[maybe_unused]] u16 address, [[maybe_unused]] u8 value) override {}
    void on_memory_write([[maybe_unused]] u16 address, [[maybe_unused]] u8 value) override {}
    void on_interrupt(u8 vector) override;

private:
    const size_t capacity_;
    std::vector<CpuSnapshot> buffer_;
    size_t head_ {0};
    size_t size_ {0};
    mutable std::mutex mutex_;
    
    // We need a pointer to the CPU to take snapshots
    AvrCpu* cpu_ {nullptr};
public:
    void set_cpu(AvrCpu* cpu) { cpu_ = cpu; }
};

} // namespace vioavr::core
