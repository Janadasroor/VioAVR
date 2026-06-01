#include "doctest.h"

#ifdef VIOAVR_HAVE_JIT
#include "vioavr/core/machine.hpp"
#include "vioavr/core/hex_image.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <cstdio>

using namespace vioavr::core;

static std::string find_hex(const std::string& name) {
    std::string paths[] = {
        "build/tests/" + name + ".hex",
        "../" + name + ".hex",
        "../../build/tests/" + name + ".hex",
        "../../tests/" + name + ".hex",
    };
    for (auto& p : paths) {
        if (std::filesystem::exists(p)) return p;
    }
    return "";
}

TEST_CASE("JIT divergence: pin down") {
    auto hex_path = find_hex("integration_stress");
    REQUIRE(!hex_path.empty());
    auto dev_machine = Machine::create_for_device("atmega328p");
    auto image = HexImageLoader::load_file(hex_path, dev_machine->bus().device());

    // Run both in lockstep: each iteration advances ref by 1, then JIT up to match
    auto ref = Machine::create_for_device("atmega328p");
    ref->bus().load_image(image);
    ref->reset();
    auto jit = Machine::create_for_device("atmega328p");
    jit->bus().load_image(image);
    jit->reset();
    jit->cpu().enable_jit(true);

    // Fast-forward to ~850
    u64 ff = 850;
    while (ff > 0) { u64 s = ff > 200 ? 200 : ff; ref->cpu().run(s); jit->cpu().run(s); ff -= s; }

    u64 ref_cyc = ref->cpu().cycles();
    u64 jit_cyc = jit->cpu().cycles();

    for (int step = 0; step < 200; step++) {
        // Advance ref by exactly 1 cycle
        ref->cpu().run(1);
        ref_cyc = ref->cpu().cycles();

        // Advance JIT to match ref's cycle count (or exceed it)
        u64 budget = ref_cyc > jit_cyc ? ref_cyc - jit_cyc : 0;
        if (budget > 0) {
            jit->cpu().run(budget);
            jit_cyc = jit->cpu().cycles();
        }

        // JIT may have overshot; advance ref to match
        if (jit_cyc > ref_cyc) {
            ref->cpu().run(jit_cyc - ref_cyc);
            ref_cyc = jit_cyc;
        }

        // Now ref_cyc == jit_cyc exactly — compare
        bool pc_ok = (ref->cpu().program_counter() == jit->cpu().program_counter());
        bool sp_ok = (ref->cpu().stack_pointer() == jit->cpu().stack_pointer());
        bool sreg_ok = (ref->cpu().sreg() == jit->cpu().sreg());
        bool gpr_ok = true;
        auto rs = ref->cpu().snapshot();
        auto js = jit->cpu().snapshot();
        for (int r = 0; r < 32; r++) {
            if (rs.gpr[r] != js.gpr[r]) { gpr_ok = false; break; }
        }

        if (!pc_ok || !sp_ok || !sreg_ok || !gpr_ok) {
            std::printf("\n=== DIVERGENCE at cycle %llu ===\n", (unsigned long long)ref_cyc);
            auto ro = ref->cpu().bus().flash_ptr()[ref->cpu().program_counter()];
            auto jo = jit->cpu().bus().flash_ptr()[jit->cpu().program_counter()];
            std::printf("  REF: PC=0x%04X opcode=0x%04X SP=0x%04X SREG=0x%02X\n",
                ref->cpu().program_counter(), ro, ref->cpu().stack_pointer(), ref->cpu().sreg());
            std::printf("  JIT: PC=0x%04X opcode=0x%04X SP=0x%04X SREG=0x%02X\n",
                jit->cpu().program_counter(), jo, jit->cpu().stack_pointer(), jit->cpu().sreg());
            for (int r = 0; r < 32; r++) {
                if (rs.gpr[r] != js.gpr[r])
                    std::printf("  R%-2d: REF=0x%02X JIT=0x%02X\n", r, rs.gpr[r], js.gpr[r]);
            }
            auto jst = jit->cpu().jit_debug_stats();
            std::printf("  JIT stats: translate=%llu execute=%llu exec_cycles=%llu\n",
                (unsigned long long)jst.translate_count,
                (unsigned long long)jst.execute_count,
                (unsigned long long)jst.execute_cycles);

            // Show ref instruction trace around divergence
            std::printf("\n  Ref trace (last 5 instructions):\n");
            auto r2 = Machine::create_for_device("atmega328p");
            r2->bus().load_image(image);
            r2->reset();
            u64 trace_start = ref_cyc > 5 ? ref_cyc - 5 : 0;
            u64 tref = trace_start;
            while (tref > 0) { u64 s = tref > 200 ? 200 : tref; r2->cpu().run(s); tref -= s; }
            for (int j = 0; j < 5; j++) {
                u16 op = r2->cpu().bus().flash_ptr()[r2->cpu().program_counter()];
                // Decode opcode to mnemonic
                u16 high = op & 0xF000;
                const char* mn = "???";
                if (op == 0x0000) mn = "NOP";
                else if (high == 0xE000) mn = "LDI";
                else if ((op & 0xFC00) == 0x0C00) mn = "ADD";
                else if ((op & 0xFC00) == 0x1800) mn = "SUB";
                else if ((op & 0xFC00) == 0x2000) mn = "AND";
                else if ((op & 0xFC00) == 0x2400) mn = "EOR";
                else if ((op & 0xFC00) == 0x2800) mn = "OR";
                else if ((op & 0xF000) == 0xC000) mn = "RJMP";
                else if ((op & 0xF000) == 0xD000) mn = "RCALL";
                else if (op == 0x9508) mn = "RET";
                else if ((op & 0xFE08) == 0xFC00) mn = "SBRC";
                else if ((op & 0xFE08) == 0xFE00) mn = "SBRS";
                else if ((op & 0xF800) == 0xB000) mn = "IN";
                else if ((op & 0xF800) == 0xB800) mn = "OUT";
                else if ((op & 0xFE0F) == 0x920F) mn = "PUSH";
                else if ((op & 0xFE0F) == 0x900F) mn = "POP";
                else if ((op & 0xFE00) == 0x9600) mn = "ADIW";
                else if ((op & 0xFE00) == 0x9700) mn = "SBIW";
                else if ((op & 0xFF00) == 0x0100) mn = "MOVW";
                else if ((op & 0xFC00) == 0x0400) mn = "CPC";
                else if ((op & 0xFC00) == 0x1400) mn = "CP";
                else if ((op & 0xFC00) == 0x0800) mn = "SBC";
                else if ((op & 0xFC00) == 0x1C00) mn = "ADC";
                else if ((op & 0xF000) == 0x3000) mn = "CPI";
                else if ((op & 0xF000) == 0x5000) mn = "SUBI";
                else if ((op & 0xF000) == 0x6000) mn = "ORI";
                else if ((op & 0xF000) == 0x7000) mn = "ANDI";
                else if ((op & 0xFE0E) == 0x940E) mn = "CALL";
                else if ((op & 0xFE0F) == 0x9400) mn = "COM";
                else if ((op & 0xFE0F) == 0x9401) mn = "NEG";
                else if ((op & 0xFE0F) == 0x9403) mn = "INC";
                else if ((op & 0xFE0F) == 0x940A) mn = "DEC";
                else if ((op & 0xFE0F) == 0x9406) mn = "LSR";
                else if ((op & 0xFE0F) == 0x9405) mn = "ASR";
                else if ((op & 0xFE0F) == 0x9407) mn = "ROR";

                std::printf("    cycle %llu: PC=0x%04X %s SP=0x%04X\n",
                    (unsigned long long)r2->cpu().cycles(),
                    r2->cpu().program_counter(), mn, r2->cpu().stack_pointer());
                r2->cpu().run(1);
            }
            break;
        }

        if (step % 20 == 0) {
            std::printf("  OK through cycle %llu (PC=0x%04X)\n",
                (unsigned long long)ref_cyc, ref->cpu().program_counter());
        }
    }
}
#else
TEST_CASE("JIT divergence: pin down") {}
#endif
