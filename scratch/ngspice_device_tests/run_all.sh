#!/usr/bin/env bash
# Run all ngspice co-simulation tests and report pass/fail
# Verifies PB0 toggles (both LOW and HIGH states observed)
DIR="$(cd "$(dirname "$0")" && pwd)"
NGSPICE="${NGSPICE:-/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice}"

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
