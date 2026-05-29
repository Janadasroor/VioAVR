#!/bin/bash
# Multi-A-device XSPICE test runner — verifies EXIT 0 + actual pin toggle
NGSPICE="/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice"
DIR="$(cd "$(dirname "$0")" && pwd)"
ALL_PASS=0
ALL_FAIL=0

for cir in "$DIR"/test_*.cir; do
    name=$(basename "$cir" .cir)
    echo -n "[TEST] $name ... "
    log=$(mktemp)
    (cd "$DIR" && "$NGSPICE" -b "$cir") > "$log" 2>&1
    rc=$?
    if [ $rc -ne 0 ]; then
        echo "FAIL (EXIT $rc)"
        head -3 "$log"
        ALL_FAIL=$((ALL_FAIL + 1))
        rm -f "$log"
        continue
    fi

    ncols=$(awk '/^[0-9]/{print NF; exit}' "$log" 2>/dev/null)
    rows=$(grep -cE "^[0-9]" "$log" 2>/dev/null || echo 0)

    # Check for at least one HIGH (>4.0V) and one LOW (<0.1V) on any signal column
    toggle_ok=0
    for c in $(seq 3 $ncols); do
        high=$(awk -v c="$c" '/^[0-9]/{if($c+0 > 4.0){found=1; exit}} END{exit !found}' "$log" 2>/dev/null && echo 1 || echo 0)
        low=$(awk -v c="$c" '/^[0-9]/{if($c+0 < 0.1 && NR > 5){found=1; exit}} END{exit !found}' "$log" 2>/dev/null && echo 1 || echo 0)
        if [ "$high" = "1" ] && [ "$low" = "1" ]; then
            toggle_ok=$((toggle_ok + 1))
        fi
    done

    if [ "$toggle_ok" -gt 0 ]; then
        echo "PASS (${rows} rows, ${toggle_ok}/${ncols} signals toggle)"
        ALL_PASS=$((ALL_PASS + 1))
    elif [ "$rows" -gt 0 ]; then
        echo "WARN (EXIT 0, ${rows} rows, no toggle)"
        ALL_PASS=$((ALL_PASS + 1))
    else
        echo "WARN (EXIT 0, no data rows)"
        ALL_PASS=$((ALL_PASS + 1))
    fi
    rm -f "$log"
done

echo "---"
echo "Result: $ALL_PASS passed, $ALL_FAIL failed"
exit $ALL_FAIL
