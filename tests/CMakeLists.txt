cmake_minimum_required(VERSION 3.25)

find_program(AVR_GCC avr-gcc)
find_program(AVR_OBJCOPY avr-objcopy)

if(AVR_GCC AND AVR_OBJCOPY)
    message(STATUS "Found avr-gcc: ${AVR_GCC}")
    message(STATUS "Found avr-objcopy: ${AVR_OBJCOPY}")

    set(AVR_MCU "atmega328p" CACHE STRING "AVR MCU for C tests")
    set(AVR_F_CPU "16000000UL" CACHE STRING "AVR CPU Frequency for C tests")

    function(add_avr_test name source)
        set(elf_file "${CMAKE_CURRENT_BINARY_DIR}/${name}.elf")
        set(hex_file "${CMAKE_CURRENT_BINARY_DIR}/${name}.hex")

        set(TEST_MCU ${AVR_MCU})
        set(EXTRA_FLAGS "")

        # Detect AVR8X (MegaX) tests
        if(name MATCHES "8x" OR name MATCHES "avr8x")
            set(TEST_MCU "atmega4809")
            # Point to DFP if not in standard gcc paths
            # Note: -B specifies where to find specs and crt files
            list(APPEND EXTRA_FLAGS "-B${ATMEGA_DFP_DIR}/gcc/dev/atmega4809")
            list(APPEND EXTRA_FLAGS "-I${ATMEGA_DFP_DIR}/include")
        endif()

        # Detect AT90PWM tests
        if(name MATCHES "at90pwm" OR name MATCHES "psc" OR name MATCHES "adc_pwm")
            set(TEST_MCU "at90pwm316")
        endif()

        # Detect ATmega16 tests
        if(name MATCHES "^atmega16_" OR name MATCHES "^atmega16$")
            set(TEST_MCU "atmega16")
        endif()

        # Detect ATmega8 tests
        if(name MATCHES "^atmega8_" OR name MATCHES "^atmega8$")
            set(TEST_MCU "atmega8")
        endif()

        # Detect ATmega808 tests
        if(name MATCHES "^atmega808_" OR name MATCHES "^atmega808$")
            set(TEST_MCU "atmega808")
            list(APPEND EXTRA_FLAGS "-B${ATMEGA_DFP_DIR}/gcc/dev/atmega808")
            list(APPEND EXTRA_FLAGS "-I${ATMEGA_DFP_DIR}/include")
        endif()

        add_custom_command(
            OUTPUT ${elf_file}
            COMMAND ${AVR_GCC} -mmcu=${TEST_MCU} ${EXTRA_FLAGS} -Os -DF_CPU=${AVR_F_CPU} -o ${elf_file} ${source}
            DEPENDS ${source}
            COMMENT "Compiling ${source} to ${elf_file} (MCU: ${TEST_MCU})"
        )

        add_custom_command(
            OUTPUT ${hex_file}
            COMMAND ${AVR_OBJCOPY} -j .text -j .data -O ihex ${elf_file} ${hex_file}
            DEPENDS ${elf_file}
            COMMENT "Generating ${hex_file}"
        )

        add_custom_target(${name}_hex ALL DEPENDS ${hex_file})
    endfunction()

    file(GLOB C_TEST_SOURCES "*.c" "firmware/*.c")
    foreach(source IN LISTS C_TEST_SOURCES)
        get_filename_component(name ${source} NAME_WE)
        add_avr_test(${name} ${source})
    endforeach()
else()
    message(WARNING "avr-gcc or avr-objcopy not found. AVR C tests will not be built.")
endif()

# Custom Chip Test Kit
add_subdirectory(custom_chip)

# Host SimAVR and comparison tests
set(SIMAVR_ROOT "/home/jnd/cpp_projects/simavr" CACHE PATH "Path to SimAVR root directory")
find_path(SIMAVR_INCLUDE_DIR NAMES sim_avr.h HINTS "${SIMAVR_ROOT}/simavr/sim")
find_library(SIMAVR_LIBRARY NAMES simavr HINTS "${SIMAVR_ROOT}/simavr/obj-x86_64-linux-gnu")

if(SIMAVR_INCLUDE_DIR AND SIMAVR_LIBRARY)
    message(STATUS "Found SimAVR headers: ${SIMAVR_INCLUDE_DIR}")
    message(STATUS "Found SimAVR library: ${SIMAVR_LIBRARY}")

    add_executable(simavr_tracer programs/simavr_tracer.c)
    target_include_directories(simavr_tracer PRIVATE ${SIMAVR_INCLUDE_DIR})
    target_link_libraries(simavr_tracer PRIVATE ${SIMAVR_LIBRARY} rt)
    set_target_properties(simavr_tracer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")

    function(add_comparison_test test_name cycles)
        set(elf_file "${CMAKE_CURRENT_BINARY_DIR}/${test_name}.elf")
        add_test(
            NAME "compare_${test_name}"
            COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/programs/simavr_compare.py" "${elf_file}" "${cycles}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        )
    endfunction()

    add_comparison_test(adc_pwm 5000)
    add_comparison_test(adc_test_compare 5000)
    add_comparison_test(timer1_interrupt_compare 2000)
    add_comparison_test(timer_pwm_compare 1000)
    add_comparison_test(uart_test_compare 10000)
else()
    message(WARNING "SimAVR headers or library not found. SimAVR comparison tests will be skipped.")
endif()

