cmake_minimum_required(VERSION 3.13)
project(pipewire_sink)

if (MSVC)
    message(FATAL_ERROR "PipeWire is not supported on Windows.")
endif ()

set(SOURCES
        src/main.cpp
        )

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE sdrpp_core)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_compile_options(${PROJECT_NAME} PRIVATE -O3 -std=c++17)

target_include_directories(${PROJECT_NAME} PRIVATE /usr/include/pipewire-0.3)
target_include_directories(${PROJECT_NAME} PRIVATE /usr/include/spa-0.2)
target_link_libraries(${PROJECT_NAME} PRIVATE libpipewire-0.3.so)

# Install directives
install(TARGETS ${PROJECT_NAME} DESTINATION lib/sdrpp/plugins)