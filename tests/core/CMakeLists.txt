include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ---- Group definitions ----
# CPU (83 files → 5 groups of ~17)
set(CPU_GROUP_0_SRCS
    cpu/cpu_adc_auto_trigger_firmware_test.cpp
    cpu/cpu_adc_auto_trigger_test.cpp
    cpu/cpu_adc_descriptor_test.cpp
    cpu/cpu_adc_differential_test.cpp
    cpu/cpu_adc_ext_interrupt_auto_trigger_firmware_test.cpp
    cpu/cpu_adc_ext_interrupt_auto_trigger_test.cpp
    cpu/cpu_adc_firmware_test.cpp
    cpu/cpu_adc_free_running_firmware_test.cpp
    cpu/cpu_adc_free_running_test.cpp
    cpu/cpu_adc_test.cpp
    cpu/cpu_adc_timer_auto_trigger_firmware_test.cpp
    cpu/cpu_adc_timer_auto_trigger_test.cpp
    cpu/cpu_adc_timer_overflow_auto_trigger_firmware_test.cpp
    cpu/cpu_adc_timer_overflow_auto_trigger_test.cpp
    cpu/cpu_alu_test.cpp
    cpu/cpu_analog_comparator_firmware_test.cpp
    cpu/cpu_analog_comparator_test.cpp
)

set(CPU_GROUP_1_SRCS
    cpu/cpu_analog_digital_frontend_test.cpp
    cpu/cpu_analog_frontend_test.cpp
    cpu/cpu_analog_timer_clock_test.cpp
    cpu/cpu_bit_io_test.cpp
    cpu/cpu_branch_alias_test.cpp
    cpu/cpu_compare_test.cpp
    cpu/cpu_control_state_test.cpp
    cpu/cpu_control_sync_test.cpp
    cpu/cpu_ext_interrupt_firmware_test.cpp
    cpu/cpu_ext_interrupt_test.cpp
    cpu/cpu_extended_load_store_test.cpp
    cpu/cpu_fmul_test.cpp
    cpu/cpu_gpio_sync_test.cpp
    cpu/cpu_gpio_threshold_test.cpp
    cpu/cpu_jit_test.cpp
    cpu/cpu_indirect_control_test.cpp
    cpu/cpu_int_arbitration_test.cpp
)

set(CPU_GROUP_2_SRCS
    cpu/cpu_interrupt_chaining_test.cpp
    cpu/cpu_interrupt_priority_test.cpp
    cpu/cpu_interrupt_test.cpp
    cpu/cpu_io_test.cpp
    cpu/cpu_load_store_test.cpp
    cpu/cpu_lpm_test.cpp
    cpu/cpu_multibyte_compare_test.cpp
    cpu/cpu_pin_change_interrupt_firmware_test.cpp
    cpu/cpu_pin_change_interrupt_groups_test.cpp
    cpu/cpu_pin_change_interrupt_test.cpp
    cpu/cpu_register_mapping_test.cpp
    cpu/cpu_reset_state_test.cpp
    cpu/cpu_spi_test.cpp
    cpu/cpu_spm_boundary_test.cpp
    cpu/cpu_spm_firmware_test.cpp
    cpu/cpu_spm_lpm_fidelity_test.cpp
    cpu/cpu_stack_test.cpp
)

set(CPU_GROUP_3_SRCS
    cpu/cpu_timer10_deadtime_test.cpp
    cpu/cpu_timer10_fidelity_test.cpp
    cpu/cpu_timer10_pfc_test.cpp
    cpu/cpu_timer16_test.cpp
    cpu/cpu_timer2_assr_test.cpp
    cpu/cpu_timer2_async_test.cpp
    cpu/cpu_timer2_descriptor_test.cpp
    cpu/cpu_timer2_gpio_test.cpp
    cpu/cpu_timer_control_test.cpp
    cpu/cpu_timer_descriptor_test.cpp
    cpu/cpu_timer_dual_interrupt_test.cpp
    cpu/cpu_timer_external_clock_firmware_test.cpp
    cpu/cpu_timer_external_clock_test.cpp
    cpu/cpu_timer_fidelity_test.cpp
    cpu/cpu_timer_gpio_test.cpp
    cpu/cpu_timer_pwm_test.cpp
    cpu/cpu_timer_test.cpp
)

