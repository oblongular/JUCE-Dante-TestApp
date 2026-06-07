get_filename_component(_DANTE_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

add_library(dante_buffer_client STATIC IMPORTED GLOBAL)

set_target_properties(dante_buffer_client PROPERTIES
    IMPORTED_LOCATION "${_DANTE_SDK_ROOT}/lib/libdep_audio.a"
    INTERFACE_INCLUDE_DIRECTORIES "${_DANTE_SDK_ROOT}/include"
)

target_link_libraries(dante_buffer_client INTERFACE pthread rt)
