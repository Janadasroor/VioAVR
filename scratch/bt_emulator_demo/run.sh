#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="/home/jnd/cpp_projects/VioAVR/build"
NGSPICE="/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice"
MCU="atmega4809"

cd "$SCRIPT_DIR"

echo "==> Compiling APP firmware..."
avr-gcc -mmcu=$MCU -Os -DF_CPU=16000000UL -o app.elf app.c 2>&1
avr-objcopy -O ihex app.elf app.hex 2>&1

echo "==> Setting up cosim links..."
ln -sf "$BUILD_DIR/cosim/libavr_cosim.so" cosim/libavr_cosim.so

echo "==> Building cosim shim..."
cmake --build "$BUILD_DIR" -j8 --target avr_cosim 2>&1

echo "==> Running ngspice (native HC-05)..."
"$NGSPICE" -b bt_hc05_native.cir > sim_matrix.log 2>&1

echo "==> Done. Output in sim_matrix.log"

echo "---"
echo "Rows in log:"
grep -c "^[0-9]" sim_matrix.log || true

echo "APP TXD (PA1) transitions (col 3):"
awk '/Index.*time.*v/,0' sim_matrix.log | grep -E "^[0-9]" | awk '{if ($3 > 4.0) print "HIGH:", $2; else if ($3 < 0.5) print " LOW:", $2}' | head -30

echo ""
echo "To view waveforms: /home/jnd/cpp_projects/VioAVR/build/oscope/vioavr_oscope --cir bt_hc05_native.cir"