set(CPU_GROUP_4_SRCS
    cpu/cpu_timer_waveform_test.cpp
    cpu/cpu_trace_util_test.cpp
    cpu/cpu_twi_test.cpp
    cpu/cpu_uart_firmware_test.cpp
    cpu/cpu_uart_interrupt_test.cpp
    cpu/cpu_uart_test.cpp
    cpu/cpu_voltage_interrupt_test.cpp
    cpu/cpu_watchdog_test.cpp
    cpu/cpu_word_ops_test.cpp
    cpu/device_catalog_test.cpp
    cpu/hex_loader_test.cpp
    cpu/instruction_timing_fidelity_test.cpp
    cpu/smoke_test.cpp
    cpu/sync_engine_test.cpp
)

# AVR8X (54 files → 3 groups of 18)
set(AVR8X_GROUP_0_SRCS
    avr8x/avr8x_ac_extended_test.cpp
    avr8x/avr8x_ac_fidelity_test.cpp
    avr8x/avr8x_ac8x_extended_test.cpp
    avr8x/avr8x_adc8x_extended_test.cpp
    avr8x/avr8x_adc_fidelity_test.cpp
    avr8x/avr8x_ccl_evsys_fidelity_test.cpp
    avr8x/avr8x_ccl_fidelity_test.cpp
    avr8x/avr8x_ccl_rslatch_test.cpp
    avr8x/avr8x_ccl_seq_fidelity_test.cpp
    avr8x/avr8x_ccl_sequential_test.cpp
    avr8x/avr8x_ccl_sequential_unit_test.cpp
    avr8x/avr8x_adc_free_running_test.cpp
    avr8x/avr8x_bod_extended_test.cpp
    avr8x/avr8x_clkctrl_fidelity_test.cpp
    avr8x/avr8x_clkctrl_extended_test.cpp
    avr8x/avr8x_cpu_int_test.cpp
    avr8x/avr8x_cpu_system_test.cpp
    avr8x/avr8x_cpuint_extended_test.cpp
)

set(AVR8X_GROUP_1_SRCS
    avr8x/avr8x_cpuint_fidelity_test.cpp
    avr8x/avr8x_crc_extended_test.cpp
    avr8x/avr8x_crc_fidelity_test.cpp
    avr8x/avr8x_evsys_chain_test.cpp
    avr8x/avr8x_evsys_dma_chain_test.cpp
    avr8x/avr8x_evsys_extended_test.cpp
    avr8x/avr8x_evsys_fidelity_test.cpp
    avr8x/avr8x_evsys_test.cpp
    avr8x/avr8x_gpio_fidelity_test.cpp
    avr8x/avr8x_gpio_extended_test.cpp
    avr8x/avr8x_nvm_extended_test.cpp
    avr8x/avr8x_nvm_fidelity_test.cpp
    avr8x/avr8x_rtc_extended_test.cpp
    avr8x/avr8x_rtc_fidelity_test.cpp
    avr8x/avr8x_spi_extended_test.cpp
    avr8x/avr8x_spi_fidelity_test.cpp
    avr8x/avr8x_spi_multi_byte_test.cpp
    avr8x/avr8x_sysctrl_fidelity_test.cpp
)

set(AVR8X_GROUP_2_SRCS
    avr8x/avr8x_tca_fidelity_test.cpp
    avr8x/avr8x_tce_fidelity_test.cpp
    avr8x/avr8x_adcea_fidelity_test.cpp
    avr8x/avr8x_tca_frq_mode_test.cpp
    avr8x/avr8x_tca_pwm_period_test.cpp
    avr8x/avr8x_tcb_fidelity_test.cpp
    avr8x/avr8x_tcb_pulse_width_test.cpp
    avr8x/avr8x_twi_extended_test.cpp
    avr8x/avr8x_twi_fidelity_test.cpp
    avr8x/avr8x_uart_evsys_fidelity_test.cpp
    avr8x/avr8x_unified_map_test.cpp
    avr8x/avr8x_usart_extended_test.cpp
    avr8x/avr8x_usart_fidelity_test.cpp
    avr8x/avr8x_wdt_extended_test.cpp
    avr8x/avr8x_wdt_fidelity_test.cpp
    avr8x/avr8x_wdt_timeout_test.cpp
    avr8x/avr8x_zcd_extended_test.cpp
    avr8x/avr8x_zcd_fidelity_test.cpp
)

