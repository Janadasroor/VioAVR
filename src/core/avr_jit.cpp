#include "vioavr/core/avr_jit.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/types.hpp"
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace vioavr::core::jit {

// ---------------------------------------------------------------------------
// C++ flag helpers called from JIT-compiled code
// These must be 'extern "C"' so their addresses are stable for embedding
// in JIT code (no name mangling issues).
// ---------------------------------------------------------------------------
extern "C" {

void jit_add_flags(JitState* state, uint8_t rd, uint8_t rr, uint8_t result) {
    bool d7 = (rd & 0x80) != 0;
    bool s7 = (rr & 0x80) != 0;
    bool r7 = (result & 0x80) != 0;
    bool d3 = (rd & 0x08) != 0;
    bool s3 = (rr & 0x08) != 0;
    bool r3 = (result & 0x08) != 0;
    bool c = (d7 && s7) || (s7 && !r7) || (!r7 && d7);
    bool z = (result == 0);
    bool n = r7;
    bool v = (d7 && s7 && !r7) || (!d7 && !s7 && r7);
    bool h = (d3 && s3) || (s3 && !r3) || (!r3 && d3);
    bool s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | sreg_to_byte(c, z, n, v, s, h, false, false);
}

void jit_sub_flags(JitState* state, uint8_t rd, uint8_t rr, uint8_t result) {
    bool d7 = (rd & 0x80) != 0;
    bool s7 = (rr & 0x80) != 0;
    bool r7 = (result & 0x80) != 0;
    bool d3 = (rd & 0x08) != 0;
    bool s3 = (rr & 0x08) != 0;
    bool r3 = (result & 0x08) != 0;
    bool c = (!d7 && s7) || (s7 && r7) || (r7 && !d7);
    bool z = (result == 0);
    bool n = r7;
    bool v = (d7 && !s7 && !r7) || (!d7 && s7 && r7);
    bool h = (!d3 && s3) || (s3 && r3) || (r3 && !d3);
    bool s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | sreg_to_byte(c, z, n, v, s, h, false, false);
}

void jit_logic_flags(JitState* state, uint8_t /*rd*/, uint8_t /*rr*/, uint8_t result) {
    bool z = (result == 0);
    bool n = (result & 0x80) != 0;
    bool v = false;
    bool s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | sreg_to_byte(false, z, n, v, s, false, false, false);
}

// Sync pending block cycles to peripherals (call before I/O reads)
void jit_io_tick(JitState* state) {
    uint64_t block = state->jit_block_cycles;
    uint64_t synced = state->jit_synced_cycles;
    if (block > synced) {
        uint64_t delta = block - synced;
        state->bus->tick_peripherals(delta, 0xFF);
        state->cycles += delta;
        state->jit_synced_cycles = block;
    }
}

// Block end: sync remaining cycles, add instruction count, reset accumulators
void jit_block_end_tick(JitState* state, uint32_t insn_count) {
    jit_io_tick(state);
    state->instructions_executed += insn_count;
    state->jit_block_cycles = 0;
    state->jit_synced_cycles = 0;
}

// Memory access helpers for JIT-compiled LDD/STD/LDS/STS/IN/OUT
uint8_t jit_read_data(JitState* state, uint16_t addr) {
    auto& dev = state->bus->device();
    if (addr == dev.sreg_address) {
        return state->sreg;
    }
    if (addr == dev.spl_address) {
        return static_cast<uint8_t>(state->sp & 0xFF);
    }
    if (addr == dev.sph_address) {
        return static_cast<uint8_t>((state->sp >> 8) & 0xFF);
    }
    auto sram = dev.sram_range();
    if (addr >= sram.begin && addr <= sram.end) {
        return state->bus->data_space()[addr];
    }
    jit_io_tick(state);  // sync peripherals before reading
    return state->bus->read_data(addr);
}

void jit_write_data(JitState* state, uint16_t addr, uint8_t value) {
    auto& dev = state->bus->device();
    if (addr == dev.sreg_address) {
        bool old_i = (state->sreg & 0x80) != 0;
        bool new_i = (value & 0x80) != 0;
        state->sreg = value;
        if (new_i && !old_i)
            state->interrupt_delay = 1;
        return;
    }
    if (addr == dev.spl_address) {
        state->sp = (state->sp & 0xFF00) | value;
        state->bus->write_data(addr, value);
        return;
    }
    if (addr == dev.sph_address) {
        state->sp = (state->sp & 0x00FF) | (static_cast<uint16_t>(value) << 8);
        state->bus->write_data(addr, value);
        return;
    }
    auto reg_range = dev.register_file_range;
    if (addr >= reg_range.begin && addr <= reg_range.end) {
        state->gpr[addr - reg_range.begin] = value;
        return;
    }
    auto sram = dev.sram_range();
    if (addr >= sram.begin && addr <= sram.end) {
        state->bus->data_space()[addr] = value;
        return;
    }
    state->bus->write_data(addr, value);
    state->bus_data[addr] = value;
}

void jit_push(JitState* state, uint8_t value) {
    state->bus->write_data(state->sp, value);
    state->sp--;
}

uint8_t jit_pop(JitState* state) {
    state->sp++;
    return state->bus->read_data(state->sp);
}

// Shift helpers
void jit_lsr(JitState* state, uint8_t rd) {
    uint8_t val = state->gpr[rd];
    uint8_t old_bit0 = val & 1;
    state->gpr[rd] = val >> 1;
    uint8_t r = state->gpr[rd];
    uint8_t c = old_bit0;
    uint8_t n = (r & 0x80) ? 1 : 0;
    uint8_t z = (r == 0) ? 1 : 0;
    uint8_t v = n ^ c;
    uint8_t s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | c | (z << 1) | (n << 2) | (v << 3) | (s << 4);
}

void jit_asr(JitState* state, uint8_t rd) {
    int8_t val = static_cast<int8_t>(state->gpr[rd]);
    uint8_t old_bit0 = static_cast<uint8_t>(val) & 1;
    state->gpr[rd] = static_cast<uint8_t>(val >> 1);
    uint8_t r = state->gpr[rd];
    uint8_t c = old_bit0;
    uint8_t n = (r & 0x80) ? 1 : 0;
    uint8_t z = (r == 0) ? 1 : 0;
    uint8_t v = n ^ c;
    uint8_t s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | c | (z << 1) | (n << 2) | (v << 3) | (s << 4);
}

void jit_ror(JitState* state, uint8_t rd) {
    uint8_t val = state->gpr[rd];
    uint8_t old_c = state->sreg & 1;
    uint8_t old_bit0 = val & 1;
    state->gpr[rd] = static_cast<uint8_t>((val >> 1) | (old_c << 7));
    uint8_t r = state->gpr[rd];
    uint8_t c = old_bit0;
    uint8_t n = (r & 0x80) ? 1 : 0;
    uint8_t z = (r == 0) ? 1 : 0;
    uint8_t v = n ^ c;
    uint8_t s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | c | (z << 1) | (n << 2) | (v << 3) | (s << 4);
}

void jit_sbci(JitState* state, uint8_t rd, uint8_t imm) {
    uint8_t old_sreg = state->sreg;
    uint8_t carry = old_sreg & 1;
    uint8_t lhs = state->gpr[rd];
    uint8_t result = static_cast<uint8_t>(lhs - imm - carry);
    state->gpr[rd] = result;

    bool d7 = (lhs & 0x80) != 0;
    bool s7 = (imm & 0x80) != 0;
    bool r7 = (result & 0x80) != 0;
    bool d3 = (lhs & 0x08) != 0;
    bool s3 = (imm & 0x08) != 0;
    bool r3 = (result & 0x08) != 0;
    bool c = (!d7 && s7) || (s7 && r7) || (r7 && !d7);
    bool z = ((old_sreg & 2) != 0) && (result == 0);
    bool n = r7;
    bool v = (d7 && !s7 && !r7) || (!d7 && s7 && r7);
    bool h = (!d3 && s3) || (s3 && r3) || (r3 && !d3);
    bool s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | sreg_to_byte(c, z, n, v, s, h, false, false);
}

void jit_adc(JitState* state, uint8_t rd, uint8_t rr) {
    uint8_t carry = state->sreg & 1;
    uint8_t lhs = state->gpr[rd];
    uint8_t rhs = state->gpr[rr];
    uint8_t result = static_cast<uint8_t>(lhs + rhs + carry);
    state->gpr[rd] = result;
    bool d7 = (lhs & 0x80) != 0;
    bool s7 = (rhs & 0x80) != 0;
    bool r7 = (result & 0x80) != 0;
    bool d3 = (lhs & 0x08) != 0;
    bool s3 = (rhs & 0x08) != 0;
    bool r3 = (result & 0x08) != 0;
    bool c = (d7 && s7) || (s7 && !r7) || (!r7 && d7);
    bool z = (result == 0);
    bool n = r7;
    bool v = (d7 && s7 && !r7) || (!d7 && !s7 && r7);
    bool h = (d3 && s3) || (s3 && !r3) || (!r3 && d3);
    bool s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | sreg_to_byte(c, z, n, v, s, h, false, false);
}

void jit_sbc(JitState* state, uint8_t rd, uint8_t rr) {
    uint8_t old_sreg = state->sreg;
    uint8_t carry = old_sreg & 1;
    uint8_t lhs = state->gpr[rd];
    uint8_t rhs = state->gpr[rr];
    uint8_t result = static_cast<uint8_t>(lhs - rhs - carry);
    state->gpr[rd] = result;
    bool d7 = (lhs & 0x80) != 0;
    bool s7 = (rhs & 0x80) != 0;
    bool r7 = (result & 0x80) != 0;
    bool d3 = (lhs & 0x08) != 0;
    bool s3 = (rhs & 0x08) != 0;
    bool r3 = (result & 0x08) != 0;
    bool c = (!d7 && s7) || (s7 && r7) || (r7 && !d7);
    bool z = ((old_sreg & 2) != 0) && (result == 0);
    bool n = r7;
    bool v = (d7 && !s7 && !r7) || (!d7 && s7 && r7);
    bool h = (!d3 && s3) || (s3 && r3) || (r3 && !d3);
    bool s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | sreg_to_byte(c, z, n, v, s, h, false, false);
}

void jit_cpc(JitState* state, uint8_t rd, uint8_t rr) {
    uint8_t old_sreg = state->sreg;
    uint8_t carry = old_sreg & 1;
    uint8_t lhs = state->gpr[rd];
    uint8_t rhs = state->gpr[rr];
    uint8_t result = static_cast<uint8_t>(lhs - rhs - carry);
    bool d7 = (lhs & 0x80) != 0;
    bool s7 = (rhs & 0x80) != 0;
    bool r7 = (result & 0x80) != 0;
    bool d3 = (lhs & 0x08) != 0;
    bool s3 = (rhs & 0x08) != 0;
    bool r3 = (result & 0x08) != 0;
    bool c = (!d7 && s7) || (s7 && r7) || (r7 && !d7);
    bool z = ((old_sreg & 2) != 0) && (result == 0);
    bool n = r7;
    bool v = (d7 && !s7 && !r7) || (!d7 && s7 && r7);
    bool h = (!d3 && s3) || (s3 && r3) || (r3 && !d3);
    bool s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | sreg_to_byte(c, z, n, v, s, h, false, false);
}

// Flag-preservation helper for instructions that don't modify C/H
static void sreg_preserve_c(JitState* state, uint8_t c, uint8_t z, uint8_t n, uint8_t v, uint8_t s) {
    state->sreg = (state->sreg & 0xE0) | c | (z << 1) | (n << 2) | (v << 3) | (s << 4);
}

void jit_com(JitState* state, uint8_t rd) {
    uint8_t val = state->gpr[rd];
    uint8_t result = static_cast<uint8_t>(0xFF - val);
    state->gpr[rd] = result;
    uint8_t r7 = (result >> 7) & 1;
    sreg_preserve_c(state, 1, result == 0, r7, 0, r7);
}

void jit_neg(JitState* state, uint8_t rd) {
    uint8_t val = state->gpr[rd];
    uint8_t result = static_cast<uint8_t>(0 - val);
    state->gpr[rd] = result;
    bool r7 = (result & 0x80) != 0;
    bool d3 = (val & 0x08) != 0;
    bool r3 = (result & 0x08) != 0;
    bool z = (result == 0);
    bool n = r7;
    bool v = (result == 0x80);
    bool s = n ^ v;
    bool c = (result != 0);
    bool h = d3 || r3;
    state->sreg = (state->sreg & 0xC0) | sreg_to_byte(c, z, n, v, s, h, false, false);
}

void jit_inc(JitState* state, uint8_t rd) {
    uint8_t val = state->gpr[rd];
    uint8_t result = static_cast<uint8_t>(val + 1);
    state->gpr[rd] = result;
    uint8_t r7 = (result >> 7) & 1;
    sreg_preserve_c(state, state->sreg & 1, result == 0, r7, result == 0x80, r7 ^ (result == 0x80));
}

void jit_dec(JitState* state, uint8_t rd) {
    uint8_t val = state->gpr[rd];
    uint8_t result = static_cast<uint8_t>(val - 1);
    state->gpr[rd] = result;
    uint8_t r7 = (result >> 7) & 1;
    sreg_preserve_c(state, state->sreg & 1, result == 0, r7, result == 0x7F, r7 ^ (result == 0x7F));
}

void jit_sbi(JitState* state, uint8_t io_offset, uint8_t bit_index) {
    uint16_t addr = static_cast<uint16_t>(0x20 + io_offset);
    uint8_t val = jit_read_data(state, addr);
    val |= static_cast<uint8_t>(1u << bit_index);
    jit_write_data(state, addr, val);
}

void jit_cbi(JitState* state, uint8_t io_offset, uint8_t bit_index) {
    uint16_t addr = static_cast<uint16_t>(0x20 + io_offset);
    uint8_t val = jit_read_data(state, addr);
    val &= static_cast<uint8_t>(~(1u << bit_index));
    jit_write_data(state, addr, val);
}

void jit_mul(JitState* state, uint8_t rd, uint8_t rr) {
    uint16_t product = static_cast<uint16_t>(state->gpr[rd]) * static_cast<uint16_t>(state->gpr[rr]);
    state->gpr[0] = static_cast<uint8_t>(product & 0xFF);
    state->gpr[1] = static_cast<uint8_t>((product >> 8) & 0xFF);
    bool z = (product == 0);
    bool c = (product & 0x8000) != 0;
    state->sreg = (state->sreg & 0xC0) | (c ? 1u : 0u) | (z ? 2u : 0u);
}

void jit_adiw(JitState* state, uint8_t pair, uint8_t imm) {
    uint8_t lo = static_cast<uint8_t>(24 + pair * 2);
    uint16_t val = static_cast<uint16_t>(state->gpr[lo]) |
                   (static_cast<uint16_t>(state->gpr[lo + 1]) << 8);
    uint16_t result = static_cast<uint16_t>(val + imm);
    state->gpr[lo] = static_cast<uint8_t>(result & 0xFF);
    state->gpr[lo + 1] = static_cast<uint8_t>((result >> 8) & 0xFF);
    bool r15 = (result >> 15) != 0;
    bool v15 = (val >> 15) != 0;
    bool c = v15 && !r15;
    bool v = v15 ^ r15;
    bool n = r15;
    bool z = (result == 0);
    bool s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | (c ? 1u : 0u) | (z ? 2u : 0u) | (n ? 4u : 0u) |
                  (v ? 8u : 0u) | (s ? 16u : 0u);
}

void jit_sbiw(JitState* state, uint8_t pair, uint8_t imm) {
    uint8_t lo = static_cast<uint8_t>(24 + pair * 2);
    uint16_t val = static_cast<uint16_t>(state->gpr[lo]) |
                   (static_cast<uint16_t>(state->gpr[lo + 1]) << 8);
    uint16_t result = static_cast<uint16_t>(val - imm);
    state->gpr[lo] = static_cast<uint8_t>(result & 0xFF);
    state->gpr[lo + 1] = static_cast<uint8_t>((result >> 8) & 0xFF);
    bool r15 = (result >> 15) != 0;
    bool v15 = (val >> 15) != 0;
    bool c = !v15 && r15;
    bool v = v15 ^ r15;
    bool n = r15;
    bool z = (result == 0);
    bool s = n ^ v;
    state->sreg = (state->sreg & 0xC0) | (c ? 1u : 0u) | (z ? 2u : 0u) | (n ? 4u : 0u) |
                  (v ? 8u : 0u) | (s ? 16u : 0u);
}

// Subroutine helpers
void jit_rcall(JitState* state, uint32_t return_pc) {
    state->bus->write_data(state->sp, static_cast<uint8_t>(return_pc & 0xFF));
    state->sp--;
    state->bus->write_data(state->sp, static_cast<uint8_t>((return_pc >> 8) & 0xFF));
    state->sp--;
}

void jit_ret(JitState* state) {
    state->sp++;
    uint8_t high = state->bus->read_data(state->sp);
    state->sp++;
    uint8_t low = state->bus->read_data(state->sp);
    state->pc = static_cast<uint32_t>(low) | (static_cast<uint32_t>(high) << 8);
}

void jit_reti(JitState* state) {
    // Pop PC from stack (same as RET)
    state->sp++;
    uint8_t high = state->bus->read_data(state->sp);
    state->sp++;
    uint8_t low = state->bus->read_data(state->sp);
    state->pc = static_cast<uint32_t>(low) | (static_cast<uint32_t>(high) << 8);

    // Set I-flag in SREG
    state->sreg |= 0x80U;

    // Decrement interrupt depth (don't underflow)
    if (state->interrupt_depth > 0) {
        state->interrupt_depth--;
    }

    // One-instruction delay before next interrupt
    state->interrupt_delay = 1;

    // Notify peripherals (e.g., CPUINT) that RETI executed
    if (state->bus != nullptr) {
        state->bus->on_reti();
    }
}

} // extern "C"

// ---------------------------------------------------------------------------
// AVR opcode decode helpers (static, in namespace scope)
// ---------------------------------------------------------------------------
static constexpr uint8_t rd5(u16 op) { return static_cast<uint8_t>((op >> 4) & 0x1F); }
static constexpr uint8_t rr5(u16 op) {
    return static_cast<uint8_t>(((op >> 5) & 0x10) | (op & 0x0F));
}
static constexpr uint8_t reg4_high(u16 op) { return static_cast<uint8_t>(16 + ((op >> 4) & 0xF)); }
static constexpr uint8_t imm8_ldi(u16 op) {
    return static_cast<uint8_t>(((op >> 4) & 0xF0) | (op & 0x0F));
}

// 8-bit register MOV: dst = src, using MOV r/m8, r8 (0x88 /r)
static void emit_mov8(CodeBuffer& buf, Reg8 dst, Reg8 src) {
    // opcode 88 /r: MOV r/m8, r8 (dst is r/m8, src is r8)
    bool need_rex = static_cast<uint8_t>(dst) >= 8 || static_cast<uint8_t>(src) >= 8;
    buf.rex(false, static_cast<uint8_t>(src) >= 8, false, static_cast<uint8_t>(dst) >= 8);
    buf.emit8(0x88);
    buf.modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
}

// ---------------------------------------------------------------------------
// AvrJit implementation
// ---------------------------------------------------------------------------
AvrJit::AvrJit() {}

AvrJit::~AvrJit() { invalidate_all(); }

void* AvrJit::allocate_executable(size_t size) {
#ifdef _WIN32
    void* ptr = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    return ptr;
#else
    long page_size = sysconf(_SC_PAGESIZE);
    size_t alloc_size = ((size + page_size - 1) / page_size) * page_size;
    void* ptr = mmap(nullptr, alloc_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (ptr == MAP_FAILED) ? nullptr : ptr;
#endif
}

void AvrJit::free_executable(void* ptr, size_t size) {
    if (!ptr) return;
#ifdef _WIN32
    (void)size;
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    long page_size = sysconf(_SC_PAGESIZE);
    size_t alloc_size = ((size + page_size - 1) / page_size) * page_size;
    munmap(ptr, alloc_size);
#endif
}

void AvrJit::invalidate_all() {
    for (auto& [pc, block] : cache_.blocks)
        block.release_code();
    cache_.blocks.clear();
}

// ---------------------------------------------------------------------------
// Windows x64 calling-convention helpers
// System V (non-Windows): RDI=arg0, RSI=arg1, RDX=arg2, RCX=arg3
// Windows x64:           RCX=arg0, RDX=arg1, R8=arg2,  R9=arg3
// Shadow space: 32 bytes + 8-byte alignment = 40 bytes subtracted from RSP
// ---------------------------------------------------------------------------
#ifdef _WIN32
#define JIT_CALL_1ARG(func_) \
    buf.mov(Reg64::rcx, Reg64::r14); \
    buf.sub_imm8(Reg64::rsp, 40); \
    buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(func_)); \
    buf.call(Reg64::r11); \
    buf.add_imm8(Reg64::rsp, 40)
#define JIT_CALL_2ARG(func_, a1_) \
    buf.mov(Reg64::rcx, Reg64::r14); \
    buf.mov(Reg64::rdx, (a1_)); \
    buf.sub_imm8(Reg64::rsp, 40); \
    buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(func_)); \
    buf.call(Reg64::r11); \
    buf.add_imm8(Reg64::rsp, 40)
