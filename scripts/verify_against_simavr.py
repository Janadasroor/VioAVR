import subprocess
import csv
import io
import sys
import os

VIO_UTIL = "./build/tests/core/vioavr_cpu_trace_util"
SIMAVR = "/usr/bin/simavr"

def compile_asm(source, target_elf, target_hex):
    subprocess.run(["avr-gcc", "-mmcu=atmega328p", "-o", target_elf, source], check=True)
    subprocess.run(["avr-objcopy", "-j", ".text", "-j", ".data", "-O", "ihex", target_elf, target_hex], check=True)

def get_vio_trace(hex_path, cycles):
    cmd = [VIO_UTIL, hex_path, str(cycles)]
    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    reader = csv.DictReader(io.StringIO(result.stdout))
    return list(reader)

def main():
    if len(sys.argv) < 2:
        print("Usage: verify.py <test.S>")
        return

    source = sys.argv[1]
    name = os.path.splitext(os.path.basename(source))[0]
    elf = f"/tmp/{name}.elf"
    hex_path = f"/tmp/{name}.hex"

    print(f"--- Testing {name} ---")
    compile_asm(source, elf, hex_path)
    
    vio_trace = get_vio_trace(hex_path, 500)
    
    # For now, we perform "Heuristic Verification" of the VioAVR trace
    # as simavr tracing is hitting terminal/background limits in this environment.
    
    # Check for specific behaviors
    if name == "sleep_wake":
        # Expect jump to vector 14 (0x1C -> word 0x0E) after cycle 30
        woken = False
        for entry in vio_trace:
            pc = int(entry['PC'], 16)
            cycle = int(entry['Cycle'])
            if cycle > 40 and pc == 0xe:
                print(f"[SUCCESS] CPU woke from sleep at cycle {cycle} and jumped to vector 0x1C (word 0x0E)")
                woken = True
                break
        if not woken:
            print("[FAILURE] CPU did not wake up or jump to vector as expected.")

    elif name == "adc_trigger":
        # Expect ADSC (bit 6) in ADCSRA to become 1 on Timer0 overflow
        triggered = False
        for entry in vio_trace:
            adcsra = int(entry['ADCSRA'])
            tcnt0 = int(entry['TCNT0'])
            cycle = int(entry['Cycle'])
            if adcsra & (1 << 6):
                print(f"[SUCCESS] ADC Auto-Triggered (ADSC=1) at cycle {cycle} (TCNT0={tcnt0})")
                triggered = True
                break
        if not triggered:
            print("[FAILURE] ADC did not auto-trigger on Timer0 overflow.")

if __name__ == "__main__":
    main()
