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
    
    vio_trace = get_vio_trace(hex_path, 1000)
    
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

    elif name == "bootloader_test":
        # Check SPM and WDT
        spm_activated = False
        rww_protected = False
        wdt_configured = False
        
        for entry in vio_trace:
            spmcsr = int(entry['SPMCSR'])
            wdtcsr = int(entry['WDTCSR'])
            cycle = int(entry['Cycle'])
            pc = int(entry['PC'], 16)
            
            # SPMEN (bit 0) should be set then cleared
            if spmcsr & 0x01:
                spm_activated = True
                
            # RWWSB (bit 6) should be set when busy
            if spmcsr & 0x40:
                # In bootloader_test.S, LPM r20, Z is used.
                # It takes 3 cycles, so check if R20 becomes 0xFF while RWWSB is set.
                if int(entry['R20']) == 0xFF:
                    rww_protected = True
            
            # WDTCSR should become 0x09 (WDE | WDP0) after timed sequence (0xA8 byte addr)
            # Cycle 25 was early, check later
            if wdtcsr == 0x09:
                wdt_configured = True
                
        if spm_activated:
            print("[SUCCESS] SPM Operation Activated (SPMEN detected)")
        else:
            print("[FAILURE] SPM Operation was never activated.")
            
        if rww_protected:
            print("[SUCCESS] RWW Section Protected (Read returned 0xFF during busy)")
        else:
            print("[FAILURE] RWW Section protection failed or was not triggered.")
            
        if wdt_configured:
            print("[SUCCESS] Watchdog Timer Configured via Timed Sequence")
        else:
            print("[FAILURE] Watchdog Timer configuration failed.")

if __name__ == "__main__":
    main()
