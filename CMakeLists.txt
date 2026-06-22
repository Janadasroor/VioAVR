cmake_minimum_required(VERSION 3.25)

project(
    VioAVR
    VERSION 0.1.0
    DESCRIPTION "Lightweight AVR ISS for ATmega8/328 co-simulation"
    LANGUAGES C CXX
)

# ── Enforce canonical build system ─────────────────────────────────
# Only Ninja is supported. The old Makefile-based build is removed.
# Override with -DVIOAVR_ALLOW_ANY_GENERATOR=ON for CI or cross-platform builds.
# Use: cmake -B build --preset release
if(NOT VIOAVR_ALLOW_ANY_GENERATOR AND NOT CMAKE_GENERATOR STREQUAL "Ninja")
    message(FATAL_ERROR
        "Unsupported generator '${CMAKE_GENERATOR}'. "
        "VioAVR requires Ninja. Use:\n"
        "  cmake -B build --preset release\n"
        "or:\n"
        "  cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release\n"
        "To allow other generators (e.g., on CI), pass -DVIOAVR_ALLOW_ANY_GENERATOR=ON"
    )
endif()

# ── Enforce unity build for core library ───────────────────────────
# Without unity, the 87-source-file core library takes ~25× longer to compile.
option(VIOAVR_DISABLE_UNITY "Disable unity/jumbo build (NOT RECOMMENDED)" OFF)
if(VIOAVR_DISABLE_UNITY)
    message(WARNING
        "Unity build disabled — expect significantly slower builds. "
        "Enable it by removing -DVIOAVR_DISABLE_UNITY=ON"
    )
endif()

