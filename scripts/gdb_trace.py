import gdb
import sys

def get_regs():
    regs = {}
    for i in range(32):
        name = f"r{i}"
        val = int(gdb.parse_and_eval(f"${name}"))
        regs[name] = val
    regs["pc"] = int(gdb.parse_and_eval("$pc"))
    regs["sreg"] = int(gdb.parse_and_eval("$SREG"))
    return regs

def run_trace(max_steps=100):
    gdb.execute("target remote :1234")
    # Get initial state? No, simavr starts at 0.
    
    for i in range(max_steps):
        # We need to know the PC BEFORE the instruction?
        # VioAVR trace prints: [0xPC] MNEMONIC (OPCODE)
        # then register changes.
        
        pc = int(gdb.parse_and_eval("$pc"))
        # We can't easily get the mnemonic from GDB without a 'disassemble' call
        # but we can get the opcode bytes.
        
        # Step one instruction
        gdb.execute("stepi", to_string=True)
        
        # Get new regs
        regs = get_regs()
        
        # Print in a format we can easily parse
        print(f"STEP {i} PC=0x{pc:04x}")
        for r, v in regs.items():
            print(f"  {r}=0x{v:02x}")

if __name__ == "__main__":
    run_trace(20)
