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
)

target_include_directories(vioavr_oscope PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vioavr_oscope PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
)
