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

passed=0 failed=0
for f in "$DIR"/*_cosim.cir; do
    chip=$(basename "$f" _cosim.cir)
    echo -n "$chip ... "
    output=$("$NGSPICE" -b "$f" 2>/dev/null)
    # Scan all data rows for LOW (< 0.1V) and HIGH (> 4.0V)
    read low high <<<$(echo "$output" | grep -E '^[0-9]' | awk '
        { if ($3+0 < 0.1) l=1; if ($3+0 > 4.0) h=1 }
        END { print l+0, h+0 }
    ')
    last=$(echo "$output" | grep -E '^[0-9]' | tail -1 | awk '{print $3}')
    if [ "$low" = "1" ] && [ "$high" = "1" ]; then
        echo "PASS (pb0_an=$last, saw both LOW and HIGH)"
        passed=$((passed + 1))
    elif [ "$low" = "1" ]; then
        echo "FAIL (stuck LOW, pb0_an=$last)"
        failed=$((failed + 1))
    else
        echo "FAIL (stuck HIGH, pb0_an=$last)"
        failed=$((failed + 1))
    fi
done
echo "---"
echo "$passed passed, $failed failed, $((passed+failed)) total"
