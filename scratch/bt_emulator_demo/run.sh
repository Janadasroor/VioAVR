#!/bin/bash
set -e
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


SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${VIOAVR_BUILD_DIR:-../../build}"
NGSPICE="$(find_ngspice)"
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
echo "To view waveforms: ../../build/oscope/vioavr_oscope --cir bt_hc05_native.cir"
