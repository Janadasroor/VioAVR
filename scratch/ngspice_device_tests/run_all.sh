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

# Run all ngspice co-simulation tests and report pass/fail
# Verifies PB0 toggles (both LOW and HIGH states observed)
DIR="$(cd "$(dirname "$0")" && pwd)"
NGSPICE="$(find_ngspice)"
BUILD_DIR="${VIOAVR_BUILD_DIR:-$(cd "$DIR/../.." && pwd)/build}"
REPO_ROOT="$(cd "$DIR/../.." && pwd)"
MATRIXC_ROOT="${VioMATRIXC_ROOT:-$REPO_ROOT/../VioMATRIXC}"

# Each netlist instantiates A_dac and A_AVR (d_cosim), so the code models have
# to be loaded and the co-simulation shim has to be reachable as
# ./cosim/libavr_cosim.so relative to the directory ngspice runs in.
COSIM_LIB="$BUILD_DIR/cosim/libavr_cosim.so"
if [[ ! -f "$COSIM_LIB" ]]; then
    echo "cosim shim not built: $COSIM_LIB" >&2
    echo "build it with: cmake --build <build> --target avr_cosim" >&2
    exit 1
fi
mkdir -p "$DIR/cosim"
ln -sf "$COSIM_LIB" "$DIR/cosim/libavr_cosim.so"
{
    echo "codemodel $MATRIXC_ROOT/src/xspice/icm/digital/digital.cm"
    echo "codemodel $MATRIXC_ROOT/src/xspice/icm/analog/analog.cm"
    echo "codemodel $BUILD_DIR/cosim/avr_adc_bridge.cm"
} > "$DIR/.spiceinit"

passed=0 failed=0
for f in "$DIR"/*_cosim.cir; do
    chip=$(basename "$f" _cosim.cir)
    echo -n "$chip ... "
    # The netlists reference ./cosim/libavr_cosim.so and the firmware hex
    # relative to the working directory, so run them from the test directory.
    output=$(cd "$DIR" && "$NGSPICE" -b "$(basename "$f")" 2>/dev/null)
    # The netlist prints the pin the firmware toggles; take it from there.
    node=$(grep -oE 'print v\([a-z0-9_]+\)' "$f" | head -1 | sed 's/print v(\([^)]*\))/\1/')
    [ -n "$node" ] || node="pb0_an"
    # Scan all data rows for LOW (< 0.1V) and HIGH (> 4.0V)
    read low high <<<$(echo "$output" | grep -E '^[0-9]' | awk '
        { if ($3+0 < 0.1) l=1; if ($3+0 > 4.0) h=1 }
        END { print l+0, h+0 }
    ')
    last=$(echo "$output" | grep -E '^[0-9]' | tail -1 | awk '{print $3}')
    if [ "$low" = "1" ] && [ "$high" = "1" ]; then
        echo "PASS ($node=$last, saw both LOW and HIGH)"
        passed=$((passed + 1))
    elif [ "$low" = "1" ]; then
        echo "FAIL (stuck LOW, $node=$last)"
        failed=$((failed + 1))
    else
        echo "FAIL (stuck HIGH, $node=$last)"
        failed=$((failed + 1))
    fi
done
echo "---"
echo "$passed passed, $failed failed, $((passed+failed)) total"
