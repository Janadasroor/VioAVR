#!/usr/bin/env bash
# Resolve locations without hardcoding anyone's home directory:
#   BUILD_DIR  = $VIOAVR_BUILD_DIR, else <repo>/build
#   NGSPICE    = $NGSPICE, else $VioMATRIXC_ROOT/{release,build}/src/ngspice,
#                else <sibling checkout>/../VioMATRIXC/..., else PATH
find_ngspice() {
    if [[ -n "${NGSPICE:-}" ]]; then echo "$NGSPICE"; return; fi
    local roots=()
    [[ -n "${VioMATRIXC_ROOT:-}" ]] && roots+=("$VioMATRIXC_ROOT")
    [[ -n "${VIOAVR_ROOT:-}" ]] && roots+=("$VIOAVR_ROOT/../VioMATRIXC")
    roots+=("$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../../VioMATRIXC")
    for r in "${roots[@]}"; do
        for layout in release build; do
            [[ -x "$r/$layout/src/ngspice" ]] && { echo "$r/$layout/src/ngspice"; return; }
        done
    done
    command -v ngspice 2>/dev/null || echo ngspice
}

# Multi-MCU Co-Simulation Demo — Build and Run
# 2026-06-03

set -euo pipefail

DEMO_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${VIOAVR_BUILD_DIR:-../../build}"
NGSPICE="$(find_ngspice)"

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
