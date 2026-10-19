#!/usr/bin/env bash
set -euo pipefail
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


DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${VIOAVR_BUILD_DIR:-../../../build}"
NGSPICE="$(find_ngspice)"

echo "=== Build firmware ==="
make -C "$DIR" clean all

echo "=== Build cosim shim ==="
cmake --build "$BUILD_DIR" -j8 --target avr_cosim 2>/dev/null

echo "=== Link cosim shim ==="
ln -sf "$BUILD_DIR/cosim/libavr_cosim.so" "$DIR/cosim/"

echo "=== Simulate ==="
cd "$DIR" && "$NGSPICE" -b multibyte.cir

echo "=== Check results ==="
python3 check_multibyte.py sim_matrix.log
