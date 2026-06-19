add_executable(vioavr main.cpp cmd_arduino.cpp cmd_debug.cpp)

target_link_libraries(vioavr
    PRIVATE
        VioAVR::core
)

target_compile_features(vioavr PRIVATE cxx_std_20)
enable_lto(vioavr)
