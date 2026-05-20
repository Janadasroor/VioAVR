import sys
import os

def analyze():
    log_path = "/home/jnd/cpp_projects/VioAVR/scratch/atmega2560_matrix_demo/sim_matrix.log"
    
    if not os.path.exists(log_path):
        print(f"Error: Log file not found at {log_path}")
        return
        
    # We will map: time_value -> { pin_name: voltage_value }
    time_data = {}
    current_columns = []
    
    with open(log_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
                
            parts = line.split()
            
            # Check for header line
            if "Index" in parts and "time" in parts:
                # We found a header block!
                # parts[0] is 'Index', parts[1] is 'time', parts[2:] are pin names like 'v(pb0_an)'
                current_columns = [p.replace("v(", "").replace(")", "") for p in parts[2:]]
                continue
                
            if "------------------------" in line:
                continue
                
            # Parse data line (if we have active columns and it starts with an index integer)
            if current_columns and len(parts) >= 2:
                try:
                    idx = int(parts[0])
                    t = float(parts[1])
                    vals = [float(p) for p in parts[2:]]
                    
                    if t not in time_data:
                        time_data[t] = {}
                        
                    for col_name, val in zip(current_columns, vals):
                        time_data[t][col_name] = val
                except ValueError:
                    pass
                except IndexError:
                    pass

    if not time_data:
        print("No valid simulation data could be parsed from sim_matrix.log")
        return
        
    print(f"Parsed {len(time_data)} distinct time steps.")
    sorted_times = sorted(time_data.keys())
    print(f"Simulation time range: {sorted_times[0]*1e3:.3f} ms to {sorted_times[-1]*1e3:.3f} ms\n")
    
    # We want to find the ON intervals for each of the 10 pins
    pins = ["pb0_an", "pb1_an", "pd0_an", "pd1_an", "pd2_an", "pd3_an", "pd4_an", "pd5_an", "pd6_an", "pd7_an"]
    
    print("--- 10-LED Toggling States & Timing Analysis ---")
    print(f"{'Pin Name':<10} | {'Status':<10} | {'Active ON Intervals (ms)':<35}")
    print("-" * 65)
    
    for pin in pins:
        on_intervals = []
        in_on_state = False
        start_time = 0.0
        
        for t in sorted_times:
            val = time_data[t].get(pin, 0.0)
            is_on = (val > 4.0)  # Logic HIGH in 5V system is > 4V
            
            if is_on and not in_on_state:
                in_on_state = True
                start_time = t
            elif not is_on and in_on_state:
                in_on_state = False
                on_intervals.append((start_time, t))
                
        # Handle trailing ON state if it ran to the end of simulation
        if in_on_state:
            on_intervals.append((start_time, sorted_times[-1]))
            
        if on_intervals:
            intervals_str = ", ".join([f"[{s*1e3:.2f} - {e*1e3:.2f}]" for s, e in on_intervals])
            status = "ACTIVE"
        else:
            intervals_str = "No active ON states detected"
            status = "INACTIVE"
            
        print(f"{pin:<10} | {status:<10} | {intervals_str}")
        
    print("\nVerification: Sequential scan check...")
    # Let's print out the exact sequence of which pin goes high at each 100us step
    sequence = []
    last_pin = None
    for t in sorted_times:
        active_pins = [p for p in pins if time_data[t].get(p, 0.0) > 4.0]
        if active_pins:
            current_pin = active_pins[0]
            if current_pin != last_pin:
                sequence.append((t, current_pin))
                last_pin = current_pin
                
    print("\nToggled Sequence Over Time:")
    for t, pin in sequence[:25]:  # Print first 25 transitions
        print(f"  At t = {t*1e3:.2f} ms: {pin.upper()} turned ON")
    if len(sequence) > 25:
        print(f"  ... and {len(sequence) - 25} more transitions")

if __name__ == "__main__":
    analyze()
