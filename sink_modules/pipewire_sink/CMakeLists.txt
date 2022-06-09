cmake_minimum_required(VERSION 3.13)
project(pipewire_sink)

if (MSVC)
    message(FATAL_ERROR "PipeWire is not supported on Windows. (use -DOPT_BUILD_PIPEWIRE_SINK=OFF)")
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

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMENT "Copy $<TARGET_FILE_NAME:${PROJECT_NAME}> to $<TARGET_FILE_DIR:sdrpp>/modules"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" $<TARGET_FILE_DIR:sdrpp>/modules/$<TARGET_FILE_NAME:${PROJECT_NAME}>)
endif ()

# Install directives
install(TARGETS ${PROJECT_NAME} DESTINATION lib/sdrpp/plugins)