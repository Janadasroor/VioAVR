set(TEST_SOURCES
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
    avr8x/avr8x_tca_fidelity_test.cpp
    avr8x/avr8x_tce_fidelity_test.cpp
    xmega/xmega_tc_fidelity_test.cpp
    xmega/xmega_awex_fidelity_test.cpp
    xmega/xmega_adc_fidelity_test.cpp
    xmega/xmega_ac_fidelity_test.cpp
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
    can/can_fidelity_test.cpp
    can/can_protocol_fidelity_test.cpp
    can/can_stability_test.cpp
    can/lin_protocol_fidelity_test.cpp
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
    cpu/gdb_stub_test.cpp
    cpu/hex_loader_test.cpp
    cpu/instruction_timing_fidelity_test.cpp
    cpu/smoke_test.cpp
    cpu/sync_engine_test.cpp
    cpu_bench_util.cpp
    cpu_trace_util.cpp
    peripherals/adc_fidelity_test.cpp
    peripherals/atmega8_firmware_test.cpp
    peripherals/atmega808_firmware_test.cpp
    peripherals/at90pwm316_adc_pwm_integration_test.cpp
    peripherals/at90pwm316_fidelity_test.cpp
    peripherals/inverter_atmega328p_waveform_test.cpp
    peripherals/inverter_atmega328p_firmware_test.cpp
    peripherals/inverter_psc_firmware_test.cpp
    peripherals/inverter_psc_waveform_test.cpp
    peripherals/inverter_atmega16_firmware_test.cpp
    peripherals/inverter_atmega16_waveform_test.cpp
    peripherals/ext_int_firmware_test.cpp
    peripherals/analog_comparator_firmware_test.cpp
    peripherals/pin_change_firmware_test.cpp
    peripherals/timer2_async_firmware_test.cpp
    peripherals/twi_firmware_test.cpp
    peripherals/i2c_scanner_firmware_test.cpp
    peripherals/at90pwm316_pid_test.cpp
    peripherals/at90pwm_cycle_by_cycle_test.cpp
    peripherals/atmega2560_timers_test.cpp
    peripherals/atmega328p_voltmeter_test.cpp
    peripherals/atmega328p_sysmon_test.cpp
    peripherals/blinky_external_test.cpp
    peripherals/uart_hello_external_test.cpp
    peripherals/test0_firmware_test.cpp
    peripherals/test_interrupt_firmware_test.cpp
    peripherals/spi_loopback_test.cpp
    peripherals/wdt_firmware_test.cpp
    peripherals/dac_fidelity_test.cpp
    peripherals/dma_fidelity_test.cpp
    peripherals/eeprom_firmware_test.cpp
    peripherals/eeprom_fidelity_test.cpp
    peripherals/evsys_fidelity_test.cpp
    peripherals/ext_interrupt_fidelity_test.cpp
    peripherals/fetch_metadata_test.cpp
    peripherals/fuse_fidelity_test.cpp
    peripherals/cfd_fidelity_test.cpp
    peripherals/gpio_fidelity_test.cpp
    peripherals/gpio_readback_test.cpp
    peripherals/input_pin_fidelity_test.cpp
    peripherals/integration_stress_runner.cpp
    peripherals/lcd_3290_test.cpp
    peripherals/lcd_exhaustive_test.cpp
    peripherals/lcd_fidelity_test.cpp
    peripherals/lockbit_fidelity_test.cpp
    peripherals/math_fidelity_test.cpp
    peripherals/nvm_stall_fidelity_test.cpp
    peripherals/opamp_fidelity_test.cpp
    peripherals/ptc_fidelity_test.cpp
    peripherals/tcd_fidelity_test.cpp
    peripherals/performance_benchmark_test.cpp
    peripherals/jit_benchmark_test.cpp
    peripherals/jit_debug.cpp
    peripherals/jit_divergence_test.cpp
    peripherals/pin_mux_priority_test.cpp
    peripherals/pinmux_fidelity_test.cpp
    peripherals/pll_test.cpp
    peripherals/power_clock_fidelity_test.cpp
    peripherals/power_management_test.cpp
    peripherals/power_sleep_firmware_test.cpp
    peripherals/psc_firmware_test.cpp
    peripherals/psc_fidelity_test.cpp
    peripherals/signal_tracer_test.cpp
    peripherals/sleep_fidelity_test.cpp
    peripherals/soc_interconnect_test.cpp
    peripherals/spi_firmware_test.cpp
    peripherals/spi_fidelity_test.cpp
    peripherals/spm_fidelity_test.cpp
    peripherals/spm_test.cpp
    peripherals/timer16_fidelity_test.cpp
    peripherals/timer2_async_fidelity_test.cpp
    peripherals/twi_fidelity_test.cpp
    peripherals/usi_fidelity_test.cpp
    peripherals/uart_fidelity_test.cpp
    peripherals/uart_stress_test_runner.cpp
    peripherals/xmem_fidelity_test.cpp
    usb/usb_basic_test.cpp
    usb/usb_connectivity_test.cpp
    usb/usb_double_banking_test.cpp
    usb/usb_enumeration_test.cpp
    usb/usb_interrupt_test.cpp
    usb/usb_protocol_fidelity_test.cpp
    usb/usb_set_address_fidelity_test.cpp
    viospice/bridge_shm_test.cpp
    viospice/attiny_shm_test.cpp
    viospice/viospice_timer2_async_test.cpp
    viospice/viospice_all_devices_test.cpp
    viospice/viospice_ngspice_all_devices_test.cpp
    viospice/hc05_fidelity_test.cpp
    viospice/viospice_attiny_tca_pwm_test.cpp
    viospice/viospice_peripheral_functional_test.cpp
    attiny/attiny13_fidelity_test.cpp
    attiny/attiny85_fidelity_test.cpp
    attiny/attiny861_fidelity_test.cpp
    attiny/attiny412_fidelity_test.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

foreach(source IN LISTS TEST_SOURCES)
    get_filename_component(test_name ${source} NAME_WE)
    set(target_name "vioavr_${test_name}")
    
    add_executable(${target_name} ${source})
    target_link_libraries(${target_name} PRIVATE VioAVR::core)
    # Don't register utilities as tests
    if (NOT test_name STREQUAL "cpu_trace_util" AND NOT test_name STREQUAL "cpu_bench_util")
        add_test(NAME ${target_name} COMMAND ${target_name})
        set_tests_properties(${target_name} PROPERTIES TIMEOUT 45)
    endif()
    if (test_name STREQUAL "at90pwm316_pid_test")
        add_dependencies(${target_name} psc_pid_hex)
    endif()
    if (test_name STREQUAL "atmega328p_voltmeter_test")
        add_dependencies(${target_name} voltmeter_hex)
    endif()
    if (test_name STREQUAL "atmega328p_sysmon_test")
        add_dependencies(${target_name} sysmon_hex)
    endif()
    if (test_name STREQUAL "blinky_external_test")
        add_dependencies(${target_name} blinky_hex)
    endif()
    if (test_name STREQUAL "test0_firmware_test")
        add_dependencies(${target_name} test0_hex)
    endif()
    if (test_name STREQUAL "test_interrupt_firmware_test")
        add_dependencies(${target_name} test_interrupt_hex)
    endif()
    if (test_name STREQUAL "spi_loopback_test")
        add_dependencies(${target_name} spi_loopback_hex)
    endif()
    if (test_name STREQUAL "wdt_firmware_test")
        add_dependencies(${target_name} wdt_test_hex)
    endif()
    if (test_name STREQUAL "inverter_atmega328p_firmware_test")
        add_dependencies(${target_name} atmega328p_inverter_pwm_hex)
    endif()
    if (test_name STREQUAL "inverter_psc_firmware_test")
        add_dependencies(${target_name} inverter_psc_pwm_hex)
    endif()
    if (test_name STREQUAL "inverter_atmega16_firmware_test")
        add_dependencies(${target_name} atmega16_inverter_pwm_hex)
    endif()
    if (test_name STREQUAL "atmega8_firmware_test")
        add_dependencies(${target_name} atmega8_test_hex)
    endif()
    if (test_name STREQUAL "atmega808_firmware_test")
        add_dependencies(${target_name} atmega808_test_hex)
    endif()
    if (test_name STREQUAL "ext_int_firmware_test")
        add_dependencies(${target_name} ext_int_test_hex)
    endif()
    if (test_name STREQUAL "analog_comparator_firmware_test")
        add_dependencies(${target_name} analog_comparator_test_hex)
    endif()
    if (test_name STREQUAL "pin_change_firmware_test")
        add_dependencies(${target_name} pin_change_test_hex)
    endif()
    if (test_name STREQUAL "timer2_async_firmware_test")
        add_dependencies(${target_name} timer2_async_test_hex)
    endif()
    if (test_name STREQUAL "twi_firmware_test")
        add_dependencies(${target_name} twi_eeprom_test_hex)
    endif()
    if (test_name STREQUAL "i2c_scanner_firmware_test")
        add_dependencies(${target_name} i2c_scanner_loopback_hex)
    endif()
    if (test_name STREQUAL "eeprom_firmware_test")
        add_dependencies(${target_name} eeprom_test_hex)
    endif()
    if (test_name STREQUAL "spi_firmware_test")
        add_dependencies(${target_name} spi_test_hex)
    endif()
    if (test_name STREQUAL "psc_firmware_test")
        add_dependencies(${target_name} psc_test_hex)
    endif()
    if (test_name STREQUAL "math_fidelity_test")
        add_dependencies(${target_name} test_math_advanced_hex)
    endif()
    if (test_name STREQUAL "jit_benchmark_test")
        add_dependencies(${target_name} benchmark_hex)
    endif()
    if (test_name STREQUAL "jit_debug")
        add_dependencies(${target_name} benchmark_hex)
        add_dependencies(${target_name} integration_stress_hex)
    endif()
    if (test_name STREQUAL "jit_divergence_test")
        add_dependencies(${target_name} integration_stress_hex)
    endif()
    if (test_name STREQUAL "performance_benchmark_test")
        add_dependencies(${target_name} benchmark_hex)
        add_dependencies(${target_name} integration_stress_hex)
        set_tests_properties(${target_name} PROPERTIES TIMEOUT 120)
    endif()
endforeach()
