#!/usr/bin/env python3
"""
Generate a GDB script to extract SimAVR trace with explicit register names.

Usage: python3 gen_gdb_script.py <elf_file> [--steps N] > script.gdb
"""

import sys
import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("elf", help="ELF file to run")
    parser.add_argument("--steps", type=int, default=20)
    parser.add_argument("--port", type=int, default=1234)
    args = parser.parse_args()
    
    print("set pagination off")
    print("set confirm off")
    print(f"target remote localhost:{args.port}")
    print("set logging file /dev/stdout")
    print("set logging enabled on")
    
    regs = [f"$r{i}" for i in range(32)]
    reg_fmt = ",".join(["%d"] * 32)
    
    for step in range(args.steps + 1):
        if step > 0:
            print("stepi")
        
        # printf step,pc,sreg,sp,r0,...,r31
        fmt = f"{step},0x%x,0x%x,0x%x," + reg_fmt + "\\n"
        args_str = f"$pc,$sreg,$sp,{','.join(regs)}"
        print(f'printf "{fmt}",{args_str}')
    
    print("quit")

if __name__ == "__main__":
    main()
