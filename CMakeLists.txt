cmake_minimum_required(VERSION 3.25)

project(
    VioAVR
    VERSION 0.1.0
    DESCRIPTION "Lightweight AVR ISS for ATmega8/328 co-simulation"
    LANGUAGES C CXX
)

option(VIOAVR_BUILD_TESTS "Build unit tests" ON)
option(VIOAVR_ENABLE_QT "Build Qt integration layer" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch Microchip ATmega DFP
include(FetchContent)
FetchContent_Declare(
    atmega_dfp
    URL https://packs.download.microchip.com/Microchip.ATmega_DFP.3.6.299.atpack
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/avr-pack
)
FetchContent_MakeAvailable(atmega_dfp)
set(ATMEGA_DFP_DIR ${atmega_dfp_SOURCE_DIR} CACHE PATH "Path to ATmega DFP")

# Optimization and LTO
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
    
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-O3 -march=native)
    endif()
endif()

if(PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

add_subdirectory(src/core)
add_subdirectory(apps/vioavr-cli)
add_subdirectory(apps/viospice-poc)

if(VIOAVR_ENABLE_QT)
    add_subdirectory(src/qt)
endif()

if(VIOAVR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    add_subdirectory(tests/core)
endif()
add_executable(vioavr-bridge-daemon apps/vioavr-bridge-daemon/main.cpp)
target_link_libraries(vioavr-bridge-daemon PRIVATE VioAVR::core)
