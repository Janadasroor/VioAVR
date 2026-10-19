cmake_minimum_required(VERSION 3.25)

find_program(AVR_GCC avr-gcc)
find_program(AVR_OBJCOPY avr-objcopy)

# Track whether firmware hex targets are available
# (used by tests/core/CMakeLists.txt to skip deps on missing fw targets)
if(AVR_GCC AND AVR_OBJCOPY)
    set(AVR_FW_AVAILABLE TRUE)
else()
    set(AVR_FW_AVAILABLE FALSE)
endif()

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

add_subdirectory(core)