#define JIT_CALL_3ARG(func_, a1_, a2_) \
    buf.mov(Reg64::rcx, Reg64::r14); \
    buf.mov(Reg64::rdx, (a1_)); \
    buf.mov(Reg64::r8, (a2_)); \
    buf.sub_imm8(Reg64::rsp, 40); \
    buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(func_)); \
    buf.call(Reg64::r11); \
    buf.add_imm8(Reg64::rsp, 40)
#else
#define JIT_CALL_1ARG(func_) \
    buf.mov(Reg64::rdi, Reg64::r14); \
    buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(func_)); \
    buf.call(Reg64::r11)
#define JIT_CALL_2ARG(func_, a1_) \
    buf.mov(Reg64::rdi, Reg64::r14); \
    buf.mov(Reg64::rsi, (a1_)); \
    buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(func_)); \
    buf.call(Reg64::r11)
#define JIT_CALL_3ARG(func_, a1_, a2_) \
    buf.mov(Reg64::rdi, Reg64::r14); \
    buf.mov(Reg64::rsi, (a1_)); \
    buf.mov(Reg64::rdx, (a2_)); \
    buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(func_)); \
    buf.call(Reg64::r11)
#endif

// Helper: emit call to a C++ flag helper.
// Expects: RAX = result, RDX = original Rd value, RCX = Rr value
// Clobbers: RDI/RCX, RSI/RDX, RDX/R8, RCX/R9, RAX, R11
// ---------------------------------------------------------------------------
static void emit_flag_call(CodeBuffer& buf, const void* helper_addr) {
#ifdef _WIN32
    buf.mov(Reg64::r8, Reg64::rcx);
    buf.mov(Reg64::rcx, Reg64::r14);
    buf.mov(Reg64::rdx, Reg64::rdx);
    buf.mov(Reg64::r9, Reg64::rax);
    buf.sub_imm8(Reg64::rsp, 40);
    buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(helper_addr));
    buf.call(Reg64::r11);
    buf.add_imm8(Reg64::rsp, 40);
