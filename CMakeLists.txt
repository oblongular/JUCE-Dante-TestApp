cmake_minimum_required(VERSION 3.22)
project(JUCE-Dante-TestApp VERSION 1.0.0)

# Dante DEP SDK (pre-built)
include(../Dante-DEP-Client-SDK/cmake/libDanteAudio.cmake)

# JUCE
add_subdirectory(../JUCE juce)

juce_add_console_app(JUCE-Dante-TestApp PRODUCT_NAME "JUCE Dante Test App")

target_sources(JUCE-Dante-TestApp PRIVATE
    main.cpp
)

target_compile_definitions(JUCE-Dante-TestApp PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DANTE=1
)

target_link_libraries(JUCE-Dante-TestApp PRIVATE
    juce::juce_audio_devices
    juce::juce_core
    DanteAudio
)

# Rebuild the Dante SDK .a from source (explicit only: cmake --build build --target dante_sdk_dist)
set(_DANTE_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../Dante-DEP-Client-SDK")
set(_DANTE_DIST_DIR "${CMAKE_BINARY_DIR}/dante-dep-sdk")
set(_DANTE_DIST_LIB "${_DANTE_DIST_DIR}/lib/libDanteAudio.a")

add_library(dante_audio_impl OBJECT EXCLUDE_FROM_ALL "${_DANTE_SDK_ROOT}/src/DanteAudio.cpp")
target_include_directories(dante_audio_impl PRIVATE "${_DANTE_SDK_ROOT}/include")

add_custom_command(
    OUTPUT "${_DANTE_DIST_LIB}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_DANTE_DIST_DIR}/lib"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${_DANTE_SDK_ROOT}/lib/libdep_audio.a"
            "${_DANTE_DIST_LIB}"
    COMMAND ${CMAKE_AR} q "${_DANTE_DIST_LIB}"
            $<TARGET_OBJECTS:dante_audio_impl>
    DEPENDS dante_audio_impl "${_DANTE_SDK_ROOT}/lib/libdep_audio.a"
    COMMENT "Building combined Dante SDK library"
)

add_custom_target(dante_sdk_dist
    DEPENDS "${_DANTE_DIST_LIB}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_DANTE_DIST_DIR}/include/dante"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${_DANTE_SDK_ROOT}/include/dante/DanteAudio.hpp"
            "${_DANTE_DIST_DIR}/include/dante/DanteAudio.hpp"
    COMMENT "Dante SDK distribution: build/dante-dep-sdk/"
)
