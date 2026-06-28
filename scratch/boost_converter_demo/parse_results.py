#!/usr/bin/env python3
import sys

def parse_ngspice_output(filename):
    print("=== Boost Converter Simulation Results ===")
    
    vout_data = []
    vfb_data = []
    gate_data = []
    
    with open(filename, 'r') as f:
        lines = f.readlines()
        
    for line in lines:
        parts = line.strip().split()
        if len(parts) >= 5 and parts[0].isdigit():
            try:
                time = float(parts[1])
                vout = float(parts[3])
                vfb = float(parts[4])
                vout_data.append((time, vout, vfb))
            except ValueError:
                pass
        elif len(parts) >= 3 and parts[0].isdigit():
            try:
                time = float(parts[1])
                gate = float(parts[2])
                gate_data.append((time, gate))
            except ValueError:
                pass
                
    if not vout_data:
        print("Error: No simulation data found.")
        return

    # Print transient points
    print(f"{'Time (ms)':<12}{'V_in (V)':<12}{'V_out (V)':<12}{'V_feedback (V)':<16}")
    print("-" * 52)
    
    # Print sample points
    intervals = [0.0, 1.0, 3.0, 5.0, 7.0, 9.99]
    printed_indices = set()
    
    for target_t in intervals:
        # Find closest time
        closest = min(vout_data, key=lambda x: abs(x[0]*1000.0 - target_t))
        time_ms = closest[0] * 1000.0
        if closest not in printed_indices:
            printed_indices.add(closest)
            print(f"{time_ms:<12.3f}{5.0:<12.1f}{closest[1]:<12.3f}{closest[2]:<16.3f}")
            
    final = vout_data[-1]
    print("-" * 52)
    print(f"Final output: {final[1]:.3f} V (Feedback: {final[2]:.3f} V)")
    print("SUCCESS: Boost converter loop successfully simulated!")

if __name__ == "__main__":
    parse_ngspice_output("sim_output.txt")
