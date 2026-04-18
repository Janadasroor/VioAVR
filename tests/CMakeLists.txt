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

        add_custom_command(
            OUTPUT ${elf_file}
            COMMAND ${AVR_GCC} -mmcu=${AVR_MCU} -Os -DF_CPU=${AVR_F_CPU} -o ${elf_file} ${source}
            DEPENDS ${source}
            COMMENT "Compiling ${source} to ${elf_file}"
        )

        add_custom_command(
            OUTPUT ${hex_file}
            COMMAND ${AVR_OBJCOPY} -j .text -j .data -O ihex ${elf_file} ${hex_file}
            DEPENDS ${elf_file}
            COMMENT "Generating ${hex_file}"
        )

        add_custom_target(${name}_hex ALL DEPENDS ${hex_file})
        
        # Copy to tests/ directory for convenience (or maybe just keep in build)
        # For now, keep it in build and maybe add a test that uses it.
    endfunction()

    file(GLOB C_TEST_SOURCES "*.c" "firmware/*.c")
    foreach(source IN LISTS C_TEST_SOURCES)
        get_filename_component(name ${source} NAME_WE)
        add_avr_test(${name} ${source})
    endforeach()
else()
    message(WARNING "avr-gcc or avr-objcopy not found. AVR C tests will not be built.")
endif()
