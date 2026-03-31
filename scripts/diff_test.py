import subprocess
import os
import time
import re
import sys

def run_vioavr(hex_file, max_cycles=1000):
    cmd = ["./build/apps/vioavr-cli/vioavr_cli", "--trace", "--max-cycles", str(max_cycles), hex_file]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"VioAVR failed with exit code {result.returncode}")
        print(result.stdout)
        print(result.stderr)
        return []
    for line in result.stdout.splitlines():
        # [0x0001] RJMP     (0xcfff)
        instr_match = re.match(r"\[0x([0-9a-fA-F]+)\]\s+(\w+)\s+\(0x([0-9a-fA-F]+)\)", line)
        if instr_match:
            pc = int(instr_match.group(1), 16)
            # If we had a previous instruction, save its result state
            if "pc" in current_instr:
                trace.append(current_instr)
            
            current_instr = {"pc": pc, "regs": {}, "sreg": 0}
            continue
        
        #   R16 = 0x55
        reg_match = re.match(r"\s+R(\d+)\s+=\s+0x([0-9a-fA-F]+)", line)
        if reg_match:
            reg_idx = int(reg_match.group(1))
            val = int(reg_match.group(2), 16)
            current_instr["regs"][f"r{reg_idx}"] = val
            
    if "pc" in current_instr:
        trace.append(current_instr)
            
    return trace

def run_simavr(hex_file, max_steps=100):
    # Launch simavr in background
    sim_proc = subprocess.Popen(["simavr", "-g", "-m", "atmega328", "-f", "16000000", hex_file],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(0.5)
    
    try:
        cmd = ["avr-gdb", "-nx", "-batch", 
               "-ex", "source scripts/gdb_trace.py", 
               "-ex", f"python run_trace({max_steps})"]
        result = subprocess.run(cmd, capture_output=True, text=True)
    finally:
        sim_proc.terminate()
        sim_proc.wait()
    
    trace = []
    current_step = None
    
    for line in result.stdout.splitlines():
        # STEP 0 PC=0x0000
        step_match = re.match(r"STEP (\d+) PC=0x([0-9a-fA-F]+)", line)
        if step_match:
            if current_step:
                trace.append(current_step)
            pc_byte = int(step_match.group(2), 16)
            current_step = {"pc": pc_byte // 2, "regs": {}}
            continue
        
        #   r16=0x55
        reg_match = re.match(r"\s+([r|pc|sreg]\w*)=0x([0-9a-fA-F]+)", line)
        if reg_match and current_step:
            name = reg_match.group(1)
            val = int(reg_match.group(2), 16)
            if name == "pc":
                current_step["pc_next"] = val // 2
            elif name == "sreg":
                current_step["sreg"] = val
            else:
                current_step["regs"][name] = val
                
    if current_step:
        trace.append(current_step)
        
    return trace

def compare(vio_trace, sim_trace):
    steps = min(len(vio_trace), len(sim_trace))
    for i in range(steps):
        v = vio_trace[i]
        s = sim_trace[i]
        
        if v["pc"] != s["pc"]:
            print(f"Step {i}: PC mismatch! VioAVR=0x{v['pc']:04x}, simavr=0x{s['pc']:04x}")
            return False
            
        # Compare registers that were updated in VioAVR
        for r, val in v["regs"].items():
            if s["regs"].get(r) != val:
                print(f"Step {i} (PC=0x{v['pc']:04x}): Register {r} mismatch! VioAVR=0x{val:02x}, simavr=0x{s['regs'].get(r, 0):02x}")
                return False
                
        # SREG comparison is tricky because simavr might have more flags or different layout
        # But for ATmega328 they should match.
        # However, VioAVR trace doesn't currently print SREG changes in StandardTraceHook!
        # I should add that.
        
    print(f"Verified {steps} steps successfully.")
    return True

if __name__ == "__main__":
    hex_file = "test_alu.hex"
    if len(sys.argv) > 1:
        hex_file = sys.argv[1]
        
    print(f"Comparing VioAVR vs simavr for {hex_file}...")
    max_cycles = 1000
    if "eeprom" in hex_file:
        max_cycles = 100000
    vio = run_vioavr(hex_file, max_cycles)
    sim = run_simavr(hex_file, len(vio))
    
    if compare(vio, sim):
        sys.exit(0)
    else:
        sys.exit(1)