#else
    buf.mov(Reg64::rdi, Reg64::r14);
    buf.mov(Reg64::rsi, Reg64::rdx);
    buf.mov(Reg64::rdx, Reg64::rcx);
    buf.mov(Reg64::rcx, Reg64::rax);
    buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(helper_addr));
    buf.call(Reg64::r11);
#endif
}

// Helper: emit MOV [R14+32], imm32 (set state->pc)
static void emit_set_pc(CodeBuffer& buf, uint32_t pc) {
    buf.rex(false, false, false, true);
    buf.emit8(0xC7);
    buf.modrm(1, 0, 6); // mod=01, reg=0, rm=6
    buf.emit8(32);      // offset of pc in JitState
    buf.emit32(pc);
}

// ---------------------------------------------------------------------------
// translate_instruction: emit x86 code for one AVR instruction
// pc = address of this instruction (word address)
// Returns number of 16-bit words consumed (0 = can't JIT)
// ---------------------------------------------------------------------------
uint32_t AvrJit::translate_instruction(CodeBuffer& buf, u16 opcode,
                                       uint32_t pc, bool& block_ended,
                                       CodeBuffer::Label* skip_patch_out) {
    u16 high = opcode & 0xF000;

    switch (high) {
    case 0x0000: case 0x1000: {
        if (opcode == 0x0000) { // NOP
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // MOVW Rd, Rr (0x01xx) — dddd = Rd/2, rrrr = Rr/2
        if ((opcode & 0xFF00) == 0x0100) {
            uint8_t rd = static_cast<uint8_t>(((opcode >> 4) & 0x0F) * 2);
            uint8_t rr = static_cast<uint8_t>((opcode & 0x0F) * 2);
            buf.movzx_membase(Reg64::rax, Reg64::r14, rr);
            buf.mov_membase8(Reg8::al, Reg64::r14, rd);
            buf.movzx_membase(Reg64::rax, Reg64::r14, rr + 1);
            buf.mov_membase8(Reg8::al, Reg64::r14, rd + 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // CP (0x14xx) - Compare registers
        if ((opcode & 0xFC00) == 0x1400) {
            uint8_t rd = rd5(opcode);
            uint8_t rr = rr5(opcode);
            buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
            buf.mov(Reg64::rdx, Reg64::rax);
            buf.movzx_membase(Reg64::rcx, Reg64::r14, rr);
            buf.sub8(Reg8::al, Reg8::cl);
            emit_flag_call(buf, reinterpret_cast<void*>(&jit_sub_flags));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // ADC Rd, Rr (0x1Cxx) — add with carry
        if ((opcode & 0xFC00) == 0x1C00) {
            uint8_t rd = rd5(opcode);
            uint8_t rr = rr5(opcode);
            JIT_CALL_3ARG(&jit_adc, static_cast<int32_t>(rd), static_cast<int32_t>(rr));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // ADD Rd, Rr (0x0Cxx)
        if ((opcode & 0xFC00) == 0x0C00) {
            uint8_t rd = rd5(opcode);
            uint8_t rr = rr5(opcode);
            buf.movzx_membase(Reg64::rcx, Reg64::r14, rr);
            buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
            buf.mov(Reg64::rdx, Reg64::rax);
            buf.add8(Reg8::al, Reg8::cl);
            buf.mov_membase8(Reg8::al, Reg64::r14, rd);
            emit_flag_call(buf, reinterpret_cast<void*>(&jit_add_flags));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // SUB Rd, Rr (0x18xx)
        if ((opcode & 0xFC00) == 0x1800) {
            uint8_t rd = rd5(opcode);
            uint8_t rr = rr5(opcode);
            buf.movzx_membase(Reg64::rcx, Reg64::r14, rr);
            buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
            buf.mov(Reg64::rdx, Reg64::rax);
            buf.sub8(Reg8::al, Reg8::cl);
            buf.mov_membase8(Reg8::al, Reg64::r14, rd);
            emit_flag_call(buf, reinterpret_cast<void*>(&jit_sub_flags));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // SBC Rd, Rr (0x08xx) — subtract with carry
        if ((opcode & 0xFC00) == 0x0800) {
            uint8_t rd = rd5(opcode);
            uint8_t rr = rr5(opcode);
            JIT_CALL_3ARG(&jit_sbc, static_cast<int32_t>(rd), static_cast<int32_t>(rr));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // CPC Rd, Rr (0x04xx) — compare with carry (no write)
        if ((opcode & 0xFC00) == 0x0400) {
            uint8_t rd = rd5(opcode);
            uint8_t rr = rr5(opcode);
            JIT_CALL_3ARG(&jit_cpc, static_cast<int32_t>(rd), static_cast<int32_t>(rr));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        return 0;
    }

    case 0x2000: case 0x2400: case 0x2800: case 0x2C00: {
        // AND (0x2xxx), EOR (0x24xx), OR (0x28xx), MOV (0x2Cxx)
        uint8_t rd = rd5(opcode);
        uint8_t rr = rr5(opcode);

        // Load Rr into DL
        buf.movzx_membase(Reg64::rcx, Reg64::r14, rr);
        // Load Rd into AL
        buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
        // Save original Rd in EDX
        buf.mov(Reg64::rdx, Reg64::rax);

        if ((opcode & 0xFC00) == 0x2000)       // AND
            buf.and8(Reg8::al, Reg8::cl);
        else if ((opcode & 0xFC00) == 0x2400)  // EOR
            buf.xor8(Reg8::al, Reg8::cl);
        else if ((opcode & 0xFC00) == 0x2800)  // OR
            buf.or8(Reg8::al, Reg8::cl);
        else                                    // MOV
            emit_mov8(buf, Reg8::al, Reg8::cl);

        // Store result to gpr[rd]
        buf.mov_membase8(Reg8::al, Reg64::r14, rd);

        // Update flags (except for MOV)
        if ((opcode & 0xFC00) != 0x2C00) {
            buf.movzx_membase(Reg64::rax, Reg64::r14, rd); // reload result
            emit_flag_call(buf, reinterpret_cast<void*>(&jit_logic_flags));
        }
        emit_set_pc(buf, pc + 1);
        return 1;
    }

    case 0x3000: // CPI Rd, K
    case 0x5000: // SUBI Rd, K
    {
        uint8_t rd = reg4_high(opcode);
        uint8_t imm = imm8_ldi(opcode);

        buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
        if ((opcode & 0xF000) == 0x3000) { // CPI (no store)
            buf.mov(Reg64::rdx, Reg64::rax);
            buf.cmp_al_imm8(imm);
            buf.mov(Reg64::rcx, static_cast<int32_t>(imm));
            // Recompute result for flag helper
            buf.sub_al_imm8(imm); // AL = Rd - imm = result
            emit_flag_call(buf, reinterpret_cast<void*>(&jit_sub_flags));
        } else { // SUBI (store result)
            buf.mov(Reg64::rdx, Reg64::rax);
            buf.sub_al_imm8(imm);
            buf.mov_membase8(Reg8::al, Reg64::r14, rd);
            buf.mov(Reg64::rcx, static_cast<int32_t>(imm));
            emit_flag_call(buf, reinterpret_cast<void*>(&jit_sub_flags));
        }
        emit_set_pc(buf, pc + 1);
        return 1;
    }

    case 0x4000: { // SBCI Rd, K
        uint8_t rd = reg4_high(opcode);
        uint8_t imm = imm8_ldi(opcode);
        JIT_CALL_3ARG(&jit_sbci, static_cast<int32_t>(rd), static_cast<int32_t>(imm));
        emit_set_pc(buf, pc + 1);
        return 1;
    }

    case 0x7000: // ANDI Rd, K
    case 0x6000: // ORI Rd, K
    {
        uint8_t rd = reg4_high(opcode);
        uint8_t imm = imm8_ldi(opcode);

        buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
        buf.mov(Reg64::rdx, Reg64::rax);

        if ((opcode & 0xF000) == 0x6000)
            buf.or_al_imm8(imm);
        else
            buf.and_al_imm8(imm);

        buf.mov_membase8(Reg8::al, Reg64::r14, rd);
        buf.mov(Reg64::rcx, static_cast<int32_t>(imm));
        buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
        emit_flag_call(buf, reinterpret_cast<void*>(&jit_logic_flags));
        emit_set_pc(buf, pc + 1);
        return 1;
    }

    case 0x8000: case 0x9000: case 0xA000: case 0xB000: {
        // RET (0x9508) and RETI (0x9518) — both JIT-compiled. RETI handles I-flag
        // re-enable, interrupt_depth decrement, and bus->on_reti() via jit_reti().
        if (opcode == 0x9508) {
            JIT_CALL_1ARG(&jit_ret);
            buf.add_membase_imm32(Reg64::r14, 80, 3);
            block_ended = true;
            return 1;
        }
        if (opcode == 0x9518) {
            JIT_CALL_1ARG(&jit_reti);
            buf.add_membase_imm32(Reg64::r14, 80, 4); // 4 cycles for 2-byte PC
            block_ended = true;
            return 1;
        }

        // LDD Y+q: (opcode & 0xD208) == 0x8008
        if ((opcode & 0xD208) == 0x8008) {
            uint8_t dest = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            uint8_t q = static_cast<uint8_t>(((opcode >> 8) & 0x38) | (opcode & 0x07));
            // Load Y pointer: Y = gpr[28] | (gpr[29] << 8)
            buf.movzx_membase(Reg64::rax, Reg64::r14, 28); // low byte
            buf.movzx_membase(Reg64::rcx, Reg64::r14, 29); // high byte
            buf.shl32_imm(Reg32::ecx, 8);
            buf.or32(Reg32::eax, Reg32::ecx); // EAX = Y
            buf.add32_imm(Reg32::eax, q);     // EAX = Y + q
            // Call jit_read_data(state, addr)
            JIT_CALL_2ARG(&jit_read_data, Reg64::rax);
            // Store result in gpr[dest]
            buf.mov_membase8(Reg8::al, Reg64::r14, dest);
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // LDD Z+q: (opcode & 0xD208) == 0x8000
        if ((opcode & 0xD208) == 0x8000) {
            uint8_t dest = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            uint8_t q = static_cast<uint8_t>(((opcode >> 8) & 0x38) | (opcode & 0x07));
            buf.movzx_membase(Reg64::rax, Reg64::r14, 30);
            buf.movzx_membase(Reg64::rcx, Reg64::r14, 31);
            buf.shl32_imm(Reg32::ecx, 8);
            buf.or32(Reg32::eax, Reg32::ecx);
            buf.add32_imm(Reg32::eax, q);
            JIT_CALL_2ARG(&jit_read_data, Reg64::rax);
            buf.mov_membase8(Reg8::al, Reg64::r14, dest);
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // STD Y+q: (opcode & 0xD208) == 0x8208
        if ((opcode & 0xD208) == 0x8208) {
            uint8_t src = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            uint8_t q = static_cast<uint8_t>(((opcode >> 8) & 0x38) | (opcode & 0x07));
            buf.movzx_membase(Reg64::rax, Reg64::r14, 28);
            buf.movzx_membase(Reg64::rcx, Reg64::r14, 29);
            buf.shl32_imm(Reg32::ecx, 8);
            buf.or32(Reg32::eax, Reg32::ecx);
            buf.add32_imm(Reg32::eax, q);
            // Load value to write
#ifdef _WIN32
            buf.movzx_membase(Reg64::r8, Reg64::r14, src);
            buf.mov(Reg64::rcx, Reg64::r14);
            buf.mov(Reg64::rdx, Reg64::rax);
            buf.sub_imm8(Reg64::rsp, 40);
            buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(&jit_write_data));
            buf.call(Reg64::r11);
            buf.add_imm8(Reg64::rsp, 40);
#else
            buf.movzx_membase(Reg64::rdx, Reg64::r14, src);
            buf.mov(Reg64::rdi, Reg64::r14);
            buf.mov(Reg64::rsi, Reg64::rax);
            buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(&jit_write_data));
            buf.call(Reg64::r11);
#endif
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // STD Z+q: (opcode & 0xD208) == 0x8200
        if ((opcode & 0xD208) == 0x8200) {
            uint8_t src = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            uint8_t q = static_cast<uint8_t>(((opcode >> 8) & 0x38) | (opcode & 0x07));
            buf.movzx_membase(Reg64::rax, Reg64::r14, 30);
            buf.movzx_membase(Reg64::rcx, Reg64::r14, 31);
            buf.shl32_imm(Reg32::ecx, 8);
            buf.or32(Reg32::eax, Reg32::ecx);
            buf.add32_imm(Reg32::eax, q);
#ifdef _WIN32
            buf.movzx_membase(Reg64::r8, Reg64::r14, src);
            buf.mov(Reg64::rcx, Reg64::r14);
            buf.mov(Reg64::rdx, Reg64::rax);
            buf.sub_imm8(Reg64::rsp, 40);
            buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(&jit_write_data));
            buf.call(Reg64::r11);
            buf.add_imm8(Reg64::rsp, 40);
#else
            buf.movzx_membase(Reg64::rdx, Reg64::r14, src);
            buf.mov(Reg64::rdi, Reg64::r14);
            buf.mov(Reg64::rsi, Reg64::rax);
            buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(&jit_write_data));
            buf.call(Reg64::r11);
#endif
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // IN: (opcode & 0xF800) == 0xB000 — calls jit_read_data for peripheral sync
        if ((opcode & 0xF800) == 0xB000) {
            uint8_t reg = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            uint8_t io_offset = static_cast<uint8_t>(((opcode >> 5) & 0x30) | (opcode & 0x0F));
            uint16_t data_addr = 0x20 + io_offset;
            if (data_addr == sreg_address_) {
                buf.movzx_membase(Reg64::rax, Reg64::r14, 38); // al = JitState.sreg
            } else {
                JIT_CALL_2ARG(&jit_read_data, static_cast<int32_t>(data_addr));
            }
            buf.mov_membase8(Reg8::al, Reg64::r14, reg);        // gpr[reg] = al
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // OUT: (opcode & 0xF800) == 0xB800
        if ((opcode & 0xF800) == 0xB800) {
            uint8_t reg = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            uint8_t io_offset = static_cast<uint8_t>(((opcode >> 5) & 0x30) | (opcode & 0x0F));
            uint16_t data_addr = 0x20 + io_offset;
#ifdef _WIN32
            buf.movzx_membase(Reg64::r8, Reg64::r14, reg);
            buf.mov(Reg64::rcx, Reg64::r14);
            buf.mov(Reg64::rdx, static_cast<int32_t>(data_addr));
            buf.sub_imm8(Reg64::rsp, 40);
            buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(&jit_write_data));
            buf.call(Reg64::r11);
            buf.add_imm8(Reg64::rsp, 40);
#else
            buf.movzx_membase(Reg64::rdx, Reg64::r14, reg);
            buf.mov(Reg64::rdi, Reg64::r14);
            buf.mov(Reg64::rsi, static_cast<int32_t>(data_addr));
            buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(&jit_write_data));
            buf.call(Reg64::r11);
#endif
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // PUSH: (opcode & 0xFE0F) == 0x920F
        if ((opcode & 0xFE0F) == 0x920F) {
            uint8_t reg = static_cast<uint8_t>((opcode >> 4) & 0x1F);
#ifdef _WIN32
            buf.movzx_membase(Reg64::rdx, Reg64::r14, reg);
            buf.mov(Reg64::rcx, Reg64::r14);
            buf.sub_imm8(Reg64::rsp, 40);
            buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(&jit_push));
            buf.call(Reg64::r11);
            buf.add_imm8(Reg64::rsp, 40);
#else
            buf.movzx_membase(Reg64::rsi, Reg64::r14, reg);
            buf.mov(Reg64::rdi, Reg64::r14);
            buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(&jit_push));
            buf.call(Reg64::r11);
#endif
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // POP: (opcode & 0xFE0F) == 0x900F
        if ((opcode & 0xFE0F) == 0x900F) {
            uint8_t reg = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            JIT_CALL_1ARG(&jit_pop);
            buf.mov_membase8(Reg8::al, Reg64::r14, reg);
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // LSR Rd: (opcode & 0xFE0F) == 0x9406
        if ((opcode & 0xFE0F) == 0x9406) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            JIT_CALL_2ARG(&jit_lsr, static_cast<int32_t>(rd));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // ASR Rd: (opcode & 0xFE0F) == 0x9405
        if ((opcode & 0xFE0F) == 0x9405) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            JIT_CALL_2ARG(&jit_asr, static_cast<int32_t>(rd));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // ROR Rd: (opcode & 0xFE0F) == 0x9407
        if ((opcode & 0xFE0F) == 0x9407) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            JIT_CALL_2ARG(&jit_ror, static_cast<int32_t>(rd));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // LDS Rd, k: (opcode & 0xFE0F) == 0x9000 (2-word)
        if ((opcode & 0xFE0F) == 0x9000) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            if (pc + 1 < flash_size_) {
                uint16_t addr = flash_[pc + 1];
                JIT_CALL_2ARG(&jit_read_data, static_cast<int32_t>(addr));
                buf.mov_membase8(Reg8::al, Reg64::r14, rd);
                buf.add_membase_imm32(Reg64::r14, 80, 1);
                emit_set_pc(buf, pc + 2);
                return 2;
            }
            return 0;
        }

        // STS k, Rr: (opcode & 0xFE0F) == 0x9200 (2-word)
        if ((opcode & 0xFE0F) == 0x9200) {
            uint8_t rr = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            if (pc + 1 < flash_size_) {
                uint16_t addr = flash_[pc + 1];
#ifdef _WIN32
                buf.movzx_membase(Reg64::r8, Reg64::r14, rr);
                buf.mov(Reg64::rcx, Reg64::r14);
                buf.mov(Reg64::rdx, static_cast<int32_t>(addr));
                buf.sub_imm8(Reg64::rsp, 40);
                buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(&jit_write_data));
                buf.call(Reg64::r11);
                buf.add_imm8(Reg64::rsp, 40);
#else
                buf.movzx_membase(Reg64::rdx, Reg64::r14, rr);
                buf.mov(Reg64::rdi, Reg64::r14);
                buf.mov(Reg64::rsi, static_cast<int32_t>(addr));
                buf.movabs(Reg64::r11, reinterpret_cast<uint64_t>(&jit_write_data));
                buf.call(Reg64::r11);
#endif
                buf.add_membase_imm32(Reg64::r14, 80, 1);
                emit_set_pc(buf, pc + 2);
                return 2;
            }
            return 0;
        }

        // BSET: (opcode & 0xFF8F) == 0x9408 - set bit in SREG
        if ((opcode & 0xFF8F) == 0x9408) {
            uint8_t bit = static_cast<uint8_t>((opcode >> 4) & 0x07);
            buf.or_membase_imm8(Reg64::r14, 38, static_cast<uint8_t>(1 << bit));
            if (bit == 7) // SEI: set interrupt_delay = 1 (one-instruction delay)
                buf.mov_membase_imm8(Reg64::r14, 65, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // BCLR: (opcode & 0xFF8F) == 0x9488 - clear bit in SREG
        if ((opcode & 0xFF8F) == 0x9488) {
            uint8_t bit = static_cast<uint8_t>((opcode >> 4) & 0x07);
            buf.and_membase_imm8(Reg64::r14, 38, static_cast<uint8_t>(~(1 << bit)));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // JMP: (opcode & 0xFE0E) == 0x940C (2-word)
        if ((opcode & 0xFE0E) == 0x940C) {
            if (pc + 1 >= flash_size_) return 0;
            uint16_t second = flash_[pc + 1];
            uint32_t target = (((static_cast<uint32_t>(opcode) & 0x01F0) << 13) |
                               ((static_cast<uint32_t>(opcode) & 0x0001) << 16) |
                               second);
            if (target >= flash_size_) return 0;
            buf.add_membase_imm32(Reg64::r14, 80, 2);
            emit_set_pc(buf, target);
            block_ended = true;
            return 2;
        }

        // CALL: (opcode & 0xFE0E) == 0x940E (2-word)
        if ((opcode & 0xFE0E) == 0x940E) {
            if (pc + 1 >= flash_size_) return 0;
            uint16_t second = flash_[pc + 1];
            uint32_t target = (((static_cast<uint32_t>(opcode) & 0x01F0) << 13) |
                               ((static_cast<uint32_t>(opcode) & 0x0001) << 16) |
                               second);
            if (target >= flash_size_) return 0;
            // Push return address
            uint32_t return_pc = pc + 2;
            JIT_CALL_2ARG(&jit_rcall, static_cast<int64_t>(return_pc));
            buf.add_membase_imm32(Reg64::r14, 80, 3);
            // Set PC to target
            emit_set_pc(buf, target);
            block_ended = true;
            return 2;
        }

        // COM Rd: (opcode & 0xFE0F) == 0x9400
        if ((opcode & 0xFE0F) == 0x9400) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            JIT_CALL_2ARG(&jit_com, static_cast<int32_t>(rd));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // NEG Rd: (opcode & 0xFE0F) == 0x9401
        if ((opcode & 0xFE0F) == 0x9401) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            JIT_CALL_2ARG(&jit_neg, static_cast<int32_t>(rd));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // SWAP Rd: (opcode & 0xFE0F) == 0x9402 — no flags affected
        if ((opcode & 0xFE0F) == 0x9402) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
            buf.shl8_imm(Reg8::al, 4);
            buf.mov(Reg64::rcx, Reg64::rax);
            buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
            buf.shr8_imm(Reg8::al, 4);
            buf.or8(Reg8::al, Reg8::cl);
            // Mask to 8 bits
            buf.mov_membase8(Reg8::al, Reg64::r14, rd);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // INC Rd: (opcode & 0xFE0F) == 0x9403
        if ((opcode & 0xFE0F) == 0x9403) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            JIT_CALL_2ARG(&jit_inc, static_cast<int32_t>(rd));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // DEC Rd: (opcode & 0xFE0F) == 0x940A
        if ((opcode & 0xFE0F) == 0x940A) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            JIT_CALL_2ARG(&jit_dec, static_cast<int32_t>(rd));
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // SBI (0x9A00-0x9BFF): Set Bit in I/O Register
        if ((opcode & 0xFF00) == 0x9A00) {
            uint8_t io_offset = static_cast<uint8_t>((opcode >> 3) & 0x1F);
            uint8_t bit_index = static_cast<uint8_t>(opcode & 0x07);
            JIT_CALL_3ARG(&jit_sbi, static_cast<int32_t>(io_offset), static_cast<int32_t>(bit_index));
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // CBI (0x9800-0x99FF): Clear Bit in I/O Register
        if ((opcode & 0xFF00) == 0x9800) {
            uint8_t io_offset = static_cast<uint8_t>((opcode >> 3) & 0x1F);
            uint8_t bit_index = static_cast<uint8_t>(opcode & 0x07);
            JIT_CALL_3ARG(&jit_cbi, static_cast<int32_t>(io_offset), static_cast<int32_t>(bit_index));
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // SBIS (0x9B00-0x9BFF): Skip if Bit in I/O Register Set
        if ((opcode & 0xFF00) == 0x9B00) {
            uint8_t io_offset = static_cast<uint8_t>((opcode >> 3) & 0x1F);
            uint8_t bit_index = static_cast<uint8_t>(opcode & 0x07);
            uint16_t data_addr = static_cast<uint16_t>(0x20 + io_offset);
            bool next_2word = false;
            if (pc + 1 < flash_size_) {
                u16 n = flash_[pc + 1];
                if ((n & 0xFE0F) == 0x9000 && (n & 0x0F) != 0x0F) next_2word = true;
                else if ((n & 0xFE0F) == 0x9200 && (n & 0x0F) != 0x0F) next_2word = true;
                else if ((n & 0xFE0E) == 0x940C) next_2word = true;
                else if ((n & 0xFE0E) == 0x940E) next_2word = true;
            }
            JIT_CALL_2ARG(&jit_read_data, static_cast<int32_t>(data_addr));
            buf.test8_imm(Reg8::al, static_cast<uint8_t>(1u << bit_index));
            auto taken = buf.jnz_forward(); // JNZ if bit SET → skip taken
            auto done = buf.jmp_forward();  // NOT TAKEN: jump over taken path
            buf.patch_jnz(taken);
            // TAKEN path
            emit_set_pc(buf, pc + 1 + (next_2word ? 2u : 1u));
            // n-1: block_cycles includes the skipped instruction's base cycle,
            // so we subtract 1 to avoid double-counting (n=1→0, n=2→1).
            buf.add_membase_imm32(Reg64::r14, 80, next_2word ? 1u : 0u);
            buf.sub_membase_imm32(Reg64::r14, 72, 1);
            if (skip_patch_out) *skip_patch_out = buf.jmp_forward(); // skip next instr(s)
            buf.patch_jump(done);
            return 1;
        }

        // SBIC (0x9900-0x99FF): Skip if Bit in I/O Register Clear
        if ((opcode & 0xFF00) == 0x9900) {
            uint8_t io_offset = static_cast<uint8_t>((opcode >> 3) & 0x1F);
            uint8_t bit_index = static_cast<uint8_t>(opcode & 0x07);
            uint16_t data_addr = static_cast<uint16_t>(0x20 + io_offset);
            bool next_2word = false;
            if (pc + 1 < flash_size_) {
                u16 n = flash_[pc + 1];
                if ((n & 0xFE0F) == 0x9000 && (n & 0x0F) != 0x0F) next_2word = true;
                else if ((n & 0xFE0F) == 0x9200 && (n & 0x0F) != 0x0F) next_2word = true;
                else if ((n & 0xFE0E) == 0x940C) next_2word = true;
                else if ((n & 0xFE0E) == 0x940E) next_2word = true;
            }
            JIT_CALL_2ARG(&jit_read_data, static_cast<int32_t>(data_addr));
            buf.test8_imm(Reg8::al, static_cast<uint8_t>(1u << bit_index));
            auto taken = buf.jz_forward();  // JZ if bit CLEAR → skip taken
            auto done = buf.jmp_forward();  // NOT TAKEN: jump over taken path
            buf.patch_jz(taken);
            // TAKEN path
            emit_set_pc(buf, pc + 1 + (next_2word ? 2u : 1u));
            buf.add_membase_imm32(Reg64::r14, 80, next_2word ? 1u : 0u);
            buf.sub_membase_imm32(Reg64::r14, 72, 1);
            if (skip_patch_out) *skip_patch_out = buf.jmp_forward();
            buf.patch_jump(done);
            return 1;
        }

        // MUL (0x9C00-0x9FFF): Multiply Rd × Rr → R1:R0
        if ((opcode & 0xFC00) == 0x9C00) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            uint8_t rr = static_cast<uint8_t>((opcode & 0x0F) | ((opcode >> 5) & 0x10));
            JIT_CALL_3ARG(&jit_mul, static_cast<int32_t>(rd), static_cast<int32_t>(rr));
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // ADIW (0x9600-0x97FF): Add Immediate to Word
        if ((opcode & 0xFF00) == 0x9600) {
            uint8_t pair = static_cast<uint8_t>((opcode >> 4) & 0x03);
            uint8_t imm = static_cast<uint8_t>(((opcode >> 2) & 0x30) | (opcode & 0x0F));
            JIT_CALL_3ARG(&jit_adiw, static_cast<int32_t>(pair), static_cast<int32_t>(imm));
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        // SBIW (0x9700-0x97FF): Subtract Immediate from Word
        if ((opcode & 0xFF00) == 0x9700) {
            uint8_t pair = static_cast<uint8_t>((opcode >> 4) & 0x03);
            uint8_t imm = static_cast<uint8_t>(((opcode >> 2) & 0x30) | (opcode & 0x0F));
            JIT_CALL_3ARG(&jit_sbiw, static_cast<int32_t>(pair), static_cast<int32_t>(imm));
            buf.add_membase_imm32(Reg64::r14, 80, 1);
            emit_set_pc(buf, pc + 1);
            return 1;
        }

        return 0;
    }

    case 0xC000: { // RJMP
        if ((opcode & 0xF000) != 0xC000) return 0;
        int16_t disp = static_cast<int16_t>(opcode & 0x0FFF);
        if (disp & 0x0800) disp |= 0xF000;
        uint32_t target = pc + 1 + static_cast<uint32_t>(static_cast<int32_t>(disp));
        buf.add_membase_imm32(Reg64::r14, 80, 1);
        emit_set_pc(buf, target);
        block_ended = true;
        return 1;
    }

    case 0xD000: { // RCALL
        int16_t disp = static_cast<int16_t>(opcode & 0x0FFF);
        if (disp & 0x0800) disp |= 0xF000;
        uint32_t return_pc = pc + 1;
        // Push return address on stack
        JIT_CALL_2ARG(&jit_rcall, static_cast<int64_t>(return_pc));
        buf.add_membase_imm32(Reg64::r14, 80, 2);
        // Set PC to target
        uint32_t target = return_pc + static_cast<uint32_t>(static_cast<int32_t>(disp));
        emit_set_pc(buf, target);
        block_ended = true;
        return 1;
    }

    case 0xE000: { // LDI Rd, K
        uint8_t rd = reg4_high(opcode);
        uint8_t imm = imm8_ldi(opcode);
        buf.mov_membase_imm8(Reg64::r14, rd, imm);
        emit_set_pc(buf, pc + 1);
        return 1;
    }

    case 0xF000: {
        // BRBS (0xF000-0xF3FF) / BRBC (0xF400-0xF7FF)
        // AVR encoding: 1111 0skk kkkk kbbb
        //   s = 0 for BRBS (branch if bit SET), s = 1 for BRBC (branch if bit CLEAR)
        //   kkkkkkk = 7-bit signed displacement, bbb = SREG bit number
        if ((opcode & 0xFC00) == 0xF000) { // BRBS — branch if bit SET
            uint8_t bit = static_cast<uint8_t>(opcode & 0x07);
            int8_t disp = static_cast<int8_t>((opcode >> 3) & 0x7F);
            if (disp & 0x40) disp |= static_cast<int8_t>(0x80);
            uint32_t target = pc + 1 + static_cast<uint32_t>(static_cast<int32_t>(disp));

            buf.movzx_membase(Reg64::rax, Reg64::r14, 38);
            buf.test8_imm(Reg8::al, static_cast<uint8_t>(1 << bit));
            auto skip_label = buf.jnz_forward(); // JNZ if bit SET → branch taken
            emit_set_pc(buf, pc + 1);            // not taken
            auto done_label = buf.jmp_forward();
            buf.patch_jnz(skip_label);
            emit_set_pc(buf, target);            // taken
            buf.add_membase_imm32(Reg64::r14, 80, 1); // taken: +1 extra cycle
            buf.patch_jump(done_label);
            block_ended = true;
            return 1;
        }
        if ((opcode & 0xFC00) == 0xF400) { // BRBC — branch if bit CLEAR
            uint8_t bit = static_cast<uint8_t>(opcode & 0x07);
            int8_t disp = static_cast<int8_t>((opcode >> 3) & 0x7F);
            if (disp & 0x40) disp |= static_cast<int8_t>(0x80);
            uint32_t target = pc + 1 + static_cast<uint32_t>(static_cast<int32_t>(disp));

            buf.movzx_membase(Reg64::rax, Reg64::r14, 38);
            buf.test8_imm(Reg8::al, static_cast<uint8_t>(1 << bit));
            auto skip_label = buf.jz_forward();  // JZ if bit CLEAR → branch taken
            emit_set_pc(buf, pc + 1);            // not taken
            auto done_label = buf.jmp_forward();
            buf.patch_jz(skip_label);
            emit_set_pc(buf, target);            // taken
            buf.add_membase_imm32(Reg64::r14, 80, 1); // taken: +1 extra cycle
            buf.patch_jump(done_label);
            block_ended = true;
            return 1;
        }
        // SBRC: (opcode & 0xFE08) == 0xFC00 — skip if bit CLEAR in register
        if ((opcode & 0xFE08) == 0xFC00) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            uint8_t bit = static_cast<uint8_t>(opcode & 0x07);
            bool next_2word = false;
            if (pc + 1 < flash_size_) {
                u16 n = flash_[pc + 1];
                if ((n & 0xFE0F) == 0x9000 && (n & 0x0F) != 0x0F) next_2word = true;
                else if ((n & 0xFE0F) == 0x9200 && (n & 0x0F) != 0x0F) next_2word = true;
                else if ((n & 0xFE0E) == 0x940C) next_2word = true;
                else if ((n & 0xFE0E) == 0x940E) next_2word = true;
            }
            buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
            buf.test8_imm(Reg8::al, static_cast<uint8_t>(1 << bit));
            auto taken = buf.jz_forward();  // JZ if bit CLEAR → skip taken
            auto done = buf.jmp_forward();  // NOT TAKEN: jump over taken path
            buf.patch_jz(taken);
            // TAKEN path
            emit_set_pc(buf, pc + 1 + (next_2word ? 2u : 1u));
            buf.add_membase_imm32(Reg64::r14, 80, next_2word ? 1u : 0u);
            buf.sub_membase_imm32(Reg64::r14, 72, 1);
            if (skip_patch_out) *skip_patch_out = buf.jmp_forward();
            buf.patch_jump(done);
            return 1;
        }
        // SBRS: (opcode & 0xFE08) == 0xFE00 — skip if bit SET in register
        if ((opcode & 0xFE08) == 0xFE00) {
            uint8_t rd = static_cast<uint8_t>((opcode >> 4) & 0x1F);
            uint8_t bit = static_cast<uint8_t>(opcode & 0x07);
            bool next_2word = false;
            if (pc + 1 < flash_size_) {
                u16 n = flash_[pc + 1];
                if ((n & 0xFE0F) == 0x9000 && (n & 0x0F) != 0x0F) next_2word = true;
                else if ((n & 0xFE0F) == 0x9200 && (n & 0x0F) != 0x0F) next_2word = true;
                else if ((n & 0xFE0E) == 0x940C) next_2word = true;
                else if ((n & 0xFE0E) == 0x940E) next_2word = true;
            }
            buf.movzx_membase(Reg64::rax, Reg64::r14, rd);
            buf.test8_imm(Reg8::al, static_cast<uint8_t>(1 << bit));
            auto taken = buf.jnz_forward(); // JNZ if bit SET → skip taken
            auto done = buf.jmp_forward();  // NOT TAKEN: jump over taken path
            buf.patch_jnz(taken);
            // TAKEN path
            emit_set_pc(buf, pc + 1 + (next_2word ? 2u : 1u));
            buf.add_membase_imm32(Reg64::r14, 80, next_2word ? 1u : 0u);
            buf.sub_membase_imm32(Reg64::r14, 72, 1);
            if (skip_patch_out) *skip_patch_out = buf.jmp_forward();
            buf.patch_jump(done);
            return 1;
        }
        return 0;
    }

    default:
        return 0;
    }
}

// ---------------------------------------------------------------------------
// translate: build a basic block starting at start_pc
// ---------------------------------------------------------------------------
bool AvrJit::translate(uint32_t start_pc, const u16* flash, uint32_t flash_size,
                       u16 sreg_address) {
    flash_ = flash;
    flash_size_ = flash_size;
    sreg_address_ = sreg_address;
    translate_count_++;
    CodeBuffer buf;

    // Prologue
    buf.push(Reg64::r14);            // save r14 (callee-saved) + realign stack to 16 bytes
#ifdef _WIN32
    buf.mov(Reg64::r14, Reg64::rcx); // R14 = JitState* (first argument, Windows)
#else
    buf.mov(Reg64::r14, Reg64::rdi); // R14 = JitState* (first argument, SysV)
#endif

    uint32_t pc = start_pc;
    uint64_t block_cycles = 0;
    bool block_ended = false;
    uint32_t insn_count = 0;
    CodeBuffer::Label pending_skip_patch;
    bool has_pending_skip = false;
    constexpr uint32_t MAX_INSTRUCTIONS = 256;

    // Record block start for backward branch inlining
    block_start_pc_ = start_pc;
    block_start_offset_ = buf.size();

    while (pc < flash_size && !block_ended && insn_count < MAX_INSTRUCTIONS) {
        u16 opcode = flash[pc];

        // Detect backward conditional branch to block start → inline as native loop
        if (insn_count > 0 && ((opcode & 0xFC00) == 0xF000 || (opcode & 0xFC00) == 0xF400)) {
            bool is_brbc = (opcode & 0xFC00) == 0xF400;
            uint8_t bit = static_cast<uint8_t>(opcode & 0x07);
            int8_t disp = static_cast<int8_t>((opcode >> 3) & 0x7F);
            if (disp & 0x40) disp |= static_cast<int8_t>(0x80);
            uint32_t target = pc + 1 + static_cast<uint32_t>(static_cast<int32_t>(disp));
            if (target == block_start_pc_) {
                // Emit inline loop: test SREG bit, loop back if condition true
                // block_cycles/insn_count do not include this branch instruction yet
                uint32_t loop_cycles = static_cast<uint32_t>(block_cycles + 2);  // base + taken extra
                uint32_t loop_insns = static_cast<uint32_t>(insn_count + 1);
                buf.movzx_membase(Reg64::rax, Reg64::r14, 38);
                buf.test8_imm(Reg8::al, static_cast<uint8_t>(1 << bit));
                if (is_brbc) {
                    auto exit_label = buf.jz_forward();  // condition true → exit loop
                    // Taken (bit CLEAR): loop back
                    buf.add_membase_imm32(Reg64::r14, 80, loop_cycles);
                    buf.add_membase_imm32(Reg64::r14, 72, loop_insns);
                    int8_t back = static_cast<int8_t>(block_start_offset_ - buf.size() - 2);
                    buf.jmp_rel8(back);
                    buf.patch_jz(exit_label);
                    // Not taken: 1 cycle for the branch, fall through
                    buf.add_membase_imm32(Reg64::r14, 80, 1);
                    emit_set_pc(buf, pc + 1);
                } else {
                    auto exit_label = buf.jnz_forward(); // condition true → exit loop
                    // Taken (bit SET): loop back
                    buf.add_membase_imm32(Reg64::r14, 80, loop_cycles);
                    buf.add_membase_imm32(Reg64::r14, 72, loop_insns);
                    int8_t back = static_cast<int8_t>(block_start_offset_ - buf.size() - 2);
                    buf.jmp_rel8(back);
                    buf.patch_jnz(exit_label);
                    // Not taken: 1 cycle for the branch, fall through
                    buf.add_membase_imm32(Reg64::r14, 80, 1);
                    emit_set_pc(buf, pc + 1);
                }
                // Reset accumulators — post-loop instructions start fresh
                block_cycles = 0;
                insn_count = 0;
                pc += 1;
                continue;
            }
        }

        CodeBuffer::Label skip_patch_out;
        uint32_t words = translate_instruction(buf, opcode, pc, block_ended,
                                               &skip_patch_out);
        if (words == 0) {
            break;
        }
        // If a previous skip patch is pending, patch it now — the position is
        // right after the skipped instruction's code (the skip target).
        if (has_pending_skip) {
            buf.patch_jump(pending_skip_patch);
            has_pending_skip = false;
        }
        block_cycles += 1;
        pc += words;
        insn_count++;
        // Store new skip patch from this instruction (e.g. SBIS)
        if (skip_patch_out.resolved || skip_patch_out.patch_offset > 0) {
            pending_skip_patch = skip_patch_out;
            has_pending_skip = true;
        }
    }

    if (insn_count == 0) return false;

    // Epilogue: add base cycles, sync pending cycles to peripherals, update instructions_executed
    if (block_cycles > 0)
        buf.add_membase_imm32(Reg64::r14, 80, static_cast<uint32_t>(block_cycles));
    JIT_CALL_2ARG(&jit_block_end_tick, static_cast<int32_t>(insn_count));

    // If a skip patch is still pending, the skip target was never reached
    // (block ended early). Patch to epilogue so the taken path returns and
    // the run() loop picks up from the correct PC.
    if (has_pending_skip) {
        buf.patch_jump(pending_skip_patch);
    }

    buf.pop(Reg64::r14); // restore r14
    buf.ret();

    // Allocate executable memory and copy code
    size_t code_size = buf.size();
    void* exec_mem = allocate_executable(code_size);
    if (!exec_mem) return false;
    memcpy(exec_mem, buf.data(), code_size);

    JitBlock block;
    block.start_pc = start_pc;
    block.end_pc = pc;
    block.total_cycles = block_cycles;
    block.code = exec_mem;
    block.code_size = code_size;

    cache_.blocks[start_pc] = std::move(block);
    return true;
}

// ---------------------------------------------------------------------------
// execute: run a cached block
// ---------------------------------------------------------------------------
void AvrJit::execute(uint32_t start_pc, JitState* state) {
    auto it = cache_.blocks.find(start_pc);
    if (it == cache_.blocks.end()) return;
    execute_count_++;
    uint64_t cycles_before = state->cycles;
    it->second.func()(state);
    execute_cycles_ += state->cycles - cycles_before;
}

} // namespace vioavr::core::jit
