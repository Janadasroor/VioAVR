#!/usr/bin/env bash
# Test: EVSYS->ADC->PWM Voltage-to-Duty-Cycle Mapping
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

rm -f sim_matrix_*.log

declare -A VOLTAGES
VOLTAGES=( [0v]=0.0 [1v]=1.0 [2v5]=2.5 [3v]=3.0 [5v]=5.0 )

echo "===== EVSYS->ADC->PWM Voltage Sweep Test ====="
echo ""
printf "%-8s %-10s %-14s %-10s %-10s %s\n" "Test" "Vin(V)" "Exp CMP0" "Heartbeat" "PB0 HIGH%" "Status"
echo "--------------------------------------------------------------------------------"

for tag in 0v 1v 2v5 3v 5v; do
    vin="${VOLTAGES[$tag]}"
    # Expected CMP0 = vin/5.0*1024/4
    exp_cmp0=$(echo "$vin 5.0 1024 4" | awk '{printf "%d", $1/$2*$3/$4}')
    cir="test_${tag}.cir"
    log="sim_matrix_${tag}.log"

    echo -n "  Running ${cir}..."
    if ngspice -b "$cir" > /dev/null 2>&1; then
        echo " done."
    else
        echo " FAILED!"
        printf "%-8s %-10s %-14s %-10s %-10s %s\n" "$tag" "$vin" "$exp_cmp0" "-" "-" "NGSPICE ERROR"
        continue
    fi

    if [ ! -f "$log" ]; then
        printf "%-8s %-10s %-14s %-10s %-10s %s\n" "$tag" "$vin" "$exp_cmp0" "-" "-" "NO LOG"
        continue
    fi

    total=$(tail -n +4 "$log" | wc -l)
    # PB0 HIGH = pb0_an > 2.5V
    pb0_high=$(tail -n +4 "$log" | awk '$3+0 > 2.5 {c++} END{print c+0}')
    # PC0 HIGH at any point = heartbeat alive
    hb=$(tail -n +4 "$log" | awk '$4+0 > 2.5 {found=1} END{print found+0}')

    pct="0"
    if [ "$total" -gt 0 ]; then
        pct=$(echo "$pb0_high $total" | awk '{printf "%d", $1/$2*100}')
    fi

    hb_str="NO"
    [ "$hb" = "1" ] && hb_str="YES"

    # Pass/fail: PB0 should be mostly LOW at 0V, mostly HIGH at 5V
    status="PASS"
    if [ "$hb" = "0" ]; then
        status="FAIL (no heartbeat)"
    fi
    case "$tag" in
        0v)  [ "$pct" -gt 10 ] && status="FAIL (expected ~0%, got ${pct}%)" ;;
        5v)  [ "$pct" -lt 90 ] && status="FAIL (expected ~100%, got ${pct}%)" ;;
    esac

    printf "%-8s %-10s %-14s %-10s %-10s %s\n" "$tag" "$vin" "$exp_cmp0" "$hb_str" "${pct}%" "$status"
done

echo ""
echo "===== Expected behavior ====="
echo "  0.0V -> CMP0=0   -> PB0 constant LOW  (0% duty)"
echo "  1.0V -> CMP0=51  -> PB0 ~20% duty"
echo "  2.5V -> CMP0=128 -> PB0 ~50% duty"
echo "  3.0V -> CMP0=153 -> PB0 ~60% duty"
echo "  5.0V -> CMP0=255 -> PB0 constant HIGH (~100% duty)"
echo ""
echo "NOTE: Duty % is approximate (sampled at 1us steps, PWM ~62.7kHz = 16us period)"
echo "      A ~20% duty signal sampled at 1us over 500us gives ~100 samples per period."
echo "      The dac_bridge reproduces digital PWM faithfully (t_rise=1n)."
