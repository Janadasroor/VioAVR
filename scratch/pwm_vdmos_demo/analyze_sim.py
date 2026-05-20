import sys

def analyze():
    log_path = "/home/jnd/cpp_projects/VioAVR/scratch/pwm_vdmos_demo/sim.log"
    
    times = []
    v_drains = []
    v_gates = []
    
    with open(log_path, 'r') as f:
        for line in f:
            line = line.strip()
            # Split the line by whitespace
            parts = line.split()
            if len(parts) == 5:
                # Check if first part is an integer index
                try:
                    idx = int(parts[0])
                    t = float(parts[1])
                    v_pb1 = float(parts[2])
                    v_gate = float(parts[3])
                    v_drain = float(parts[4])
                    
                    times.append(t)
                    v_drains.append(v_drain)
                    v_gates.append(v_gate)
                except ValueError:
                    pass
                    
    if not times:
        print("No valid data found in sim.log")
        return
        
    print(f"Total parsed data points: {len(times)}")
    print(f"Simulation time range: {times[0]*1e3:.3f} ms to {times[-1]*1e3:.3f} ms")
    
    # Let's calculate the average drain voltage in intervals
    def get_average_in_range(t_start, t_end):
        total_area = 0.0
        total_time = 0.0
        for i in range(len(times) - 1):
            t1, t2 = times[i], times[i+1]
            if t1 >= t_start and t2 <= t_end:
                dt = t2 - t1
                val = 0.5 * (v_drains[i] + v_drains[i+1])
                total_area += val * dt
                total_time += dt
        return total_area / total_time if total_time > 0 else 0.0

    print("\n--- Average Drain Voltages ---")
    intervals = [
        (0.0, 0.3e-3),
        (0.3e-3, 0.6e-3),
        (0.6e-3, 0.9e-3),
        (0.9e-3, 1.2e-3),
        (1.2e-3, 1.5e-3),
        (1.4e-3, 1.5e-3)
    ]
    for start, end in intervals:
        avg = get_average_in_range(start, end)
        print(f"Interval [{start*1e3:.1f} ms - {end*1e3:.1f} ms]: {avg:.3f} V")

if __name__ == "__main__":
    analyze()
