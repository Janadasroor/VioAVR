add_executable(vioavr_cli main.cpp)

target_link_libraries(vioavr_cli
    PRIVATE
        VioAVR::core
)

target_compile_features(vioavr_cli PRIVATE cxx_std_20)
