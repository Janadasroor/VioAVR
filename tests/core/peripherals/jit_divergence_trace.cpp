#include "doctest.h"
#ifdef VIOAVR_HAVE_JIT
#include "vioavr/core/machine.hpp"
#include "vioavr/core/hex_image.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <cstdio>

using namespace vioavr::core;

TEST_CASE("JIT trace: integration_stress") {
    auto hex_path = find_hex("integration_stress");
    REQUIRE(!hex_path.empty());
    auto dev = Machine::create_for_device("atmega328p");
    auto image = HexImageLoader::load_file(hex_path, dev->bus().device());

    auto ref = Machine::create_for_device("atmega328p");
    ref->bus().load_image(image);
    ref->reset();

    auto jit = Machine::create_for_device("atmega328p");
    jit->bus().load_image(image);
    jit->reset();
    jit->cpu().enable_jit(true);

    // Step one cycle at a time until divergence
    u64 max_cycles = 10000;
    u64 divergence_cycle = 0;
    for (u64 c = 0; c < max_cycles; ) {
        ref->cpu().run(1);
        jit->cpu().run(1);
        u64 refc = ref->cpu().cycles();
        u64 jitc = jit->cpu().cycles();
        // Align to min cycle to compare
        u64 minc = refc < jitc ? refc : jitc;
        if (refc == minc && jitc == minc) {
            c = minc;
            if (ref->cpu().program_counter() != jit->cpu().program_counter() ||
                ref->cpu().stack_pointer() != jit->cpu().stack_pointer() ||
                ref->cpu().sreg() != jit->cpu().sreg()) {
                divergence_cycle = c;
                printf("DIVERGENCE at cycle=%llu\n", (unsigned long long)c);
                printf("REF: PC=0x%04X SP=0x%04X SREG=0x%02X\n",
                    ref->cpu().program_counter(), ref->cpu().stack_pointer(), ref->cpu().sreg());
                printf("JIT: PC=0x%04X SP=0x%04X SREG=0x%02X\n",
                    jit->cpu().program_counter(), jit->cpu().stack_pointer(), jit->cpu().sreg());
                break;
            }
        }
    }
    CHECK(divergence_cycle == 0);
}
#else
TEST_CASE("JIT trace: integration_stress") {}
#endif
