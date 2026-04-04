#!/usr/bin/env python3
"""
Convert compiled AVR binaries to C++ test files for VioAVR simulator.

Usage:
    python3 make_tests.py [--all] [program1 program2 ...]

Reads .bin files from tests/programs/, converts to flash words,
and generates C++ test files in tests/core/.
"""

import subprocess
import sys
from pathlib import Path

PROGRAMS_DIR = Path("tests/programs")
TESTS_DIR = Path("tests/core")
NOP_PADDING = 60  # Extra NOP words after program to allow run(N) to complete

def bin_to_words(bin_path: Path) -> list[int]:
    """Read binary file and convert to little-endian 16-bit words."""
    data = bin_path.read_bytes()
    
    # Pad to even length
    if len(data) % 2 != 0:
        data = data + b'\x00'
    
    words = []
    for i in range(0, len(data), 2):
        word = data[i] | (data[i + 1] << 8)
        words.append(word)
    
    return words

def generate_test(name: str, words: list[int], device: str = "atmega328") -> str:
    """Generate C++ test file from flash words."""
    
    # Add NOP padding
    padded = words + [0x0000] * NOP_PADDING
    
    # Format words as C++ initializer
    word_lines = []
    for i in range(0, len(padded), 8):
        chunk = padded[i:i+8]
        hex_vals = ", ".join(f"0x{w:04X}U" for w in chunk)
        word_lines.append("            " + hex_vals + ",")
    
    # Remove trailing comma
    word_lines[-1] = word_lines[-1].rstrip(",")
    
    content = f'''#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "vioavr/core/avr_cpu.hpp"
#include "vioavr/core/device.hpp"
#include "vioavr/core/hex_image.hpp"
#include "vioavr/core/memory_bus.hpp"
#include "vioavr/core/devices/{device}.hpp"

/*
 * Auto-generated test from compiled AVR program: {name}.c
 * Source: tests/programs/{name}.c
 * Binary: tests/programs/{name}.bin ({len(words)} words)
 *
 * Compiled with: avr-gcc -mmcu=atmega328p -Os -c {name}.c
 */

using namespace vioavr::core;

TEST_CASE("{name.replace('_', ' ').title()}")
{{
    MemoryBus bus {{devices::{device}}};
    AvrCpu cpu {{bus}};

    // Flash program words from compiled binary
    const HexImage image {{
        .flash_words = {{
{chr(10).join(word_lines)}
        }},
        .entry_word = 0U
    }};
    
    bus.load_image(image);
    cpu.reset();

    SUBCASE("Loads program") {{
        CHECK(cpu.state() == CpuState::running);
        CHECK(cpu.program_counter() == 0U);
        CHECK(bus.loaded_program_words() == {len(padded)}U);
    }}

    SUBCASE("Executes and halts") {{
        // Run with sufficient cycle budget
        cpu.run({len(padded)});
        
        // Should halt (PC at or beyond program, or in infinite loop)
        CHECK(cpu.program_counter() >= {len(words) - 1}U);
    }}
}}
'''
    return content

def process_program(name: str, device: str = "atmega328") -> bool:
    """Process a single program: compile, extract, generate test."""
    
    src = PROGRAMS_DIR / f"{name}.c"
    elf = PROGRAMS_DIR / f"{name}.elf"
    bin_path = PROGRAMS_DIR / f"{name}.bin"
    test_out = TESTS_DIR / f"cpu_{name}_generated_test.cpp"
    
    if not src.exists():
        print(f"  SKIP  {name}.c not found")
        return False
    
    print(f"  BUILD {name}.c")
    
    # Compile
    result = subprocess.run(
        ["avr-gcc", f"-mmcu=atmega328p", "-Os", "-c", str(src), "-o", str(elf)],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"  FAIL  {name}: {result.stderr[:100]}")
        return False
    
    # Extract binary
    result = subprocess.run(
        ["avr-objcopy", "-O", "binary", str(elf), str(bin_path)],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(f"  FAIL  objcopy {name}")
        return False
    
    # Convert to words
    words = bin_to_words(bin_path)
    print(f"  EXTRACT {len(words)} words from {bin_path.name}")
    
    # Generate test
    test_content = generate_test(name, words, device)
    test_out.write_text(test_content)
    print(f"  GEN   {test_out.name}")
    
    return True

def main():
    import argparse
    parser = argparse.ArgumentParser(description="Generate C++ tests from AVR programs")
    parser.add_argument("programs", nargs="*", help="Program names (without .c)")
    parser.add_argument("--all", action="store_true", help="Process all .c files")
    parser.add_argument("--device", default="atmega328", help="Target device")
    
    args = parser.parse_args()
    
    if args.all:
        programs = [f.stem for f in PROGRAMS_DIR.glob("*.c")]
    elif args.programs:
        programs = args.programs
    else:
        print("Error: specify --all or program names", file=sys.stderr)
        sys.exit(1)
    
    print(f"Processing {len(programs)} program(s)...\n")
    
    success = 0
    for name in sorted(programs):
        if process_program(name, args.device):
            success += 1
    
    print(f"\nDone: {success}/{len(programs)} programs processed")

if __name__ == "__main__":
    main()