option(VIOAVR_BUILD_TESTS "Build unit tests" ON)
option(VIOAVR_ENABLE_QT "Build Qt integration layer" OFF)
option(VIOAVR_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(VIOAVR_ENABLE_COVERAGE "Enable code coverage instrumentation (GCC)" OFF)
option(VIOAVR_ENABLE_WERROR "Treat compiler warnings as errors" OFF)
option(VIOAVR_USE_NATIVE_ARCH "Enable -march=native for Release builds (disable for CI/cross-compile)" ON)
option(VIOAVR_ALLOW_ANY_GENERATOR "Allow non-Ninja generators (for CI/compatibility)" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── ccache ──────────────────────────────────────────────────────────
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(ENV{CCACHE_SLOPPINESS} "pch_defines,time_macros")
    set(ENV{CCACHE_FILECLONE} "true")
    message(STATUS "ccache: enabled (${CCACHE_PROGRAM})")
else()
    message(WARNING
        "ccache not found — rebuilds will be slower. "
        "Install ccache for faster incremental builds."
    )
endif()

# ── Fetch device family packs ───────────────────────────────────────
# After first fetch, packs are cached locally in avr-pack/.
# Skip network if already populated (detect by presence of a known file).
if(EXISTS "${CMAKE_SOURCE_DIR}/avr-pack/atmega/include/avr/io.h")
    set(FETCHCONTENT_FULLY_DISCONNECTED ON)
endif()
include(FetchContent)
set(FETCHCONTENT_QUIET ON)

# Microchip ATmega DFP
FetchContent_Declare(
    atmega_dfp
    URL https://packs.download.microchip.com/Microchip.ATmega_DFP.3.6.299.atpack
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/avr-pack/atmega
)
FetchContent_MakeAvailable(atmega_dfp)
set(ATMEGA_DFP_DIR ${atmega_dfp_SOURCE_DIR} CACHE PATH "Path to ATmega DFP")

# Microchip ATtiny DFP
FetchContent_Declare(
    attiny_dfp
    URL https://packs.download.microchip.com/Microchip.ATtiny_DFP.3.4.278.atpack
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/avr-pack/attiny
)
FetchContent_MakeAvailable(attiny_dfp)
set(ATTINY_DFP_DIR ${attiny_dfp_SOURCE_DIR} CACHE PATH "Path to ATtiny DFP")

# Microchip AVR-Dx DFP (AVR-DA/DB/DD/DU)
FetchContent_Declare(
    avr_dx_dfp
    URL https://packs.download.microchip.com/Microchip.AVR-Dx_DFP.2.8.343.atpack
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/avr-pack/avr-dx
)
FetchContent_MakeAvailable(avr_dx_dfp)
set(AVR_DX_DFP_DIR ${avr_dx_dfp_SOURCE_DIR} CACHE PATH "Path to AVR-Dx DFP")

# Microchip AVR-Ex DFP (AVR-EA/EB)
FetchContent_Declare(
    avr_ex_dfp
    URL https://packs.download.microchip.com/Microchip.AVR-Ex_DFP.2.12.239.atpack
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/avr-pack/avr-ex
)
FetchContent_MakeAvailable(avr_ex_dfp)
set(AVR_EX_DFP_DIR ${avr_ex_dfp_SOURCE_DIR} CACHE PATH "Path to AVR-Ex DFP")

# Microchip AVR-Lx DFP (AVR-LA)
FetchContent_Declare(
    avr_lx_dfp
    URL https://packs.download.microchip.com/Microchip.AVR-Lx_DFP.1.3.46.atpack
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/avr-pack/avr-lx
)
FetchContent_MakeAvailable(avr_lx_dfp)
set(AVR_LX_DFP_DIR ${avr_lx_dfp_SOURCE_DIR} CACHE PATH "Path to AVR-Lx DFP")

# Microchip AVR-Sx DFP (AVR-SD)
FetchContent_Declare(
    avr_sx_dfp
    URL https://packs.download.microchip.com/Microchip.AVR-Sx_DFP.1.2.26.atpack
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/avr-pack/avr-sx
)
FetchContent_MakeAvailable(avr_sx_dfp)
set(AVR_SX_DFP_DIR ${avr_sx_dfp_SOURCE_DIR} CACHE PATH "Path to AVR-Sx DFP")

# Note: XMEGA DFP is split into 5 packs (Atmel.XMEGAA/B/C/D/E_DFP).
# These are not auto-fetched — manually install to avr-pack/xmega/atdf/

# ── Optimization, LTO, arch flags ──────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Guard -march=native with VIOAVR_USE_NATIVE_ARCH (disable in CI for consistent caching)
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    if(VIOAVR_USE_NATIVE_ARCH AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
        add_compile_options(-O3 -march=native)
    endif()
endif()

# LTO is applied PER-TARGET (not globally) so tests build fast.
# Production targets use it; test targets don't.
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
if(ipo_supported)
    message(STATUS "LTO supported: yes (applied per-target)")
else()
    message(STATUS "LTO supported: no")
endif()

macro(enable_lto target)
    if(ipo_supported)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endmacro()

if(PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# Sanitizers
if(VIOAVR_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# Coverage
if(VIOAVR_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(--coverage -fno-inline -fno-elide-constructors)
        add_link_options(--coverage)
    endif()
endif()

# Warnings
if(VIOAVR_ENABLE_WERROR)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror)
    endif()
endif()

# ── Windows/MinGW: static runtime (no libgcc/libstdc++/libwinpthread DLLs) ──
# Without this, the exe requires several MinGW DLLs to be present on the
# target machine (libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll).
# Statically linking the runtime embeds them inside the exe, making it fully
# self-contained for end-users.
if(MINGW)
    add_link_options(-static-libgcc -static-libstdc++)
    message(STATUS "MinGW: static runtime linkage enabled (self-contained exe)")
endif()

# ── Detect POSIX shared memory (shm_open) for the SHM bridge ──────
# Available on Linux/macOS; not available on Windows (even with MinGW).
# Must be before add_subdirectory() calls so subdirectories see the variable.
include(CheckSymbolExists)
if(CMAKE_SYSTEM_NAME MATCHES "Linux|Darwin")
    set(HAVE_SHM_OPEN TRUE)
else()
    check_symbol_exists(shm_open "sys/mman.h" HAVE_SHM_OPEN)
endif()
add_subdirectory(src/core)
add_subdirectory(apps/vioavr)
add_subdirectory(apps/vioavr-cli)
add_subdirectory(apps/viospice-poc)
add_subdirectory(cosim)

if(VIOAVR_ENABLE_QT)
    add_subdirectory(src/qt)
endif()

find_package(Qt6 QUIET COMPONENTS Core Gui Widgets Charts)
if(Qt6Charts_FOUND AND Qt6_FOUND)
    add_subdirectory(tools/oscope)
    message(STATUS "vioavr_oscope: enabled (Qt6 Charts found)")
else()
    message(STATUS "vioavr_oscope: disabled (Qt6 Charts not found)")
endif()

if(Qt6_FOUND)
    add_subdirectory(tools/term)
    message(STATUS "vioavr_term: enabled (Qt6 Widgets found)")
else()
    message(STATUS "vioavr_term: disabled (Qt6 not found)")
endif()

if(VIOAVR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── SHM bridge daemon (requires shm_open, detected above) ─────────
if(HAVE_SHM_OPEN)
    add_executable(vioavr-bridge-daemon apps/vioavr-bridge-daemon/main.cpp)
    target_link_libraries(vioavr-bridge-daemon PRIVATE VioAVR::core)
    enable_lto(vioavr-bridge-daemon)
else()
    message(STATUS "vioavr-bridge-daemon: disabled (shm_open not available on this platform)")
endif()

# ── Install ────────────────────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Core static library
install(TARGETS vioavr_core
    EXPORT VioAVRTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# CLI apps
install(TARGETS vioavr
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Public headers
install(DIRECTORY include/vioavr/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vioavr
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h"
        PATTERN "*.hxx"
)

# CMake targets export
install(EXPORT VioAVRTargets
    FILE VioAVRTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VioAVR
    NAMESPACE VioAVR::
)

# Config file
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VioAVRConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/VioAVRConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VioAVR
    PATH_VARS INCLUDE_INSTALL_DIR
)

# Version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/VioAVRConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/VioAVRConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VioAVRConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VioAVR
)
