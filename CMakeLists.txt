cmake_minimum_required(VERSION 3.22)
project(JUCE-Dante-TestApp VERSION 1.0.0)

# Resolve dependency paths: CMake variable > environment variable > sibling directory
if(NOT JUCE_DIR)
  if(DEFINED ENV{JUCE_DIR})
    set(JUCE_DIR "$ENV{JUCE_DIR}")
  else()
    set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
  endif()
endif()

if(NOT DANTE_SDK_DIR)
  if(DEFINED ENV{DANTE_SDK_DIR})
    set(DANTE_SDK_DIR "$ENV{DANTE_SDK_DIR}")
  else()
    set(DANTE_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Dante-DEP-Client-SDK")
  endif()
endif()

# Dante DEP SDK (pre-built)
include(${DANTE_SDK_DIR}/cmake/libDanteAudio.cmake)

# JUCE
add_subdirectory(${JUCE_DIR} juce)

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
