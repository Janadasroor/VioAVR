set(CORE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

find_package(Threads REQUIRED)

add_library(vioavr_core STATIC)
add_library(VioAVR::core ALIAS vioavr_core)

target_sources(vioavr_core
    PRIVATE
        adc.cpp
        adc8x.cpp
        adc10b.cpp
        ac8x.cpp
        analog_comparator.cpp
        avr_cpu.cpp
        $<$<NOT:$<PLATFORM_ID:Windows>>:avr_jit.cpp>
        cpu_control.cpp
        device.cpp
        device_catalog.cpp
        $<$<NOT:$<PLATFORM_ID:Windows>>:gdb_stub.cpp>
        eeprom.cpp
        ext_interrupt.cpp
        gpio_port.cpp
        hc05.cpp
        hex_image.cpp
        io_peripheral.cpp
        memory_bus.cpp
        mvio.cpp
        pin_change_interrupt.cpp
        pin_mux.cpp
        register_file.cpp
        spi.cpp
        spi8x.cpp
        can.cpp
        twi.cpp
        twi8x.cpp
        usi.cpp
        sync_engine.cpp
        trace_buffer.cpp
        viospice.cpp
        viospice_c.cpp
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
        usb8x.cpp
        eusart.cpp
        pll.cpp
        xmem.cpp
        psc.cpp
        dac.cpp
        dac8x.cpp
        nvm_ctrl.cpp
        cpu_int.cpp
        dma.cpp
        zcd.cpp
        lin.cpp
        amplifier.cpp
        opamp.cpp
        ptc.cpp
        cfd.cpp
        tcd.cpp
        tce.cpp
        tc.cpp
        adcea.cpp
        awex.cpp
        ebi.cpp
        ircom.cpp
        xmega_dac.cpp
        xmegadc.cpp
        xmegaac.cpp
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

set_target_properties(vioavr_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(NOT VIOAVR_DISABLE_UNITY)
    set_target_properties(vioavr_core PROPERTIES UNITY_BUILD ON)
    set_target_properties(vioavr_core PROPERTIES UNITY_BUILD_BATCH_SIZE 12)
endif()
if(DEFINED VIOAVR_DISABLE_UNITY AND VIOAVR_DISABLE_UNITY)
    message(STATUS "vioavr_core: unity build disabled by VIOAVR_DISABLE_UNITY=ON")
endif()

if(NOT VIOAVR_DISABLE_UNITY)
    set_source_files_properties(tcd.cpp ptc.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
endif()

# ── POSIX SHM bridge (Linux/macOS only) ────────────────────────────
if(HAVE_SHM_OPEN)
    target_sources(vioavr_core PRIVATE
        bridge_shm_server.cpp
        bridge_shm_client.cpp
    )
    target_compile_definitions(vioavr_core PUBLIC HAVE_SHM_OPEN=1)
endif()

# rt library is Linux-only (clock_gettime etc. in libc on macOS/BSD)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(vioavr_core PRIVATE rt)
endif()
target_link_libraries(vioavr_core PRIVATE Threads::Threads)
