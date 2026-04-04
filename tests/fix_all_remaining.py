#!/usr/bin/env python3
"""
Fix all cpu.run(N) calls in test files by analyzing the flash_words array
and replacing with explicit step_to(cpu, TARGET_PC) calls.

Strategy:
- Parse flash_words to count actual instruction words (not 0x0000 NOPs)
- For "cpu.run(N); cpu.step();" → "step_to(cpu, N); cpu.step();" (execute instruction N)
- For "cpu.run(N); // comment\n        auto s = cpu.snapshot();" → "step_to(cpu, N);"
  The test expects state AFTER N instructions have executed, so PC should be at N.
- For "cpu.run(N); // Run until halt" → "step_to(cpu, N);"
- Add step_to() helper if not present
"""

import re
from pathlib import Path

TESTS = Path("tests/core")

HELPER = """
void step_to(AvrCpu& cpu, u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) {
        cpu.step();
    }
}
"""

def has_step_to(content: str) -> bool:
    return 'void step_to(' in content or 'step_to(cpu,' in content

def fix_file(filepath: Path) -> int:
    content = filepath.read_text()
    
    if has_step_to(content):
        return 0
    
    original_lines = content.split('\n')
    
    # Find the flash_words section to understand program structure
    in_flash = False
    flash_start_line = -1
    flash_words = []
    
    for i, line in enumerate(original_lines):
        if '.flash_words' in line or 'flash_words' in line:
            in_flash = True
            flash_start_line = i
        if in_flash:
            # Extract hex values
            matches = re.findall(r'0x([0-9A-Fa-f]+)U?', line)
            flash_words.extend([int(m, 16) for m in matches])
            if '}' in line and i > flash_start_line:
                in_flash = False
    
    total_words = len(flash_words)
    changes = 0
    
    def add_helper():
        nonlocal content
        if 'void step_to(' not in content:
            content = content.replace('\n} // namespace\n', HELPER + '\n} // namespace\n')
    
    # Pattern 1: cpu.run(N); // COMMENT\n        cpu.step(); // COMMENT
    # → step_to(cpu, N); cpu.step();
    p1 = r'(cpu\.run)\((\d+)\)(;\s*//[^\n]*)\n(\s+)cpu\.step\(\);'
    def repl1(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(2)}U);{m.group(3)}\n{m.group(4)}cpu.step();'
    content = re.sub(p1, repl1, content)
    
    # Pattern 2: cpu.run(N); // COMMENT\n        cpu.step(); (no comment on step)
    p2 = r'(cpu\.run)\((\d+)\)(;\s*//[^\n]*)\n(\s+)cpu\.step\(\);'
    if changes > 0:
        pass  # already handled above
    else:
        content = re.sub(p2, repl1, content)
    
    # Pattern 3: cpu.run(N); cpu.step(); (no comments)
    p3 = r'(cpu\.run)\((\d+)\)(;)\s*cpu\.step\(\);'
    def repl3(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(2)}U);{m.group(3)} cpu.step();'
    content = re.sub(p3, repl3, content)
    
    # Pattern 4: cpu.run(N); // Run until halt\n
    p4 = r'(cpu\.run)\((\d+)\)(;\s*// Run until halt)'
    def repl4(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(2)}U);{m.group(3)}'
    content = re.sub(p4, repl4, content)
    
    # Pattern 5: cpu.run(N); // Run to halt
    p5 = r'(cpu\.run)\((\d+)\)(;\s*// Run to halt)'
    def repl5(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(2)}U);{m.group(3)}'
    content = re.sub(p5, repl5, content)
    
    # Pattern 6: cpu.run(N); // Reach COMMENT\n        auto s = cpu.snapshot();
    # → step_to(cpu, N); auto s = cpu.snapshot();
    p6 = r'(cpu\.run)\((\d+)\)(;\s*// Reach[^\n]*)\n(\s+)auto s = cpu\.snapshot\(\);'
    def repl6(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(2)}U);{m.group(3)}\n{m.group(4)}auto s = cpu.snapshot();'
    content = re.sub(p6, repl6, content)
    
    # Pattern 7: cpu.run(N); // COMMENT\n        auto s =
    # (generic, catch remaining auto s patterns)
    p7 = r'(cpu\.run)\((\d+)\)(;\s*//[^\n]*)\n(\s+)auto s = (?!cpu\.snapshot)'
    if 'step_to(cpu,' in content:
        pass
    else:
        def repl7(m):
            nonlocal changes
            changes += 1
            return f'step_to(cpu, {m.group(2)}U);{m.group(3)}\n{m.group(4)}auto s = '
        content = re.sub(p7, repl7, content)
    
    # Pattern 8: cpu.run(N); // COMMENT (followed by CHECK)
    p8 = r'(cpu\.run)\((\d+)\)(;\s*//[^\n]*)\n(\s+)CHECK'
    def repl8(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(2)}U);{m.group(3)}\n{m.group(4)}CHECK'
    content = re.sub(p8, repl8, content)
    
    # Pattern 9: cpu.run(N); (no comment, followed by CHECK)
    p9 = r'(cpu\.run)\((\d+)\)(;)\n(\s+)CHECK'
    def repl9(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(2)}U);{m.group(3)}\n{m.group(4)}CHECK'
    content = re.sub(p9, repl9, content)
    
    # Pattern 10: cpu.run(N); // COMMENT\n        CHECK (no auto s)
    p10 = r'(cpu\.run)\((\d+)\)(;\s*//[^\n]*)\n(\s+)CHECK'
    def repl10(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(2)}U);{m.group(3)}\n{m.group(4)}CHECK'
    content = re.sub(p10, repl10, content)
    
    if changes > 0:
        add_helper()
        filepath.write_text(content)
    
    return changes