# Peripherals (77 files → 4 groups of ~19)
set(PERIPH_GROUP_0_SRCS
    peripherals/adc_fidelity_test.cpp
    peripherals/at90pwm316_fidelity_test.cpp
    peripherals/inverter_atmega328p_waveform_test.cpp
    peripherals/inverter_psc_waveform_test.cpp
    peripherals/inverter_atmega16_waveform_test.cpp
    peripherals/at90pwm_cycle_by_cycle_test.cpp
)
if(AVR_FW_AVAILABLE)
    list(APPEND PERIPH_GROUP_0_SRCS
        peripherals/atmega8_firmware_test.cpp
        peripherals/atmega808_firmware_test.cpp
        peripherals/at90pwm316_adc_pwm_integration_test.cpp
        peripherals/inverter_atmega328p_firmware_test.cpp
        peripherals/inverter_psc_firmware_test.cpp
        peripherals/inverter_atmega16_firmware_test.cpp
        peripherals/ext_int_firmware_test.cpp
        peripherals/analog_comparator_firmware_test.cpp
        peripherals/pin_change_firmware_test.cpp
        peripherals/timer2_async_firmware_test.cpp
        peripherals/twi_firmware_test.cpp
        peripherals/i2c_scanner_firmware_test.cpp
        peripherals/at90pwm316_pid_test.cpp
    )
endif()

set(PERIPH_GROUP_1_SRCS
    peripherals/atmega2560_timers_test.cpp
    peripherals/dac_fidelity_test.cpp
    peripherals/dac8x_fidelity_test.cpp
    peripherals/dma_fidelity_test.cpp
    peripherals/eeprom_fidelity_test.cpp
    peripherals/evsys_fidelity_test.cpp
    peripherals/fetch_metadata_test.cpp
    peripherals/fuse_fidelity_test.cpp
    peripherals/cfd_fidelity_test.cpp
    peripherals/zcd_fidelity_test.cpp
    peripherals/mvio_fidelity_test.cpp
    peripherals/lin_uart_fidelity_test.cpp
    peripherals/gpio_fidelity_test.cpp
)
if(AVR_FW_AVAILABLE)
    list(APPEND PERIPH_GROUP_1_SRCS
        peripherals/atmega328p_sysmon_test.cpp
        peripherals/atmega328p_voltmeter_test.cpp
        peripherals/blinky_external_test.cpp
        peripherals/uart_hello_external_test.cpp
        peripherals/test0_firmware_test.cpp
        peripherals/test_interrupt_firmware_test.cpp
        peripherals/spi_loopback_test.cpp
        peripherals/wdt_firmware_test.cpp
        peripherals/eeprom_firmware_test.cpp
        peripherals/ext_interrupt_fidelity_test.cpp
    )
endif()

set(PERIPH_GROUP_2_SRCS
    peripherals/gpio_readback_test.cpp
    peripherals/lcd_3290_test.cpp
    peripherals/lcd_exhaustive_test.cpp
    peripherals/lcd_fidelity_test.cpp
    peripherals/lockbit_fidelity_test.cpp
    peripherals/nvm_stall_fidelity_test.cpp
    peripherals/opamp_fidelity_test.cpp
    peripherals/ptc_fidelity_test.cpp
    peripherals/tcd_fidelity_test.cpp
    peripherals/pin_mux_priority_test.cpp
    peripherals/pinmux_fidelity_test.cpp
    peripherals/pll_test.cpp
)
if(AVR_FW_AVAILABLE)
    list(APPEND PERIPH_GROUP_2_SRCS
        peripherals/input_pin_fidelity_test.cpp
        peripherals/integration_stress_runner.cpp
        peripherals/math_fidelity_test.cpp
        peripherals/performance_benchmark_test.cpp
        peripherals/jit_benchmark_test.cpp
        peripherals/jit_debug.cpp
        peripherals/jit_divergence_test.cpp
    )
endif()

