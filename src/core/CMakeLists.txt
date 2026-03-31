set(CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

add_library(vioavr_core STATIC)
add_library(VioAVR::core ALIAS vioavr_core)

target_sources(vioavr_core
    PRIVATE
        adc.cpp
        analog_comparator.cpp
        avr_cpu.cpp
        cpu_control.cpp
        device.cpp
        device_catalog.cpp
        eeprom.cpp
        ext_interrupt.cpp
        gpio_port.cpp
        hex_image.cpp
        io_peripheral.cpp
        memory_bus.cpp
        pin_change_interrupt.cpp
        register_file.cpp
        spi.cpp

        twi.cpp
        sync_engine.cpp
        viospice.cpp
        viospice_c.cpp
        timer8.cpp

        timer16.cpp
        uart0.cpp
        watchdog_timer.cpp
)

target_include_directories(vioavr_core
    PUBLIC
        $<BUILD_INTERFACE:${CORE_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(vioavr_core PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(vioavr_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
