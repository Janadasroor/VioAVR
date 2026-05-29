option(VIOAVR_BUILD_COSIM "Build d_cosim co-simulation shim" ON)

if(VIOAVR_BUILD_COSIM)
    set(VioMATRIXC_ROOT "${PROJECT_SOURCE_DIR}/../VioMATRIXC"
        CACHE PATH "Path to VioMATRIXC source tree")

    # ------------------------------------------------------------------
    # d_cosim co-simulation shim (loaded by VioMATRIXC's d_cosim code model)
    # ------------------------------------------------------------------
    add_library(avr_cosim SHARED avr_cosim_shim.cpp)
    target_link_libraries(avr_cosim PRIVATE VioAVR::core)

    target_include_directories(avr_cosim PRIVATE
        "${VioMATRIXC_ROOT}/src/include"
    )

    set_target_properties(avr_cosim PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cosim"
    )

    # ------------------------------------------------------------------
    # viospice.cm — VioAVR XSPICE code model library
    # Loaded by ngspice via .spiceinit: codemodel /path/to/viospice.cm
    #
    # avr_adc_bridge: analog voltage input -> VioAVR signal bank injection.
    # Uses dlsym(RTLD_DEFAULT) to find the active VioAVR handle from
    # libavr_cosim.so (loaded with RTLD_GLOBAL by d_cosim).
    #
    # Standalone shared library — no VioAVR link deps.
    # ------------------------------------------------------------------
    set(VIOSPICE_CM_SRCS
        avr_analog_bridge/ifspec.c
        avr_analog_bridge/cfunc.c
        avr_analog_bridge/dlmain.c
    )

    add_library(viospice_cm MODULE ${VIOSPICE_CM_SRCS})
    target_include_directories(viospice_cm PRIVATE
        "${VioMATRIXC_ROOT}/src/include"
    )
    target_link_libraries(viospice_cm PRIVATE ${CMAKE_DL_LIBS})

    set_target_properties(viospice_cm PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        PREFIX ""
        OUTPUT_NAME "viospice"
        SUFFIX ".cm"
    )

    # Also build as avr_adc_bridge.cm in cosim/ subdir for direct reference
    add_library(avr_adc_bridge MODULE ${VIOSPICE_CM_SRCS})
    target_include_directories(avr_adc_bridge PRIVATE
        "${VioMATRIXC_ROOT}/src/include"
    )
    target_link_libraries(avr_adc_bridge PRIVATE ${CMAKE_DL_LIBS})

    set_target_properties(avr_adc_bridge PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cosim"
        PREFIX ""
        SUFFIX ".cm"
    )
endif()
