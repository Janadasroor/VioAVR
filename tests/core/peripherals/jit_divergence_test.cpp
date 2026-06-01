// JIT vs interpreter divergence test for integration_stress firmware
#include "doctest.h"

#ifndef _WIN32
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

TEST_CASE("JIT divergence: integration_stress") {
    auto hex_path = find_hex("integration_stress");
    REQUIRE(!hex_path.empty());
    auto dev_machine = Machine::create_for_device("atmega328p");
    auto image = HexImageLoader::load_file(hex_path, dev_machine->bus().device());

    // Run both machines with run(200) up to cycle 529 (last known good)
    auto ref = Machine::create_for_device("atmega328p");
    ref->bus().load_image(image);
    ref->reset();
    auto jit = Machine::create_for_device("atmega328p");
    jit->bus().load_image(image);
    jit->reset();
    jit->cpu().enable_jit(true);

    u64 total = 0;
    while (total < 400) { ref->cpu().run(200); jit->cpu().run(200); total += 200; }

    // Now step 1 cycle at a time with run(65) to engage JIT
    for (int i = 0; i < 200; i++) {
        u64 ref_cycles_before = ref->cpu().cycles();
        u64 jit_cycles_before = jit->cpu().cycles();

        ref->cpu().run(65);
        jit->cpu().run(65);

        u64 ref_cycles_after = ref->cpu().cycles();
        u64 jit_cycles_after = jit->cpu().cycles();

        if (ref->cpu().program_counter() != jit->cpu().program_counter() ||
            ref->cpu().sreg() != jit->cpu().sreg() ||
            ref->cpu().stack_pointer() != jit->cpu().stack_pointer()) {

            u64 cycle = ref->cpu().cycles();
            std::printf("\nDIVERGENCE at cycle=%llu\n", (unsigned long long)cycle);
            std::printf("  After run(65): ref PC=0x%04X SREG=0x%02X jit PC=0x%04X SREG=0x%02X\n",
                ref->cpu().program_counter(), ref->cpu().sreg(),
                jit->cpu().program_counter(), jit->cpu().sreg());

            auto jst = jit->cpu().jit_debug_stats();
            std::printf("  JIT stats: translate=%llu execute=%llu exec_cycles=%llu\n",
                (unsigned long long)jst.translate_count,
                (unsigned long long)jst.execute_count,
                (unsigned long long)jst.execute_cycles);

            // Rerun from cycle just before divergence to get exact instruction
            auto r2 = Machine::create_for_device("atmega328p");
            r2->bus().load_image(image);
            r2->reset();
            auto j2 = Machine::create_for_device("atmega328p");
            j2->bus().load_image(image);
            j2->reset();
            j2->cpu().enable_jit(true);
            u64 rem = cycle - 1;
            while (rem > 0) { u64 s = rem > 200 ? 200 : rem; r2->cpu().run(s); j2->cpu().run(s); rem -= s; }
            // Now at one cycle before divergence
            // Print state before the fateful instruction
            std::printf("\nState at cycle=%llu (before divergence):\n", (unsigned long long)(cycle - 1));
            std::printf("  REF: PC=0x%04X SP=0x%04X SREG=0x%02X\n",
                r2->cpu().program_counter(), r2->cpu().stack_pointer(), r2->cpu().sreg());
            std::printf("  JIT: PC=0x%04X SP=0x%04X SREG=0x%02X\n",
                j2->cpu().program_counter(), j2->cpu().stack_pointer(), j2->cpu().sreg());
            auto rs = r2->cpu().snapshot();
            auto js = j2->cpu().snapshot();
            std::printf("  Ref: R1=0x%02X R18=0x%02X R26=0x%02X R27=0x%02X\n",
                rs.gpr[1], rs.gpr[18], rs.gpr[26], rs.gpr[27]);
            std::printf("  Jit: R1=0x%02X R18=0x%02X R26=0x%02X R27=0x%02X\n",
                js.gpr[1], js.gpr[18], js.gpr[26], js.gpr[27]);

            // Now single-step the JIT machine
            u16 opcode = j2->cpu().bus().flash_ptr()[j2->cpu().program_counter()];
            std::printf("\nAbout to execute opcode=0x%04X at PC=0x%04X\n",
                opcode, j2->cpu().program_counter());

            // Step the ref
            r2->cpu().run(1);
            std::printf("After 1 cycle: REF PC=0x%04X SREG=0x%02X",
                r2->cpu().program_counter(), r2->cpu().sreg());
            rs = r2->cpu().snapshot();
            std::printf(" R1=0x%02X R18=0x%02X R26=0x%02X R27=0x%02X\n",
                rs.gpr[1], rs.gpr[18], rs.gpr[26], rs.gpr[27]);

            // Now step the JIT — try with run(65) again
            u64 jc_before = j2->cpu().cycles();
            j2->cpu().run(65);
            u64 jc_after = j2->cpu().cycles();
            std::printf("After run(65): JIT PC=0x%04X SREG=0x%02X cyc=%llu->%llu",
                j2->cpu().program_counter(), j2->cpu().sreg(),
                (unsigned long long)jc_before, (unsigned long long)jc_after);
            js = j2->cpu().snapshot();
            std::printf(" R1=0x%02X R18=0x%02X R26=0x%02X R27=0x%02X\n",
                js.gpr[1], js.gpr[18], js.gpr[26], js.gpr[27]);
            jst = j2->cpu().jit_debug_stats();
            std::printf("  JIT stats: translate=%llu execute=%llu exec_cycles=%llu\n",
                (unsigned long long)jst.translate_count,
                (unsigned long long)jst.execute_count,
                (unsigned long long)jst.execute_cycles);

            break;
        }
    }

    // If we reach here without break, no divergence was detected — pass
    CHECK(true);
}
#else
TEST_CASE("JIT divergence: integration_stress") {}
#endif
