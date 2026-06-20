get_filename_component(_DANTE_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# The test app links against the committed prebuilt — no source compilation required
add_library(dante_buffer_client STATIC IMPORTED GLOBAL)
set_target_properties(dante_buffer_client PROPERTIES
    IMPORTED_LOCATION "${_DANTE_SDK_ROOT}/lib/libdep_audio.a"
    INTERFACE_INCLUDE_DIRECTORIES "${_DANTE_SDK_ROOT}/include"
)
target_link_libraries(dante_buffer_client INTERFACE pthread rt)

# SDK dist target: compile DanteAudio.cpp and merge with libdep_audio.a to produce
# the combined artifact. This documents how the committed .a was built.
add_library(dante_audio_impl OBJECT "${_DANTE_SDK_ROOT}/src/DanteAudio.cpp")
target_include_directories(dante_audio_impl PRIVATE "${_DANTE_SDK_ROOT}/include")

set(_DANTE_DIST_DIR "${CMAKE_BINARY_DIR}/dante-dep-sdk")
set(_DANTE_DIST_LIB "${_DANTE_DIST_DIR}/lib/libdep_audio.a")

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

add_custom_target(dante_sdk_dist ALL
    DEPENDS "${_DANTE_DIST_LIB}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_DANTE_DIST_DIR}/include/dante"
    COMMAND ${CMAKE_COMMAND} -E copy
            "${_DANTE_SDK_ROOT}/include/dante/DanteAudio.hpp"
            "${_DANTE_DIST_DIR}/include/dante/DanteAudio.hpp"
    COMMENT "Dante SDK distribution: build/dante-dep-sdk/"
)
