cmake_minimum_required(VERSION 3.13)
project(radio)

file(GLOB_RECURSE SRC "src/*.cpp")

include(${SDRPP_MODULE_CMAKE})

target_include_directories(radio PRIVATE "src/")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMENT "Copy $<TARGET_FILE_NAME:${PROJECT_NAME}> to $<TARGET_FILE_DIR:sdrpp>/modules"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" $<TARGET_FILE_DIR:sdrpp>/modules/$<TARGET_FILE_NAME:${PROJECT_NAME}>)
endif ()
