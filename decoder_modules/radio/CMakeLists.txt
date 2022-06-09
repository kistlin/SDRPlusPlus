cmake_minimum_required(VERSION 3.13)
project(radio)

file(GLOB_RECURSE SRC "src/*.cpp")

add_library(radio SHARED ${SRC})
target_link_libraries(radio PRIVATE sdrpp_core)
set_target_properties(radio PROPERTIES PREFIX "")

target_include_directories(radio PRIVATE "src/")

if (MSVC)
    target_compile_options(radio PRIVATE /O2 /Ob2 /std:c++17 /EHsc)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(radio PRIVATE -O3 -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup)
else ()
    target_compile_options(radio PRIVATE -O3 -std=c++17)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMENT "Copy $<TARGET_FILE_NAME:${PROJECT_NAME}> to $<TARGET_FILE_DIR:sdrpp>/modules"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" $<TARGET_FILE_DIR:sdrpp>/modules/$<TARGET_FILE_NAME:${PROJECT_NAME}>)
endif ()

# Install directives
install(TARGETS radio DESTINATION lib/sdrpp/plugins)