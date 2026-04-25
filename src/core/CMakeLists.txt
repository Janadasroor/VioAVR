set(CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

find_package(Threads REQUIRED)

add_library(vioavr_core STATIC)
add_library(VioAVR::core ALIAS vioavr_core)

target_sources(vioavr_core
    PRIVATE
        adc.cpp
        adc8x.cpp
        ac8x.cpp
        analog_comparator.cpp
        avr_cpu.cpp
        cpu_control.cpp
        device.cpp
        device_catalog.cpp
        gdb_stub.cpp
        eeprom.cpp
        ext_interrupt.cpp
        gpio_port.cpp
        hex_image.cpp
        io_peripheral.cpp
        memory_bus.cpp
        pin_change_interrupt.cpp
        pin_mux.cpp
        register_file.cpp
        spi.cpp
        spi8x.cpp
        can.cpp
        twi.cpp
        twi8x.cpp
        sync_engine.cpp
        trace_buffer.cpp
        viospice.cpp
        viospice_c.cpp
        bridge_shm_server.cpp
        bridge_shm_client.cpp
        timer8.cpp
        timer16.cpp
        timer10.cpp
        tca.cpp
        tcb.cpp
        rtc.cpp
        evsys.cpp
        ccl.cpp
        uart.cpp
        uart8x.cpp
        machine.cpp
        watchdog_timer.cpp
        lcd_controller.cpp
        wdt8x.cpp
        crc8x.cpp
        usb.cpp
        pll.cpp
        xmem.cpp
        psc.cpp
        dac.cpp
        nvm_ctrl.cpp
        cpu_int.cpp
        dma.cpp
        zcd.cpp
        lin.cpp
        opamp.cpp
        vcd_writer.cpp
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

target_link_libraries(vioavr_core PRIVATE rt Threads::Threads)
