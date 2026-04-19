#!/bin/bash
set -e

# Configuration
PROJECT_ROOT=$(pwd)
BUILD_DIR="${PROJECT_ROOT}/build"
VIOSPICE_DIR="${PROJECT_ROOT}/viospice"
INCLUDE_DIR="${PROJECT_ROOT}/include"
OUTPUT_NAME="viospice.cm"

# Find ngspice headers
NGSPICE_INC="/home/jnd/cpp_projects/ngspice/src/include"
if [ ! -d "$NGSPICE_INC" ]; then
    NGSPICE_INC="/usr/include/ngspice"
fi

echo "--- Building VioSpice XSPICE Model (Manual Bridge) ---"
echo "Project Root: ${PROJECT_ROOT}"
echo "NgSpice Inc:  ${NGSPICE_INC}"

# Ensure build exists
if [ ! -f "${BUILD_DIR}/src/core/libvioavr_core.a" ]; then
    echo "Error: libvioavr_core.a not found. Please run 'make' in build directory first."
    exit 1
fi

# Compile the bridge
echo "Compiling viospice_bridge.cpp..."
g++ -std=c++20 -fPIC -O3 \
    -I"${INCLUDE_DIR}" \
    -I"${NGSPICE_INC}" \
    -I"/home/jnd/cpp_projects/ngspice/release/src/include" \
    -c "${VIOSPICE_DIR}/viospice_bridge.cpp" -o "${BUILD_DIR}/viospice_bridge.o"

# Link into a shared library
echo "Linking ${OUTPUT_NAME}..."
g++ -shared -fPIC \
    "${BUILD_DIR}/viospice_bridge.o" \
    "${BUILD_DIR}/src/core/libvioavr_core.a" \
    -lrt -lpthread \
    -o "${BUILD_DIR}/${OUTPUT_NAME}"

echo "--- Success! ---"
echo "Model created at: ${BUILD_DIR}/${OUTPUT_NAME}"
