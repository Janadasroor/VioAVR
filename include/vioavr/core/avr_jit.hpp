#pragma once

#include "vioavr/core/types.hpp"
#include "vioavr/core/x86_assembler.hpp"
#include <sys/mman.h>
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <vector>
#include <memory>

namespace vioavr::core {

class AvrCpu;
class MemoryBus;

namespace jit {

// State block exchanged between interpreter and JIT.
// The JIT code reads these fields, executes instructions, and writes results.
// The caller syncs state back to AvrCpu after each block.
struct alignas(64) JitState {
    uint8_t gpr[32]{};
    uint32_t pc{0};
    uint16_t sp{0};
    uint8_t sreg{0};
    uint8_t reserved1{0};
    uint64_t cycles{0};
    MemoryBus* bus{nullptr};
    uint8_t* bus_data{nullptr};  // points to bus->data_space() for fast I/O access
    uint8_t interrupt_depth{0};  // for JIT-compiled RETI
    uint8_t interrupt_delay{0};  // one-instruction delay after SEI/RETI
    uint8_t _pad2[6]{};
    uint64_t instructions_executed{0};
    uint8_t _pad[48]{};          // pad to 128 bytes (2×64 cache lines)
};

inline uint8_t sreg_to_byte(bool c, bool z, bool n, bool v, bool s, bool h, bool t, bool i) {
    return (static_cast<uint8_t>(c) << 0) |
           (static_cast<uint8_t>(z) << 1) |
           (static_cast<uint8_t>(n) << 2) |
           (static_cast<uint8_t>(v) << 3) |
           (static_cast<uint8_t>(s) << 4) |
           (static_cast<uint8_t>(h) << 5) |
           (static_cast<uint8_t>(t) << 6) |
           (static_cast<uint8_t>(i) << 7);
}

inline void sreg_from_byte(uint8_t sreg, bool& c, bool& z, bool& n, bool& v, bool& s, bool& h, bool& t, bool& i) {
    c = (sreg >> 0) & 1;
    z = (sreg >> 1) & 1;
    n = (sreg >> 2) & 1;
    v = (sreg >> 3) & 1;
    s = (sreg >> 4) & 1;
    h = (sreg >> 5) & 1;
    t = (sreg >> 6) & 1;
    i = (sreg >> 7) & 1;
}

struct JitBlock {
    uint32_t start_pc{};
    uint32_t end_pc{};
    uint64_t total_cycles{};
    void* code{nullptr};
    size_t code_size{0};

    ~JitBlock() { release_code(); }
    JitBlock() = default;
    JitBlock(JitBlock&& other) noexcept
        : start_pc(other.start_pc), end_pc(other.end_pc),
          total_cycles(other.total_cycles), code(other.code), code_size(other.code_size) {
        other.code = nullptr;
        other.code_size = 0;
    }
    JitBlock& operator=(JitBlock&& other) noexcept {
        if (this != &other) {
            release_code();
            start_pc = other.start_pc;
            end_pc = other.end_pc;
            total_cycles = other.total_cycles;
            code = other.code;
            code_size = other.code_size;
            other.code = nullptr;
            other.code_size = 0;
        }
        return *this;
    }
    JitBlock(const JitBlock&) = delete;
    JitBlock& operator=(const JitBlock&) = delete;

    void release_code() {
        if (code != nullptr && code_size > 0) {
            munmap(code, code_size);
            code = nullptr;
            code_size = 0;
        }
    }

    using ExecFunc = void (*)(JitState*);
    ExecFunc func() const { return reinterpret_cast<ExecFunc>(code); }
};

class AvrJit {
public:
    AvrJit();
    ~AvrJit();

    AvrJit(const AvrJit&) = delete;
    AvrJit& operator=(const AvrJit&) = delete;

    // Try to translate a block starting at PC. Returns true on success.
    bool translate(uint32_t start_pc, const u16* flash, uint32_t flash_size,
                   u16 sreg_address = 0x5F);

    // Execute a cached block. JitState is pre-populated with current CPU state.
    void execute(uint32_t start_pc, JitState* state);

    // Returns true if a block for PC is cached.
    bool has_block(uint32_t pc) const { return cache_.blocks.count(pc) > 0; }

    // Invalidate all cached blocks (SPM write, etc.)
    void invalidate_all();

    void* allocate_executable(size_t size);
    void free_executable(void* ptr, size_t size);

    // Get number of cached blocks
    size_t cache_size() const { return cache_.blocks.size(); }

    struct DebugStats {
        uint64_t translate_count;
        uint64_t execute_count;
        uint64_t execute_cycles;
    };
    DebugStats debug_stats() const {
        return {translate_count_, execute_count_, execute_cycles_};
    }

private:
    struct BlockCache {
        std::unordered_map<uint32_t, JitBlock> blocks;
    } cache_;

    const u16* flash_{nullptr};
    uint32_t flash_size_{0};
    u16 sreg_address_{0x5F};

    // Debug counters
    uint64_t translate_count_{0};
    uint64_t execute_count_{0};
    uint64_t execute_cycles_{0};

    // Translate a single AVR instruction. Returns number of 16-bit words consumed.
    // Returns 0 if instruction can't be JIT'd.
    // skip_patch_out: if non-null, set to a jmp_forward label that the caller
    // must patch after emitting the next instruction (for SBIS/SBIC/SBRC/SBRS).
    uint32_t translate_instruction(CodeBuffer& buf, u16 opcode,
                                   uint32_t pc, bool& block_ended,
                                   CodeBuffer::Label* skip_patch_out = nullptr);
};

} // namespace jit
} // namespace vioavr::core
