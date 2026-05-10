# CMakeLists.txt for CustomAVR Test Kit

# Add custom chip test executable
add_executable(vioavr_custom_avr_test
    test_custom_avr.cpp
)

target_link_libraries(vioavr_custom_avr_test PRIVATE VioAVR::core)

# Include the custom chip header directory
target_include_directories(vioavr_custom_avr_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../core
)

# Add to test suite
add_test(NAME vioavr_custom_avr_full_suite COMMAND vioavr_custom_avr_test)

# Individual test categories (can run separately)
add_test(NAME vioavr_custom_memory_map 
         COMMAND vioavr_custom_avr_test -tc="*[CustomAVR]*Memory Map*")
add_test(NAME vioavr_custom_gpio_map 
         COMMAND vioavr_custom_avr_test -tc="*[CustomAVR]*GPIO Port*")
add_test(NAME vioavr_custom_timer_map 
         COMMAND vioavr_custom_avr_test -tc="*[CustomAVR]*Timer Register*")
add_test(NAME vioavr_custom_uart_map 
         COMMAND vioavr_custom_avr_test -tc="*[CustomAVR]*UART Register*")
add_test(NAME vioavr_custom_pin_mux 
         COMMAND vioavr_custom_avr_test -tc="*[CustomAVR]*Pin Mux*")
add_test(NAME vioavr_custom_interrupts 
         COMMAND vioavr_custom_avr_test -tc="*[CustomAVR]*Interrupt Vector*")
add_test(NAME vioavr_custom_functional 
         COMMAND vioavr_custom_avr_test -tc="*[CustomAVR]*Functional*")
add_test(NAME vioavr_custom_timing 
         COMMAND vioavr_custom_avr_test -tc="*[CustomAVR]*Timing*")
add_test(NAME vioavr_custom_reset 
         COMMAND vioavr_custom_avr_test -tc="*[CustomAVR]*Reset State*")
add_test(NAME vioavr_custom_priority 
         COMMAND vioavr_custom_avr_test -tc="*[CustomAVR]*Pin Mux Priority*")
