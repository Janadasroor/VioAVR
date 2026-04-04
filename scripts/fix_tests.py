#!/usr/bin/env python3
"""Add NOP padding to test files that use cpu.run() with small programs."""
import re
import sys
from pathlib import Path

TESTS_DIR = Path("tests/core")

def add_nop_padding(filepath: Path, target_words: int = 120) -> bool:
    """Add NOP padding to a test file's flash_words array."""
    content = filepath.read_text()
    
    # Find flash_words array
    # Look for pattern: .flash_words = { ... }
    match = re.search(r'(\.flash_words\s*=\s*\{)(.*?)(\},\s*\.entry_word)', content, re.DOTALL)
    if not match:
        return False
    
    prefix = match.group(1)
    existing = match.group(2)
    suffix = match.group(3)
    
    # Count existing words (lines with 0x or encode_)
    word_count = len([line for line in existing.strip().split('\n') 
                     if '0x' in line or 'encode_' in line])
    
    if word_count >= target_words:
        return False  # Already has enough words
    
    # Add NOP padding
    needed = target_words - word_count
    nop_lines = []
    for i in range(0, needed, 8):
        count = min(8, needed - i)
        nop_lines.append("            " + ", ".join(["0x0000U"] * count) + ",")
    
    new_content = content[:match.start()] + prefix + existing + "\n" + "\n".join(nop_lines) + "\n        " + suffix + content[match.end():]
    
    filepath.write_text(new_content)
    return True

def main():
    test_files = list(TESTS_DIR.glob("cpu_*.cpp")) + list(TESTS_DIR.glob("smoke_test.cpp"))
    fixed_count = 0
    
    for tf in test_files:
        if add_nop_padding(tf):
            print(f"Fixed: {tf.name}")
            fixed_count += 1
    
    print(f"\nFixed {fixed_count} test files")

if __name__ == "__main__":
    main()
