#!/usr/bin/env bash
# Multi-MCU Co-Simulation Demo — Build and Run
# 2026-06-03

set -euo pipefail

DEMO_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="/home/jnd/cpp_projects/VioAVR/build"
NGSPICE="/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice"

echo "=== Step 1: Build firmware ==="
make -C "$DEMO_DIR" clean all

echo ""
echo "=== Step 2: Build cosim shim ==="
cmake --build "$BUILD_DIR" -j8 --target avr_cosim 2>/dev/null

echo ""
echo "=== Step 3: Link cosim shim ==="
ln -sf "$BUILD_DIR/cosim/libavr_cosim.so" "$DEMO_DIR/cosim/"

echo ""
echo "=== Step 4: Simulate ==="
cd "$DEMO_DIR"
"$NGSPICE" -b multi_mcu_fixed.cir
echo ""
echo "=== Step 5: Done ==="
echo "Result: sim_matrix.log"
ls -la sim_matrix.log
