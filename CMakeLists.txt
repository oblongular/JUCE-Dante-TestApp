cmake_minimum_required(VERSION 3.22)
project(JUCE-Dante-TestApp VERSION 1.0.0)

# Dante DEP SDK (pre-built)
include(dante-dep-sdk/cmake/DanteAudioBuffers.cmake)

# JUCE
add_subdirectory(../JUCE juce)

juce_add_console_app(JUCE-Dante-TestApp PRODUCT_NAME "JUCE Dante Test App")

target_sources(JUCE-Dante-TestApp PRIVATE main.cpp)

target_compile_definitions(JUCE-Dante-TestApp PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DANTE=1
)

target_link_libraries(JUCE-Dante-TestApp PRIVATE
    juce::juce_audio_devices
    juce::juce_core
    dante_buffer_client
)
