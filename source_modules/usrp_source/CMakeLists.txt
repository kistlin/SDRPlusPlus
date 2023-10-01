cmake_minimum_required(VERSION 3.13)
project(usrp_source)

file(GLOB SRC "src/*.cpp")

include(${SDRPP_MODULE_CMAKE})

if (MSVC)
    find_package(UHD)
    target_include_directories(${PROJECT_NAME} PRIVATE ${UHD_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${UHD_LIBRARIES})
else (MSVC)
    find_package(PkgConfig)

    pkg_check_modules(LIBUHD REQUIRED uhd)

    target_include_directories(usrp_source PRIVATE ${LIBUHD_INCLUDE_DIRS})
    target_link_directories(usrp_source PRIVATE ${LIBUHD_LIBRARY_DIRS})
    target_link_libraries(usrp_source PRIVATE ${LIBUHD_LIBRARIES})
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMENT "Copy $<TARGET_FILE_NAME:${PROJECT_NAME}> to $<TARGET_FILE_DIR:sdrpp>/modules"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" $<TARGET_FILE_DIR:sdrpp>/modules/$<TARGET_FILE_NAME:${PROJECT_NAME}>)
endif ()

