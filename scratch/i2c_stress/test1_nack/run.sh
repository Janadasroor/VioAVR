#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="/home/jnd/cpp_projects/VioAVR/build"
NGSPICE="/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice"

echo "=== Build firmware ==="
make -C "$DIR" clean all

echo "=== Build cosim shim ==="
cmake --build "$BUILD_DIR" -j8 --target avr_cosim 2>/dev/null

echo "=== Link cosim shim ==="
ln -sf "$BUILD_DIR/cosim/libavr_cosim.so" "$DIR/cosim/"

echo "=== Simulate ==="
cd "$DIR" && "$NGSPICE" -b nack_test.cir

echo "=== Check results ==="
python3 check_nack.py sim_matrix.log
