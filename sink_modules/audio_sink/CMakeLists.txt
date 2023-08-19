cmake_minimum_required(VERSION 3.13)
project(audio_sink)

file(GLOB SRC "src/*.cpp")

include(${SDRPP_MODULE_CMAKE})

if (MSVC)
    # Lib path
    target_link_directories(audio_sink PRIVATE "C:/libs/rtaudio-6.0.0/install/lib")

    # Misc headers
    target_include_directories(audio_sink PRIVATE "C:/libs/rtaudio-6.0.0/install/include")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(audio_sink PRIVATE rtaudiod)
else()
    target_link_libraries(audio_sink PRIVATE rtaudio)
endif ()
else (MSVC)
    find_package(PkgConfig)

    pkg_check_modules(RTAUDIO REQUIRED rtaudio)

    target_include_directories(audio_sink PRIVATE ${RTAUDIO_INCLUDE_DIRS})
    target_link_directories(audio_sink PRIVATE ${RTAUDIO_LIBRARY_DIRS})
    target_link_libraries(audio_sink PRIVATE ${RTAUDIO_LIBRARIES})

endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMENT "Copy $<TARGET_FILE_NAME:${PROJECT_NAME}> to $<TARGET_FILE_DIR:sdrpp>/modules"
            COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${PROJECT_NAME}>" $<TARGET_FILE_DIR:sdrpp>/modules/$<TARGET_FILE_NAME:${PROJECT_NAME}>)
endif ()

