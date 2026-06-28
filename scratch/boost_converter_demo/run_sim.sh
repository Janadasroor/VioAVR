#!/bin/bash
set -e

# Compile firmware
echo "=== Compiling ATmega328P Closed-Loop Firmware ==="
make clean
make

# Copy libraries from build directory
echo "=== Copying XSPICE Code Models ==="
cp ../../build/viospice.cm .
cp ../../build/cosim/libavr_cosim.so .
cp ../../build/cosim/avr_adc_bridge.cm .

# Run ngspice simulation in batch mode
echo "=== Running Closed-loop Boost Converter Co-Simulation ==="
ngspice -b boost_cosim.cir > sim_output.txt

# Run Python parser
python3 parse_results.py