# Process all failing test files
files_to_fix = [
    "cpu_small_ops_test.cpp",
    "cpu_compare_test.cpp",
    "cpu_stack_test.cpp",
    "cpu_fmul_test.cpp",
    "cpu_interrupt_test.cpp",
    "cpu_interrupt_priority_test.cpp",
    "cpu_interrupt_chaining_test.cpp",
    "cpu_ext_interrupt_test.cpp",
    "cpu_ext_interrupt_firmware_test.cpp",
    "cpu_pin_change_interrupt_test.cpp",
    "cpu_pin_change_interrupt_firmware_test.cpp",
    "cpu_uart_test.cpp",
    "cpu_uart_interrupt_test.cpp",
    "cpu_uart_firmware_test.cpp",
    "cpu_timer_pwm_test.cpp",
    "cpu_timer_control_test.cpp",
    "cpu_timer_dual_interrupt_test.cpp",
    "cpu_timer_external_clock_firmware_test.cpp",
    "cpu_watchdog_test.cpp",
    "cpu_sleep_wake_test.cpp",
    "cpu_io_test.cpp",
    "cpu_gpio_sync_test.cpp",
    "cpu_voltage_interrupt_test.cpp",
    "cpu_bit_io_test.cpp",
    "cpu_branch_alias_test.cpp",
    "cpu_extended_load_store_test.cpp",
    "cpu_load_store_test.cpp",
    "cpu_multibyte_compare_test.cpp",
    "cpu_register_mapping_test.cpp",
    "cpu_adc_firmware_test.cpp",
    "cpu_adc_descriptor_test.cpp",
    "cpu_adc_auto_trigger_test.cpp",
    "cpu_adc_auto_trigger_firmware_test.cpp",
    "cpu_adc_ext_interrupt_auto_trigger_test.cpp",
    "cpu_adc_ext_interrupt_auto_trigger_firmware_test.cpp",
    "cpu_adc_timer_auto_trigger_test.cpp",
    "cpu_adc_timer_auto_trigger_firmware_test.cpp",
    "cpu_adc_timer_overflow_auto_trigger_test.cpp",
    "cpu_adc_timer_overflow_auto_trigger_firmware_test.cpp",
    "cpu_analog_comparator_firmware_test.cpp",
    "cpu_analog_digital_frontend_test.cpp",
    "cpu_analog_frontend_test.cpp",
]

total_changes = 0
fixed_count = 0

for name in files_to_fix:
    f = TESTS / name
    if not f.exists():
        continue
    
    c = fix_file(f)
    if c > 0:
        print(f"  {name}: {c} changes")
        total_changes += c
        fixed_count += 1

print(f"\nTotal: {total_changes} changes across {fixed_count} files")