set(PERIPH_GROUP_3_SRCS
    peripherals/power_clock_fidelity_test.cpp
    peripherals/power_management_test.cpp
    peripherals/psc_fidelity_test.cpp
    peripherals/signal_tracer_test.cpp
    peripherals/sleep_fidelity_test.cpp
    peripherals/soc_interconnect_test.cpp
    peripherals/spi_fidelity_test.cpp
    peripherals/spm_fidelity_test.cpp
    peripherals/spm_test.cpp
    peripherals/timer16_fidelity_test.cpp
    peripherals/timer2_async_fidelity_test.cpp
    peripherals/twi_fidelity_test.cpp
    peripherals/usi_fidelity_test.cpp
    peripherals/uart_fidelity_test.cpp
    peripherals/xmem_fidelity_test.cpp
)
if(AVR_FW_AVAILABLE)
    list(APPEND PERIPH_GROUP_3_SRCS
        peripherals/power_sleep_firmware_test.cpp
        peripherals/psc_firmware_test.cpp
        peripherals/spi_firmware_test.cpp
        peripherals/uart_stress_test_runner.cpp
    )
endif()

set(XMEGA_GROUP_SRCS
    xmega/xmega_tc_fidelity_test.cpp
    xmega/xmega_awex_fidelity_test.cpp
    xmega/xmega_dac_fidelity_test.cpp
    xmega/xmega_adc_fidelity_test.cpp
    xmega/xmega_ac_fidelity_test.cpp
    xmega/xmega_ebi_fidelity_test.cpp
    xmega/xmega_ircom_fidelity_test.cpp
)

set(USB_GROUP_SRCS
    usb/usb_basic_test.cpp
    usb/usb_connectivity_test.cpp
    usb/usb_double_banking_test.cpp
    usb/usb_enumeration_test.cpp
    usb/usb_interrupt_test.cpp
    usb/usb_protocol_fidelity_test.cpp
    usb/usb8x_fidelity_test.cpp
    usb/usb_set_address_fidelity_test.cpp
)

set(VIOSPICE_GROUP_SRCS
    viospice/viospice_timer2_async_test.cpp
    viospice/viospice_all_devices_test.cpp
    viospice/hc05_fidelity_test.cpp
)

# Cosimulation tests require VioMATRIXC (ngspice fork) + libavr_cosim.so
if(VIOAVR_BUILD_COSIM)
    list(APPEND VIOSPICE_GROUP_SRCS
        viospice/viospice_ngspice_all_devices_test.cpp
        viospice/viospice_attiny_tca_pwm_test.cpp
        viospice/viospice_peripheral_functional_test.cpp
        viospice/viospice_xmega_tc_pwm_test.cpp
        viospice/viospice_xmega_ac_adc_test.cpp
        viospice/viospice_xmega_awex_test.cpp
        viospice/viospice_xmega_dac_test.cpp
        viospice/viospice_multi_i2c_test.cpp
        viospice/viospice_atmega4809_tca_pwm_test.cpp
    )
endif()

# SHM bridge tests require POSIX shared memory (shm_open)
if(HAVE_SHM_OPEN)
    list(PREPEND VIOSPICE_GROUP_SRCS viospice/attiny_shm_test.cpp)
endif()

set(ATTINY_GROUP_SRCS
    attiny/attiny13_fidelity_test.cpp
    attiny/attiny85_fidelity_test.cpp
    attiny/attiny861_fidelity_test.cpp
    attiny/attiny412_fidelity_test.cpp
    attiny/attiny1634_fidelity_test.cpp
)

set(CAN_GROUP_SRCS
    can/can_fidelity_test.cpp
    can/can_protocol_fidelity_test.cpp
    can/can_stability_test.cpp
    can/lin_protocol_fidelity_test.cpp
)

# ---- Non-doctest standalone tests ----
set(NON_DOCTEST_TESTS
    c_api_test.c
    c_api_misuse_test.c
)

if(NOT WIN32)
    list(APPEND NON_DOCTEST_TESTS cpu/gdb_stub_test.cpp)
endif()

if(HAVE_SHM_OPEN)
    list(APPEND NON_DOCTEST_TESTS viospice/bridge_shm_test.cpp)
endif()

# ---- Utility targets (not registered as CTest tests) ----
set(UTILITY_SOURCES
    cpu_bench_util.cpp
    cpu_trace_util.cpp
)

