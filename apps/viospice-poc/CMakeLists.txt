add_executable(viospice-poc main.cpp)
target_link_libraries(viospice-poc PRIVATE vioavr_core)
target_include_directories(viospice-poc PRIVATE ${CMAKE_SOURCE_DIR}/include)

vioavr_static_windows_runtime(viospice-poc)
