#!/bin/bash
if [ -z "$1" ]; then
    echo "Usage: ./scripts/re-audit.sh <MCU_NAME>"
    exit 1
fi
MCU=$1
./scripts/atdf_export.py --mcu $MCU
python3 scripts/gen_audit_md.py --mcu $MCU --meta ${MCU}_metadata.json
echo "Audit report updated: docs/ACCURACY_${MCU}.md"