# ---- Helper: create a group executable with doctest ----
function(add_doctest_group TARGET_NAME)
    set(sources ${ARGN})
    add_executable(${TARGET_NAME} _group_main.cpp ${sources})
    target_link_libraries(${TARGET_NAME} PRIVATE VioAVR::core)
    # Allow duplicate helper functions (e.g., step_to) across test files
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_link_options(${TARGET_NAME} PRIVATE "LINKER:--allow-multiple-definition")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_link_options(${TARGET_NAME} PRIVATE "/FORCE:MULTIPLE")
    endif()
    
    # Register one CTest entry per group (runs all test cases in the binary)
    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
    set_tests_properties(${TARGET_NAME} PROPERTIES TIMEOUT 45)
    
    # Collect all dependencies and special timeouts for the group
    set(deps "")
    set(base_timeout 45)
    if(VIOAVR_ENABLE_COVERAGE)
        set(base_timeout 600)
    endif()
    set(group_timeout ${base_timeout})
    foreach(src ${sources})
        get_filename_component(test_name ${src} NAME_WE)
        if(test_name STREQUAL "viospice_ngspice_all_devices_test")
            if(NOT VIOAVR_ENABLE_COVERAGE)
                set(group_timeout 300)
            endif()
        endif()
        if((test_name STREQUAL "performance_benchmark_test" OR test_name STREQUAL "jit_benchmark_test") AND group_timeout LESS 180)
            set(group_timeout 180)
        endif()
        if(test_name STREQUAL "at90pwm316_pid_test")
            list(APPEND deps psc_pid_hex)
        elseif(test_name STREQUAL "atmega328p_voltmeter_test")
            list(APPEND deps voltmeter_hex)
        elseif(test_name STREQUAL "atmega328p_sysmon_test")
            list(APPEND deps sysmon_hex)
        elseif(test_name STREQUAL "blinky_external_test")
            list(APPEND deps blinky_hex)
        elseif(test_name STREQUAL "test0_firmware_test")
            list(APPEND deps test0_hex)
        elseif(test_name STREQUAL "test_interrupt_firmware_test")
            list(APPEND deps test_interrupt_hex)
        elseif(test_name STREQUAL "spi_loopback_test")
            list(APPEND deps spi_loopback_hex)
        elseif(test_name STREQUAL "wdt_firmware_test")
            list(APPEND deps wdt_test_hex)
        elseif(test_name STREQUAL "inverter_atmega328p_firmware_test")
            list(APPEND deps atmega328p_inverter_pwm_hex)
        elseif(test_name STREQUAL "inverter_psc_firmware_test")
            list(APPEND deps inverter_psc_pwm_hex)
        elseif(test_name STREQUAL "inverter_atmega16_firmware_test")
            list(APPEND deps atmega16_inverter_pwm_hex)
        elseif(test_name STREQUAL "atmega8_firmware_test")
            list(APPEND deps atmega8_test_hex)
        elseif(test_name STREQUAL "atmega808_firmware_test")
            list(APPEND deps atmega808_test_hex)
        elseif(test_name STREQUAL "ext_int_firmware_test")
            list(APPEND deps ext_int_test_hex)
        elseif(test_name STREQUAL "analog_comparator_firmware_test")
            list(APPEND deps analog_comparator_test_hex)
        elseif(test_name STREQUAL "pin_change_firmware_test")
            list(APPEND deps pin_change_test_hex)
        elseif(test_name STREQUAL "timer2_async_firmware_test")
            list(APPEND deps timer2_async_test_hex)
        elseif(test_name STREQUAL "twi_firmware_test")
            list(APPEND deps twi_eeprom_test_hex)
        elseif(test_name STREQUAL "i2c_scanner_firmware_test")
            list(APPEND deps i2c_scanner_loopback_hex)
        elseif(test_name STREQUAL "eeprom_firmware_test")
            list(APPEND deps eeprom_test_hex)
        elseif(test_name STREQUAL "spi_firmware_test")
            list(APPEND deps spi_test_hex)
        elseif(test_name STREQUAL "psc_firmware_test")
            list(APPEND deps psc_test_hex)
        elseif(test_name STREQUAL "math_fidelity_test")
            list(APPEND deps test_math_advanced_hex)
        elseif(test_name STREQUAL "jit_benchmark_test")
            list(APPEND deps benchmark_hex)
        elseif(test_name STREQUAL "jit_debug")
            list(APPEND deps benchmark_hex)
            list(APPEND deps integration_stress_hex)
        elseif(test_name STREQUAL "jit_divergence_test")
            list(APPEND deps integration_stress_hex)
        elseif(test_name STREQUAL "uart_stress_test_runner")
            list(APPEND deps uart_stress_test_hex)
        endif()
    endforeach()
    
    set_tests_properties(${TARGET_NAME} PROPERTIES TIMEOUT ${group_timeout})
    if(deps)
        list(REMOVE_DUPLICATES deps)
        # Only add dependencies that actually exist (avr-gcc may not be installed)
        set(existing_deps "")
        foreach(dep ${deps})
            if(TARGET ${dep})
                list(APPEND existing_deps ${dep})
            endif()
        endforeach()
        if(existing_deps)
            add_dependencies(${TARGET_NAME} ${existing_deps})
        endif()
    endif()
