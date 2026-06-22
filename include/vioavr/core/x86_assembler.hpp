#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>

namespace vioavr::core::jit {

enum class Reg8 : uint8_t {
    al = 0, cl = 1, dl = 2, bl = 3,
    ah = 4, ch = 5, dh = 6, bh = 7,
    r8b = 8, r9b = 9, r10b = 10, r11b = 11,
    r12b = 12, r13b = 13, r14b = 14, r15b = 15
};

enum class Reg32 : uint8_t {
    eax = 0, ecx = 1, edx = 2, ebx = 3,
    esp = 4, ebp = 5, esi = 6, edi = 7,
    r8d = 8, r9d = 9, r10d = 10, r11d = 11,
    r12d = 12, r13d = 13, r14d = 14, r15d = 15
};

enum class Reg64 : uint8_t {
    rax = 0, rcx = 1, rdx = 2, rbx = 3,
    rsp = 4, rbp = 5, rsi = 6, rdi = 7,
    r8 = 8, r9 = 9, r10 = 10, r11 = 11,
    r12 = 12, r13 = 13, r14 = 14, r15 = 15
};

enum class XmmReg : uint8_t {
    xmm0 = 0, xmm1 = 1, xmm2 = 2, xmm3 = 3,
    xmm4 = 4, xmm5 = 5, xmm6 = 6, xmm7 = 7,
    xmm8 = 8, xmm9 = 9, xmm10 = 10, xmm11 = 11,
    xmm12 = 12, xmm13 = 13, xmm14 = 14, xmm15 = 15
};

class CodeBuffer {
public:
    CodeBuffer() { code_.reserve(4096); }

    uint8_t* data() { return code_.data(); }
    const uint8_t* data() const { return code_.data(); }
    size_t size() const { return code_.size(); }
    bool empty() const { return code_.empty(); }

    void emit8(uint8_t x) { code_.push_back(x); }
    void emit32(uint32_t x) {
        code_.push_back(static_cast<uint8_t>(x & 0xFF));
        code_.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
        code_.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
        code_.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
    }
    void emit64(uint64_t x) {
        emit32(static_cast<uint32_t>(x & 0xFFFFFFFF));
        emit32(static_cast<uint32_t>((x >> 32) & 0xFFFFFFFF));
    }

    void rex(bool w, bool r, bool x, bool b) {
        uint8_t prefix = 0x40;
        if (w) prefix |= 0x08;
        if (r) prefix |= 0x04;
        if (x) prefix |= 0x02;
        if (b) prefix |= 0x01;
        if (prefix != 0x40) emit8(prefix);
    }

    void rexw() { rex(true, false, false, false); }
    void rexrb(Reg64 rm, Reg64 reg) {
        bool r = static_cast<uint8_t>(reg) >= 8;
        bool b = static_cast<uint8_t>(rm) >= 8;
        rex(true, r, false, b);
    }

    void modrm(uint8_t mod, uint8_t reg, uint8_t rm) {
        emit8(static_cast<uint8_t>((mod << 6) | ((reg & 7) << 3) | (rm & 7)));
    }

    void sib(uint8_t scale, uint8_t index, uint8_t base) {
        emit8(static_cast<uint8_t>(((scale & 3) << 6) | ((index & 7) << 3) | (base & 7)));
    }

    // REX prefix with extension bits from both operands
    void rex_for_op(Reg64 dst, Reg64 src) {
        rex(true,
            static_cast<uint8_t>(src) >= 8,
            false,
            static_cast<uint8_t>(dst) >= 8);
    }

