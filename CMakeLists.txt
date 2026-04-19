cmake_minimum_required(VERSION 3.25)

project(
    VioAVR
    VERSION 0.1.0
    DESCRIPTION "Lightweight AVR ISS for ATmega8/328 co-simulation"
    LANGUAGES CXX
)

option(VIOAVR_BUILD_TESTS "Build unit tests" ON)
option(VIOAVR_ENABLE_QT "Build Qt integration layer" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
