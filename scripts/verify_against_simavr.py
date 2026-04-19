import subprocess
import csv
import io
import sys
import os

VIO_UTIL = "./build/tests/core/vioavr_cpu_trace_util"
SIMAVR = "/usr/bin/simavr"

def compile_asm(source, target_elf, target_hex):
    # Use explicit section starts for vectors (0) and bootloader (0x7000 byte)
    subprocess.run(["avr-gcc", "-mmcu=atmega328p", "-nostartfiles", 
                   "-Wl,--section-start=.vectors=0x0", 
                   "-Wl,--section-start=.bootloader=0x7000", 
                   "-o", target_elf, source], check=True)
    # Include all sections in the hex
    subprocess.run(["avr-objcopy", "-j", ".vectors", "-j", ".bootloader", "-j", ".text", "-j", ".data", 
                   "-O", "ihex", target_elf, target_hex], check=True)

def get_vio_trace(hex_path, cycles):
    trace_file = "/tmp/vio_trace.csv"
    cmd = f"{VIO_UTIL} {hex_path} {cycles} > {trace_file}"
    subprocess.run(cmd, shell=True, check=True)
    return trace_file

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
    
    trace_path = get_vio_trace(hex_path, 1000000)
    
    with open(trace_path, 'r') as f:
        reader = csv.DictReader(f)
        
        if name == "cpu_pc_order":
            reset_found = False
            last_pc = 0
            for entry in reader:
                pc = int(entry['PC'], 16)
                cycle = int(entry['Cycle'])
                if cycle > 5000:
                    if pc == 0 and last_pc != 0:
                        print(f"[SUCCESS] PC-Order Correct: CPU reset to PC 0 at cycle {cycle}")
                        reset_found = True
                        break
                last_pc = pc
            if not reset_found:
                print("[FAILURE] CPU PC Order Bug: Reset not detected at PC 0")

        elif name == "sleep_wake":
            woken = False
            for entry in reader:
                pc = int(entry['PC'], 16)
                cycle = int(entry['Cycle'])
                if cycle > 40 and pc == 0xe:
                    print(f"[SUCCESS] CPU woke from sleep at cycle {cycle} and jumped to vector 0x1C (word 0x0E)")
                    woken = True
                    break
            if not woken:
                print("[FAILURE] CPU did not wake up or jump to vector as expected.")

        elif name == "adc_trigger":
            triggered = False
            for entry in reader:
                adcsra = int(entry['ADCSRA'])
                tcnt0 = int(entry['TCNT0'])
                cycle = int(entry['Cycle'])
                if adcsra & (1 << 6):
                    print(f"[SUCCESS] ADC Auto-Triggered (ADSC=1) at cycle {cycle} (TCNT0={tcnt0})")
                    triggered = True
                    break
            if not triggered:
                print("[FAILURE] ADC did not auto-trigger on Timer0 overflow.")

        elif name == "bootloader_test":
            spm_activated = False
            rww_protected = False
            wdt_configured = False
            for entry in reader:
                spmcsr = int(entry['SPMCSR'])
                wdtcsr = int(entry['WDTCSR'])
                if spmcsr & 0x01: spm_activated = True
                if spmcsr & 0x40:
                    if int(entry['R20']) == 0xFF: rww_protected = True
                if wdtcsr == 0x09: wdt_configured = True
            
            if spm_activated: print("[SUCCESS] SPM Operation Activated")
            if rww_protected: print("[SUCCESS] RWW Section Protected")
            if wdt_configured: print("[SUCCESS] Watchdog Timer Configured")

if __name__ == "__main__":
    main()
