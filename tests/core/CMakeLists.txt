set(TEST_SOURCES
    smoke_test.cpp
    device_catalog_test.cpp
    cpu_alu_test.cpp
    cpu_analog_comparator_test.cpp
    cpu_analog_comparator_firmware_test.cpp
    cpu_analog_digital_frontend_test.cpp
    cpu_analog_frontend_test.cpp
    cpu_analog_timer_clock_test.cpp
    cpu_adc_test.cpp
    cpu_adc_firmware_test.cpp
    cpu_adc_descriptor_test.cpp
    cpu_timer_descriptor_test.cpp
    cpu_timer2_descriptor_test.cpp
    cpu_timer2_assr_test.cpp
    cpu_timer2_async_test.cpp
    cpu_adc_free_running_test.cpp
    cpu_adc_free_running_firmware_test.cpp
    cpu_adc_auto_trigger_test.cpp
    cpu_adc_auto_trigger_firmware_test.cpp
    cpu_adc_ext_interrupt_auto_trigger_test.cpp
    cpu_adc_ext_interrupt_auto_trigger_firmware_test.cpp
    cpu_adc_timer_auto_trigger_test.cpp
    cpu_adc_timer_auto_trigger_firmware_test.cpp
    cpu_adc_timer_overflow_auto_trigger_test.cpp
    cpu_adc_timer_overflow_auto_trigger_firmware_test.cpp
    cpu_branch_alias_test.cpp
    cpu_bit_io_test.cpp
    cpu_compare_test.cpp
    cpu_control_state_test.cpp
    cpu_control_sync_test.cpp
    cpu_extended_load_store_test.cpp
    cpu_ext_interrupt_test.cpp
    cpu_ext_interrupt_firmware_test.cpp
    cpu_fmul_test.cpp
    cpu_gpio_sync_test.cpp
    cpu_gpio_threshold_test.cpp
    cpu_indirect_control_test.cpp
    cpu_interrupt_test.cpp
    cpu_interrupt_priority_test.cpp
    cpu_interrupt_chaining_test.cpp
    cpu_reset_state_test.cpp
    cpu_io_test.cpp
    cpu_load_store_test.cpp
    cpu_lpm_test.cpp
    cpu_multibyte_compare_test.cpp
    cpu_pin_change_interrupt_test.cpp
    cpu_pin_change_interrupt_groups_test.cpp
    cpu_pin_change_interrupt_firmware_test.cpp
    cpu_sleep_wake_test.cpp
    cpu_small_ops_test.cpp
    cpu_spm_boundary_test.cpp
    cpu_stack_test.cpp
    cpu_timer_test.cpp
    cpu_timer_pwm_test.cpp
    cpu_timer_control_test.cpp
    cpu_timer_gpio_test.cpp
    cpu_timer2_gpio_test.cpp
    cpu_timer_dual_interrupt_test.cpp
    cpu_timer_external_clock_test.cpp
    cpu_timer_external_clock_firmware_test.cpp
    cpu_timer16_test.cpp
    cpu_spi_test.cpp
    cpu_twi_test.cpp
    cpu_uart_test.cpp
    cpu_uart_interrupt_test.cpp
    cpu_uart_firmware_test.cpp
    cpu_voltage_interrupt_test.cpp
    cpu_word_ops_test.cpp
    cpu_watchdog_test.cpp
    cpu_register_mapping_test.cpp
    fetch_metadata_test.cpp
    hex_loader_test.cpp
    sync_engine_test.cpp
    viospice_timer2_async_test.cpp
    cpu_trace_util.cpp
    cpu_trace_util_test.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

foreach(source IN LISTS TEST_SOURCES)
    get_filename_component(test_name ${source} NAME_WE)
    set(target_name "vioavr_${test_name}")
    
    add_executable(${target_name} ${source})
    target_link_libraries(${target_name} PRIVATE VioAVR::core)
    add_test(NAME ${target_name} COMMAND ${target_name})
endforeach()
