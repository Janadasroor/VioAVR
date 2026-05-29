#!/bin/bash
# avr_adc_bridge co-simulation demo
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Symlink cosim library directory
ln -snf "$PROJECT_ROOT/build/cosim" "$SCRIPT_DIR/cosim"

# Run co-simulation (redirect stderr to /dev/null for cleaner output)
cd "$SCRIPT_DIR"
echo "Running ngspice co-simulation (500 us)..."
"$PROJECT_ROOT/../VioMATRIXC/release/src/ngspice" -b adc_bridge_cosim.cir 2>/dev/null | tee sim_matrix.log | tail -3

echo ""
echo "--- Demo complete ---"

# Check for PA0 HIGH
FIRST_HIGH=$(awk 'NR>2 && $4+0 > 4 {printf "PA0->HIGH at t=%.3f us (in=%.2f V)\n", $2*1e6, $3; exit}' sim_matrix.log)
if [ -n "$FIRST_HIGH" ]; then
    echo "$FIRST_HIGH"
else
    echo "PA0 stayed LOW (ADC result < 512)"
fi
