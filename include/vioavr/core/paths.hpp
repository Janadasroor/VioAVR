#pragma once

#include <cstdlib>
#include <string>

// Build-time locations. CMake supplies these (see the VioAVR::paths target);
// the fallbacks keep the header usable from a plain compile and are only
// accurate when running from the build tree.
#ifndef VIOAVR_ROOT_DIR
#define VIOAVR_ROOT_DIR ".."
#endif
#ifndef VIOAVR_BUILD_DIR
#define VIOAVR_BUILD_DIR "."
#endif
#ifndef VIOAVR_NGSPICE_BIN
#define VIOAVR_NGSPICE_BIN "ngspice"
#endif
#ifndef VIOAVR_COSIM_LIB_NAME
#if defined(__APPLE__)
#define VIOAVR_COSIM_LIB_NAME "libavr_cosim.dylib"
#elif defined(_WIN32)
#define VIOAVR_COSIM_LIB_NAME "avr_cosim.dll"
#else
#define VIOAVR_COSIM_LIB_NAME "libavr_cosim.so"
#endif
#endif

namespace vioavr::core {

// The ngspice binary used for co-simulation: $NGSPICE wins, then the one
// CMake found (the VioMATRIXC checkout or whatever is on PATH).
[[nodiscard]] inline std::string ngspice_binary() {
    if (const char* env = std::getenv("NGSPICE"); env != nullptr && *env != '\0') {
        return env;
    }
    return VIOAVR_NGSPICE_BIN;
}

} // namespace vioavr::core