endfunction()

# ---- Create group executables (5 CPU groups) ----
add_doctest_group(vioavr_cpu_group_0 ${CPU_GROUP_0_SRCS})
add_doctest_group(vioavr_cpu_group_1 ${CPU_GROUP_1_SRCS})
add_doctest_group(vioavr_cpu_group_2 ${CPU_GROUP_2_SRCS})
add_doctest_group(vioavr_cpu_group_3 ${CPU_GROUP_3_SRCS})
add_doctest_group(vioavr_cpu_group_4 ${CPU_GROUP_4_SRCS})

# ---- 3 AVR8X groups ----
add_doctest_group(vioavr_avr8x_group_0 ${AVR8X_GROUP_0_SRCS})
add_doctest_group(vioavr_avr8x_group_1 ${AVR8X_GROUP_1_SRCS})
add_doctest_group(vioavr_avr8x_group_2 ${AVR8X_GROUP_2_SRCS})

# ---- 4 Peripherals groups ----
add_doctest_group(vioavr_periph_group_0 ${PERIPH_GROUP_0_SRCS})
add_doctest_group(vioavr_periph_group_1 ${PERIPH_GROUP_1_SRCS})
add_doctest_group(vioavr_periph_group_2 ${PERIPH_GROUP_2_SRCS})
add_doctest_group(vioavr_periph_group_3 ${PERIPH_GROUP_3_SRCS})

# ---- Smaller groups ----
add_doctest_group(vioavr_xmega_group ${XMEGA_GROUP_SRCS})
add_doctest_group(vioavr_usb_group ${USB_GROUP_SRCS})
add_doctest_group(vioavr_viospice_group ${VIOSPICE_GROUP_SRCS})
add_doctest_group(vioavr_attiny_group ${ATTINY_GROUP_SRCS})
add_doctest_group(vioavr_can_group ${CAN_GROUP_SRCS})

# ---- Standalone non-doctest tests ----
foreach(src ${NON_DOCTEST_TESTS})
    get_filename_component(test_name ${src} NAME_WE)
    set(target_name "vioavr_${test_name}")
    add_executable(${target_name} ${src})
    target_link_libraries(${target_name} PRIVATE VioAVR::core)
    add_test(NAME ${target_name} COMMAND ${target_name})
    set_tests_properties(${target_name} PROPERTIES TIMEOUT 45)

    # c_api_test needs a real .hex file; inject the path via environment so
    # there are no hardcoded machine-local paths in the source code.
    # When avr-gcc is absent, blinky.hex won't exist — the test skips cleanly
    # by returning exit code 77 (CTest SKIP_RETURN_CODE convention).
    if(test_name STREQUAL "c_api_test")
        if(TARGET blinky_hex)
            add_dependencies(${target_name} blinky_hex)
        endif()
        set_tests_properties(${target_name} PROPERTIES
            ENVIRONMENT "VIOAVR_BLINKY_HEX=${CMAKE_BINARY_DIR}/tests/blinky.hex;VIOAVR_AVR_FW=${AVR_FW_AVAILABLE}"
            SKIP_RETURN_CODE 77
        )
    endif()
endforeach()

# ---- Utility targets (not CTest tests) ----
foreach(src ${UTILITY_SOURCES})
    get_filename_component(test_name ${src} NAME_WE)
    set(target_name "vioavr_${test_name}")
    add_executable(${target_name} ${src})
    target_link_libraries(${target_name} PRIVATE VioAVR::core)
endforeach()
