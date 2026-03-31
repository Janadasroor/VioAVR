find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(vioavr_qt STATIC)
add_library(VioAVR::qt ALIAS vioavr_qt)

target_sources(vioavr_qt
    PRIVATE
        qt_sync_bridge.cpp
)

target_include_directories(vioavr_qt
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(vioavr_qt
    PUBLIC
        Qt6::Core
        VioAVR::core
)

target_compile_features(vioavr_qt PUBLIC cxx_std_20)
