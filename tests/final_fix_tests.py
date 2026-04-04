#!/usr/bin/env python3
"""Final comprehensive fix for all remaining cpu.run(N) calls in test files."""
import re
from pathlib import Path

TESTS = Path("tests/core")

HELPER_ANON_NS = """
void step_to(AvrCpu& cpu, u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != CpuState::halted) {
        cpu.step();
    }
}
"""

HELPER_QUALIFIED = """
void step_to(vioavr::core::AvrCpu& cpu, vioavr::core::u32 target_pc) {
    while (cpu.program_counter() < target_pc && cpu.state() != vioavr::core::CpuState::halted) {
        cpu.step();
    }
}
"""

def fix_file(filepath: Path) -> int:
    content = filepath.read_text()
    original = content
    changes = 0
    
    # Skip if already has step_to
    if 'void step_to(' in content:
        return 0
    
    # Skip files that genuinely need cycle-based runs (not navigation)
    skip_patterns = ['cpu_watchdog', 'cpu_control_state', 'cpu_indirect_control', 
                     'cpu_timer_gpio', 'cpu_spm_boundary', 'sync_engine', 'smoke_test']
    for sp in skip_patterns:
        if sp in filepath.name:
            return 0
    
    # Determine if file has anonymous namespace
    has_anon_ns = '} // namespace' in content or '}  // namespace' in content
    
    # Pattern 1: cpu.run(N); cpu.step();
    p1 = r'cpu\.run\((\d+)\);\s*cpu\.step\(\);'
    def repl1(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(1)}U); cpu.step();'
    content = re.sub(p1, repl1, content)
    
    # Pattern 2: cpu.run(N); // comment\n        cpu.step();
    p2 = r'cpu\.run\((\d+)\);\s*//[^\n]*\n(\s+)cpu\.step\(\);'
    def repl2(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(1)}U);\n{m.group(2)}cpu.step();'
    content = re.sub(p2, repl2, content)
    
    # Pattern 3: cpu.run(N); // comment\n        auto s = cpu.snapshot();
    p3 = r'cpu\.run\((\d+)\);\s*//[^\n]*\n(\s+)auto s = cpu\.snapshot\(\);'
    def repl3(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(1)}U);\n{m.group(2)}auto s = cpu.snapshot();'
    content = re.sub(p3, repl3, content)
    
    # Pattern 4: cpu.run(N); // Run until halt/to halt
    p4 = r'cpu\.run\((\d+)\);\s*// Run until halt'
    content = re.sub(p4, lambda m: f'step_to(cpu, {m.group(1)}U);', content)
    if content.count(f'step_to(cpu, {m.group(1)}U);') > original.count(f'step_to(cpu, {m.group(1)}U);'):
        changes += 1
    
    p4b = r'cpu\.run\((\d+)\);\s*// Run to halt'
    content = re.sub(p4b, lambda m: f'step_to(cpu, {m.group(1)}U);', content)
    if content != original:
        changes += 1
    
    # Pattern 5: cpu.run(N); // Reach COMMENT\n        CHECK
    p5 = r'cpu\.run\((\d+)\);\s*// Reach[^\n]*\n(\s+)CHECK'
    def repl5(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(1)}U);\n{m.group(2)}CHECK'
    content = re.sub(p5, repl5, content)
    
    # Pattern 6: cpu.run(N); // COMMENT\n        CHECK
    p6 = r'cpu\.run\((\d+)\);\s*//[^\n]*\n(\s+)CHECK'
    def repl6(m):
        nonlocal changes
        changes += 1
        return f'step_to(cpu, {m.group(1)}U);\n{m.group(2)}CHECK'
    content = re.sub(p6, repl6, content)
    
    if changes == 0:
        return 0
    
    # Add helper
    helper = HELPER_ANON_NS if has_anon_ns else HELPER_QUALIFIED
    if has_anon_ns:
        content = content.replace('\n} // namespace\n', helper + '\n} // namespace\n')
        content = content.replace('\n}  // namespace\n', helper + '\n}  // namespace\n')
    else:
        # Add after includes, before TEST_CASE
        content = content.replace('\nTEST_CASE(', helper + '\nTEST_CASE(')
    
    filepath.write_text(content)
    return changes

# Files to fix
files_to_fix = [
    "cpu_fmul_test.cpp",
    "cpu_small_ops_test.cpp",
    "cpu_compare_test.cpp",
    "cpu_io_test.cpp",
    "cpu_branch_alias_test.cpp",
    "cpu_load_store_test.cpp",
    "cpu_sleep_wake_test.cpp",
    "cpu_gpio_sync_test.cpp",
    "cpu_uart_test.cpp",
    "cpu_uart_interrupt_test.cpp",
    "cpu_uart_firmware_test.cpp",
    "cpu_timer_pwm_test.cpp",
    "cpu_timer_control_test.cpp",
    "cpu_timer_dual_interrupt_test.cpp",
    "cpu_timer_external_clock_firmware_test.cpp",
    "cpu_ext_interrupt_test.cpp",
    "cpu_ext_interrupt_firmware_test.cpp",
    "cpu_pin_change_interrupt_test.cpp",
    "cpu_pin_change_interrupt_firmware_test.cpp",
    "cpu_interrupt_test.cpp",
    "cpu_interrupt_priority_test.cpp",
    "cpu_interrupt_chaining_test.cpp",
    "cpu_voltage_interrupt_test.cpp",
    "cpu_analog_comparator_firmware_test.cpp",
    "cpu_analog_digital_frontend_test.cpp",
    "cpu_analog_frontend_test.cpp",
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
]

total = 0
for name in files_to_fix:
    f = TESTS / name
    if not f.exists():
        continue
    c = fix_file(f)
    if c > 0:
        print(f"  {name}: {c} changes")
        total += c

print(f"\nTotal: {total} changes")
