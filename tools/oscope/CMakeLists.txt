cmake_minimum_required(VERSION 3.25)
project(vioavr_oscope LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts)

add_executable(vioavr_oscope
    main.cpp
    main_window.cpp
    main_window.h
    waveform_viewer.cpp
    waveform_viewer.h
    waveform_expression_dialog.cpp
    waveform_expression_dialog.h
    measurement_dialog.cpp
    measurement_dialog.h
    analysis_dialog.cpp
    analysis_dialog.h
    fft_analyzer.cpp
    fft_analyzer.h
    theme.cpp
    theme.h
    theme_manager.cpp
    theme_manager.h
    si_formatter.h
    instrument_panel.cpp
    instrument_panel.h
    led_matrix_widget.cpp
    led_matrix_widget.h
    instrument_detector.cpp
    instrument_detector.h
    led_matrix_window.cpp
    led_matrix_window.h
    segment7_widget.cpp
    segment7_widget.h
    segment7_window.cpp
    segment7_window.h
    lcd_widget.cpp
    lcd_widget.h
    lcd_window.cpp
    lcd_window.h
)

target_include_directories(vioavr_oscope PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vioavr_oscope PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
)
