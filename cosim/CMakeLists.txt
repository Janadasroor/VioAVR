option(VIOAVR_BUILD_COSIM "Build d_cosim co-simulation shim" ON)

if(VIOAVR_BUILD_COSIM)
    set(VioMATRIXC_ROOT "${PROJECT_SOURCE_DIR}/../VioMATRIXC"
        CACHE PATH "Path to VioMATRIXC source tree")

    add_library(avr_cosim SHARED avr_cosim_shim.cpp)
    target_link_libraries(avr_cosim PRIVATE VioAVR::core)

    target_include_directories(avr_cosim PRIVATE
        "${VioMATRIXC_ROOT}/src/include"
    )

    set_target_properties(avr_cosim PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cosim"
    )
endif()