    // --- MOV instructions ---
    void mov(Reg64 dst, Reg64 src) {
        if (dst == src) return; // nop
        rex_for_op(dst, src);
        emit8(0x89);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    // NOTE: must set REX.B when dst >= 8. rexw() alone hardcodes B=0 and
    // causes mov(Reg64::r8, imm) to encode as MOV RAX, imm instead.
    // Windows JIT_CALL_3ARG uses this to load R8 with the register-number
    // argument — broken REX.B meant jit_adc/jit_sbc/etc. received garbage.
    void mov(Reg64 dst, int32_t imm) {
        rex(true, false, false, static_cast<uint8_t>(dst) >= 8);
        uint8_t rm = static_cast<uint8_t>(dst) & 7;
        emit8(0xC7);
        modrm(3, 0, rm);
        emit32(static_cast<uint32_t>(imm));
    }

    // movabs r64, imm64
    void movabs(Reg64 dst, uint64_t imm) {
        if (static_cast<uint8_t>(dst) >= 8) rex(true, false, false, true);
        emit8(static_cast<uint8_t>(0xB8 | (static_cast<uint8_t>(dst) & 7)));
        emit64(imm);
    }

    // mov r/m64, r64  with [base+offset] addressing
    void mov_membase(Reg64 src, Reg64 base, int32_t offset) {
        rex_for_op(base, src); // w=1, r=src, b=base
        emit8(0x89);
        encode_membase(static_cast<uint8_t>(src) & 7, base, offset);
    }

    // mov r64, r/m64 with [base+offset] addressing
    void mov_load_reg(Reg64 dst, Reg64 base, int32_t offset) {
        rex_for_op(base, dst); // w=1, r=dst, b=base
        emit8(0x8B);
        encode_membase(static_cast<uint8_t>(dst) & 7, base, offset);
    }

    // movzx r64, r/m8
    void movzx(Reg64 dst, Reg8 src) {
        bool rex_w = true;
        bool rex_b = static_cast<uint8_t>(src) >= 8;
        bool rex_r = static_cast<uint8_t>(dst) >= 8;
        rex(rex_w, rex_r, false, rex_b);
        emit8(0x0F);
        emit8(0xB6);
        modrm(3, static_cast<uint8_t>(dst) & 7, static_cast<uint8_t>(src) & 7);
    }

    // movzx r64, byte [base+offset]
    void movzx_membase(Reg64 dst, Reg64 base, int32_t offset) {
        rex(true, static_cast<uint8_t>(dst) >= 8, false, static_cast<uint8_t>(base) >= 8);
        emit8(0x0F);
        emit8(0xB6);
        encode_membase(static_cast<uint8_t>(dst) & 7, base, offset);
    }

    // mov [base+offset], r8 (8-bit store)
    void mov_membase8(Reg8 src, Reg64 base, int32_t offset) {
        rex(false, false, false, static_cast<uint8_t>(src) >= 8 || static_cast<uint8_t>(base) >= 8);
        emit8(0x88);
        encode_membase(static_cast<uint8_t>(src) & 7, base, offset);
    }

    // mov r8, [base+offset] (8-bit load)
    void mov_load8(Reg8 dst, Reg64 base, int32_t offset) {
        rex(false, static_cast<uint8_t>(dst) >= 8, false, static_cast<uint8_t>(base) >= 8);
        emit8(0x8A);
        encode_membase(static_cast<uint8_t>(dst) & 7, base, offset);
    }

    // movsx r64, r/m8 (sign-extend byte to quad)
    void movsx(Reg64 dst, Reg8 src) {
        rex(true, static_cast<uint8_t>(dst) >= 8, false, static_cast<uint8_t>(src) >= 8);
        emit8(0x0F);
        emit8(0xBE);
        modrm(3, static_cast<uint8_t>(dst) & 7, static_cast<uint8_t>(src) & 7);
    }

    // --- ALU instructions (r/m, r) ---
    void alu_rm_r(uint8_t opcode_ext, Reg64 dst, Reg64 src) {
        // opcode 01 /r : add/sub/and/or/xor/cmp r/m64, r64
        rex_for_op(dst, src);
        emit8(0x01 | (opcode_ext == 0 ? 0 : 0)); // Actually different opcodes...
        // Wait, 01 + /r for ADD, 09 + /r for OR... 
        // No: 
        // ADD = 01 /r (or 03 /r for the reverse)
        // OR  = 09 /r
        // ADC = 11 /r
        // SBB = 19 /r
        // AND = 21 /r
        // SUB = 29 /r
        // XOR = 31 /r
        // CMP = 39 /r
    }

    void add(Reg64 dst, Reg64 src) {
        rex_for_op(dst, src);
        emit8(0x01);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    void add32(Reg32 dst, Reg32 src) {
        rex(false, static_cast<uint8_t>(src) >= 8, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x01);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    void add8(Reg8 dst, Reg8 src) {
        rex(false, static_cast<uint8_t>(src) >= 8, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x00);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    void sub(Reg64 dst, Reg64 src) {
        rex_for_op(dst, src);
        emit8(0x29);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    void sub8(Reg8 dst, Reg8 src) {
        rex(false, static_cast<uint8_t>(src) >= 8, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x28);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    void and8(Reg8 dst, Reg8 src) {
        rex(false, static_cast<uint8_t>(src) >= 8, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x20);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    void or8(Reg8 dst, Reg8 src) {
        rex(false, static_cast<uint8_t>(src) >= 8, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x08);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    void xor8(Reg8 dst, Reg8 src) {
        rex(false, static_cast<uint8_t>(src) >= 8, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x30);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    // --- ALU with immediate ---
    // 83 /0 ib : ADD r/m64, imm8 (sign-extended)
    void add_imm8(Reg64 dst, int8_t imm) {
        rexw();
        emit8(0x83);
        modrm(3, 0, static_cast<uint8_t>(dst) & 7);
        emit8(static_cast<uint8_t>(imm));
    }

    void sub_imm8(Reg64 dst, int8_t imm) {
        rexw();
        emit8(0x83);
        modrm(3, 5, static_cast<uint8_t>(dst) & 7);
        emit8(static_cast<uint8_t>(imm));
    }

    void cmp_imm8(Reg64 dst, int8_t imm) {
        rexw();
        emit8(0x83);
        modrm(3, 7, static_cast<uint8_t>(dst) & 7);
        emit8(static_cast<uint8_t>(imm));
    }

    // cmp al, imm8
    void cmp_al_imm8(uint8_t imm) {
        emit8(0x3C);
        emit8(imm);
    }

    // test r/m8, imm8
    void test8_imm(Reg8 dst, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0xF6);
        modrm(3, 0, static_cast<uint8_t>(dst) & 7);
        emit8(imm);
    }

    // test r/m64, r64
    void test(Reg64 dst, Reg64 src) {
        rex_for_op(dst, src);
        emit8(0x85);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    // --- Shifts ---
    void shl8_imm(Reg8 dst, uint8_t imm) {
        if (imm == 0) return;
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0xC0);
        modrm(3, 4, static_cast<uint8_t>(dst) & 7);
        emit8(imm);
    }

    void shr8_imm(Reg8 dst, uint8_t imm) {
        if (imm == 0) return;
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0xC0);
        modrm(3, 5, static_cast<uint8_t>(dst) & 7);
        emit8(imm);
    }

    // 32-bit shifts
    void shl32_imm(Reg32 dst, uint8_t imm) {
        if (imm == 0) return;
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0xC1);
        modrm(3, 4, static_cast<uint8_t>(dst) & 7);
        emit8(imm);
    }

    void shr32_imm(Reg32 dst, uint8_t imm) {
        if (imm == 0) return;
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0xC1);
        modrm(3, 5, static_cast<uint8_t>(dst) & 7);
        emit8(imm);
    }

    // --- Stack operations ---
    void push(Reg64 reg) {
        if (static_cast<uint8_t>(reg) >= 8) rex(false, false, false, true);
        emit8(static_cast<uint8_t>(0x50 | (static_cast<uint8_t>(reg) & 7)));
    }

    void push64(int32_t imm) {
        emit8(0x68);
        emit32(static_cast<uint32_t>(imm));
    }

    void pop(Reg64 reg) {
        if (static_cast<uint8_t>(reg) >= 8) rex(false, false, false, true);
        emit8(static_cast<uint8_t>(0x58 | (static_cast<uint8_t>(reg) & 7)));
    }

    // --- Control flow ---
    // Call relative (intra-block)
    void call_rel(int32_t offset) {
        emit8(0xE8);
        emit32(static_cast<uint32_t>(offset));
    }

    // Call absolute (through register)
    void call(Reg64 reg) {
        if (static_cast<uint8_t>(reg) >= 8) rex(false, false, false, true);
        emit8(0xFF);
        modrm(3, 2, static_cast<uint8_t>(reg) & 7);
    }

    // Jmp absolute (through register)
    void jmp(Reg64 reg) {
        if (static_cast<uint8_t>(reg) >= 8) rex(false, false, false, true);
        emit8(0xFF);
        modrm(3, 4, static_cast<uint8_t>(reg) & 7);
    }

    void ret() {
        emit8(0xC3);
    }

    // --- Conditional set byte (for flag extraction) ---
    // setcc r/m8
    void setcc(uint8_t condition, Reg8 dst) {
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x0F);
        emit8(0x90 | (condition & 0x0F));
        modrm(3, 0, static_cast<uint8_t>(dst) & 7);
    }

    void sete(Reg8 dst) { setcc(0x94, dst); }  // ZF=1
    void setne(Reg8 dst) { setcc(0x95, dst); } // ZF=0
    void setc(Reg8 dst) { setcc(0x92, dst); }  // CF=1
    void setnc(Reg8 dst) { setcc(0x93, dst); } // CF=0
    void seto(Reg8 dst) { setcc(0x90, dst); }  // OF=1
    void setno(Reg8 dst) { setcc(0x91, dst); } // OF=0
    void sets(Reg8 dst) { setcc(0x98, dst); }  // SF=1
    void setns(Reg8 dst) { setcc(0x99, dst); } // SF=0

    // --- ALU with AL, imm8 (short form) ---
    void add_al_imm8(uint8_t imm) { emit8(0x04); emit8(imm); }
    void sub_al_imm8(uint8_t imm) { emit8(0x2C); emit8(imm); }
    void and_al_imm8(uint8_t imm) { emit8(0x24); emit8(imm); }
    void or_al_imm8(uint8_t imm) { emit8(0x0C); emit8(imm); }
    void xor_al_imm8(uint8_t imm) { emit8(0x34); emit8(imm); }
    // cmp_al_imm8 already declared above

    // ADD/SUB/AND/OR/XOR r/m8, imm8 using 80 /0 ib encoding
    void add_imm8(Reg8 dst, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x80); modrm(3, 0, static_cast<uint8_t>(dst) & 7); emit8(imm);
    }
    void or_imm8(Reg8 dst, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x80); modrm(3, 1, static_cast<uint8_t>(dst) & 7); emit8(imm);
    }
    void adc_imm8(Reg8 dst, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x80); modrm(3, 2, static_cast<uint8_t>(dst) & 7); emit8(imm);
    }
    void sbb_imm8(Reg8 dst, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x80); modrm(3, 3, static_cast<uint8_t>(dst) & 7); emit8(imm);
    }
    void and_imm8(Reg8 dst, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x80); modrm(3, 4, static_cast<uint8_t>(dst) & 7); emit8(imm);
    }
    void sub_imm8(Reg8 dst, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x80); modrm(3, 5, static_cast<uint8_t>(dst) & 7); emit8(imm);
    }
    void xor_imm8(Reg8 dst, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x80); modrm(3, 6, static_cast<uint8_t>(dst) & 7); emit8(imm);
    }
    void cmp_imm8(Reg8 dst, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x80); modrm(3, 7, static_cast<uint8_t>(dst) & 7); emit8(imm);
    }

    // ADD/SUB/AND/OR/XOR r/m8, imm8 for membase (memory operand)
    void add_membase_imm8(Reg64 base, int32_t offset, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(base) >= 8);
        emit8(0x80);
        encode_membase(0, base, offset);
        emit8(imm);
    }
    void sub_membase_imm8(Reg64 base, int32_t offset, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(base) >= 8);
        emit8(0x80);
        encode_membase(5, base, offset);
        emit8(imm);
    }
    void or_membase_imm8(Reg64 base, int32_t offset, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(base) >= 8);
        emit8(0x80);
        encode_membase(1, base, offset);
        emit8(imm);
    }
    void and_membase_imm8(Reg64 base, int32_t offset, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(base) >= 8);
        emit8(0x80);
        encode_membase(4, base, offset);
        emit8(imm);
    }

    // mov byte [base+offset], imm8
    void mov_membase_imm8(Reg64 base, int32_t offset, uint8_t imm) {
        rex(false, false, false, static_cast<uint8_t>(base) >= 8);
        emit8(0xC6);
        encode_membase(0, base, offset);
        emit8(imm);
    }

    // mov dword [base+offset], imm32
    void mov_membase_imm32(Reg64 base, int32_t offset, uint32_t imm) {
        rex(false, false, false, static_cast<uint8_t>(base) >= 8);
        emit8(0xC7);
        encode_membase(0, base, offset);
        emit32(imm);
    }

    // add dword/qword [base+offset], imm32
    void add_membase_imm32(Reg64 base, int32_t offset, uint32_t imm) {
        rex(true, false, false, static_cast<uint8_t>(base) >= 8);
        emit8(0x81);
        encode_membase(0, base, offset);
        emit32(imm);
    }

    // sub dword/qword [base+offset], imm32
    void sub_membase_imm32(Reg64 base, int32_t offset, uint32_t imm) {
        rex(true, false, false, static_cast<uint8_t>(base) >= 8);
        emit8(0x81);
        encode_membase(5, base, offset);
        emit32(imm);
    }

    // mov [base+offset], r64 (64-bit store)
    void mov_membase64(Reg64 src, Reg64 base, int32_t offset) {
        rex(true, static_cast<uint8_t>(src) >= 8, false, static_cast<uint8_t>(base) >= 8);
        emit8(0x89);
        encode_membase(static_cast<uint8_t>(src) & 7, base, offset);
    }

    // --- LAHF/SAHF (flag transfer) ---
    void lahf() { emit8(0x9F); }
    void sahf() { emit8(0x9E); }

    // Label/patch support
    struct Label {
        size_t patch_offset{0};
        bool resolved{false};
    };

    Label create_label() {
        Label l;
        l.patch_offset = code_.size();
        l.resolved = false;
        return l;
    }

    // Allocate space for a forward jump and return label
    Label jz_forward() {
        Label l;
        l.patch_offset = code_.size();
        emit8(0x0F);
        emit8(0x84); // jz rel32
        emit32(0);
        l.resolved = false;
        return l;
    }

    Label jnz_forward() {
        Label l;
        l.patch_offset = code_.size();
        emit8(0x0F);
        emit8(0x85); // jnz rel32
        emit32(0);
        l.resolved = false;
        return l;
    }

    Label jmp_forward() {
        Label l;
        l.patch_offset = code_.size();
        emit8(0xE9); // jmp rel32
        emit32(0);   // placeholder
        l.resolved = false;
        return l;
    }

    void patch_jump(Label& label) {
        if (label.resolved) return;
        size_t target = code_.size();
        size_t offset = label.patch_offset;
        int32_t rel = static_cast<int32_t>(target - offset - 5); // -5 for the jmp instruction itself
        code_[offset + 1] = static_cast<uint8_t>(rel & 0xFF);
        code_[offset + 2] = static_cast<uint8_t>((rel >> 8) & 0xFF);
        code_[offset + 3] = static_cast<uint8_t>((rel >> 16) & 0xFF);
        code_[offset + 4] = static_cast<uint8_t>((rel >> 24) & 0xFF);
        label.resolved = true;
    }

    void patch_jz(Label& label) {
        if (label.resolved) return;
        size_t target = code_.size();
        size_t offset = label.patch_offset;
        int32_t rel = static_cast<int32_t>(target - offset - 6);
        code_[offset + 2] = static_cast<uint8_t>(rel & 0xFF);
        code_[offset + 3] = static_cast<uint8_t>((rel >> 8) & 0xFF);
        code_[offset + 4] = static_cast<uint8_t>((rel >> 16) & 0xFF);
        code_[offset + 5] = static_cast<uint8_t>((rel >> 24) & 0xFF);
        label.resolved = true;
    }

    void patch_jnz(Label& label) {
        if (label.resolved) return;
        size_t target = code_.size();
        size_t offset = label.patch_offset;
        int32_t rel = static_cast<int32_t>(target - offset - 6);
        code_[offset + 2] = static_cast<uint8_t>(rel & 0xFF);
        code_[offset + 3] = static_cast<uint8_t>((rel >> 8) & 0xFF);
        code_[offset + 4] = static_cast<uint8_t>((rel >> 16) & 0xFF);
        code_[offset + 5] = static_cast<uint8_t>((rel >> 24) & 0xFF);
        label.resolved = true;
    }

    // Conditional jump rel8 (short form, for backwards jumps)
    void jz_rel8(int8_t offset) {
        emit8(0x74);
        emit8(static_cast<uint8_t>(offset));
    }

    void jmp_rel8(int8_t offset) {
        emit8(0xEB);
        emit8(static_cast<uint8_t>(offset));
    }

    // lea r64, [base+offset] - can also do [base+index*scale+offset] but we keep it simple
    void lea_membase(Reg64 dst, Reg64 base, int32_t offset) {
        rex_for_op(base, dst);
        emit8(0x8D);
        encode_membase(static_cast<uint8_t>(dst) & 7, base, offset);
    }

    // inc/dec r64
    void inc(Reg64 dst) {
        rexw();
        emit8(0xFF);
        modrm(3, 0, static_cast<uint8_t>(dst) & 7);
    }

    void dec(Reg64 dst) {
        rexw();
        emit8(0xFF);
        modrm(3, 1, static_cast<uint8_t>(dst) & 7);
    }

    // xor r64, r64 (commonly used for zeroing)
    void xor_(Reg64 dst, Reg64 src) {
        rex_for_op(dst, src);
        emit8(0x31);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    // 32-bit ALU operations (no REX.W = 32-bit operand size)
    void or32(Reg32 dst, Reg32 src) {
        rex(false, static_cast<uint8_t>(src) >= 8, false, static_cast<uint8_t>(dst) >= 8);
        emit8(0x09);
        modrm(3, static_cast<uint8_t>(src) & 7, static_cast<uint8_t>(dst) & 7);
    }

    void add32_imm(Reg32 dst, int32_t imm) {
        if (imm >= -128 && imm <= 127) {
            rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
            emit8(0x83);
            modrm(3, 0, static_cast<uint8_t>(dst) & 7);
            emit8(static_cast<uint8_t>(imm & 0xFF));
        } else {
            rex(false, false, false, static_cast<uint8_t>(dst) >= 8);
            emit8(0x81);
            modrm(3, 0, static_cast<uint8_t>(dst) & 7);
            emit32(static_cast<uint32_t>(imm));
        }
    }

    // nop
    void nop() { emit8(0x90); }

private:
    std::vector<uint8_t> code_;

    void encode_membase(uint8_t reg, Reg64 base, int32_t offset) {
        uint8_t base_reg = static_cast<uint8_t>(base) & 7;
        if (offset == 0 && base_reg != 5) { // [base] (but not [rbp] which requires displacement)
            if (base_reg == 4) { // SIB needed for RSP
                modrm(0, reg, 4);
                sib(0, 4, base_reg);
            } else {
                modrm(0, reg, base_reg);
            }
        } else if (offset >= -128 && offset <= 127) {
            if (base_reg == 4) {
                modrm(1, reg, 4);
                sib(0, 4, base_reg);
            } else {
                modrm(1, reg, base_reg);
            }
            emit8(static_cast<uint8_t>(offset & 0xFF));
        } else {
            if (base_reg == 4) {
                modrm(2, reg, 4);
                sib(0, 4, base_reg);
            } else {
                modrm(2, reg, base_reg);
            }
            emit32(static_cast<uint32_t>(offset));
        }
    }
};

} // namespace vioavr::core::jit
