cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)
project(uhd_source)

set(SOURCES
        src/device.cpp
        src/main.cpp
        )

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE sdrpp_core)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

target_include_directories(${PROJECT_NAME} PRIVATE "src/")

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -std=c++17)
endif ()

if (MSVC)
    find_package(UHD)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UHD_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${UHD_LIBRARIES})
else (MSVC)
    find_package(UHD)
    target_link_libraries(${PROJECT_NAME} PRIVATE uhd)
endif ()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMENT "Copy $<TARGET_FILE_NAME:${PROJECT_NAME}> to $<TARGET_FILE_DIR:sdrpp>/modules"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" $<TARGET_FILE_DIR:sdrpp>/modules/$<TARGET_FILE_NAME:${PROJECT_NAME}>)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND MSVC)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMENT "Copy $<TARGET_FILE_NAME:${PROJECT_NAME}> to $<TARGET_FILE_DIR:sdrpp>/modules"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:${PROJECT_NAME}>/$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.pdb" $<TARGET_FILE_DIR:sdrpp>/modules/$<TARGET_FILE_BASE_NAME:${PROJECT_NAME}>.pdb)
endif()

# Install directives
install(TARGETS ${PROJECT_NAME} DESTINATION lib/sdrpp/plugins)
