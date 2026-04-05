import gdb
import sys

def extract_trace(max_steps=20):
    """Extract register trace from connected SimAVR target."""
    
    # Print header
    print("step,pc,sreg,sp", end="")
    for i in range(32):
        print(f",r{i}", end="")
    print()
    
    for step in range(max_steps + 1):
        try:
            pc = int(gdb.parse_and_eval("$pc"))
            sreg = int(gdb.parse_and_eval("$sreg"))
            sp = int(gdb.parse_and_eval("$sp"))
            
            print(f"{step},{pc},{sreg},{sp}", end="")
            for i in range(32):
                val = int(gdb.parse_and_eval(f"$r{i}"))
                print(f",{val}", end="")
            print()
            
            if step < max_steps:
                gdb.execute("stepi", to_string=True)
        except Exception as e:
            print(f"Error at step {step}: {e}", file=sys.stderr)
            break

if __name__ == "__main__":
    # Parse arguments from command line
    args = gdb.newest_frame().architecture().name()
    extract_trace(10)
    gdb.execute("quit")
