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


# Configuration
PROJECT_ROOT=$(pwd)
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build_release}"
VIOSPICE_DIR="${PROJECT_ROOT}/viospice"
INCLUDE_DIR="${PROJECT_ROOT}/include"
OUTPUT_NAME="viospice.cm"

# Find ngspice headers: $NGSPICE_ROOT, the VioMATRIXC sibling checkout, then
# the system install.
NGSPICE_INC=""
for candidate in \
    "${NGSPICE_ROOT:+$NGSPICE_ROOT/src/include}" \
    "${VioMATRIXC_ROOT:+$VioMATRIXC_ROOT/src/include}" \
    "${PROJECT_ROOT}/../VioMATRIXC/src/include" \
    "/usr/include/ngspice"
do
    if [ -n "$candidate" ] && [ -d "$candidate" ]; then
        NGSPICE_INC="$candidate"
        break
    fi
done
if [ -z "$NGSPICE_INC" ]; then
    echo "Error: could not find ngspice headers. Set NGSPICE_ROOT or VioMATRIXC_ROOT."
    exit 1
fi
NGSPICE_INC_B="$NGSPICE_INC"

echo "--- Building VioSpice XSPICE Model (Manual Bridge) ---"
echo "Project Root: ${PROJECT_ROOT}"
echo "NgSpice Inc:  ${NGSPICE_INC}"

# Ensure build exists
if [ ! -f "${BUILD_DIR}/src/core/libvioavr_core.a" ]; then
    echo "Error: libvioavr_core.a not found. Please run 'make' in build directory first."
    exit 1
fi

# Compile the bridge and ifspec
echo "Compiling viospice_bridge.cpp..."
g++ -std=c++20 -fPIC -O0 -g \
    -I"${INCLUDE_DIR}" \
    -I"${NGSPICE_INC}" \
    -I"${NGSPICE_INC_B}" \
    -c "${VIOSPICE_DIR}/viospice_bridge.cpp" -o "${BUILD_DIR}/viospice_bridge.o"

echo "Compiling modified ifspec.c..."
gcc -fPIC -O0 -g \
    -I"${INCLUDE_DIR}" \
    -I"${NGSPICE_INC}" \
    -I"${NGSPICE_INC_B}" \
    -c "${PROJECT_ROOT}/scratch/cmpp_build/d_vioavr/ifspec.c" -o "${PROJECT_ROOT}/scratch/cmpp_build/d_vioavr/ifspec.o"

# Link into a shared library
echo "Linking ${OUTPUT_NAME}..."
g++ -shared -fPIC \
    "${BUILD_DIR}/viospice_bridge.o" \
    "${PROJECT_ROOT}/scratch/cmpp_build/dlmain.o" \
    "${PROJECT_ROOT}/scratch/cmpp_build/d_vioavr/ifspec.o" \
    "${BUILD_DIR}/src/core/libvioavr_core.a" \
    -lrt -lpthread \
    -o "${BUILD_DIR}/${OUTPUT_NAME}"

echo "--- Success! ---"
echo "Model created at: ${BUILD_DIR}/${OUTPUT_NAME}"
