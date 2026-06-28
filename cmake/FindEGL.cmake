# ============================================================
#  FindEGL.cmake
#  Find EGL library and headers on Linux/Debian
#
#  Output variables:
#    EGL_FOUND       - TRUE if found
#    EGL_INCLUDE_DIRS - include directory
#    EGL_LIBRARIES   - libraries to link
#    EGL::EGL        - imported target
# ============================================================

find_path(EGL_INCLUDE_DIR
    NAMES EGL/egl.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/vc/include
    DOC "EGL include directory"
)

find_library(EGL_LIBRARY
    NAMES EGL
    PATHS
        /usr/lib
        /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
        /usr/local/lib
        /opt/vc/lib
    DOC "EGL library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EGL
    REQUIRED_VARS EGL_LIBRARY EGL_INCLUDE_DIR
)

if(EGL_FOUND AND NOT TARGET EGL::EGL)
    add_library(EGL::EGL UNKNOWN IMPORTED)
    set_target_properties(EGL::EGL PROPERTIES
        IMPORTED_LOCATION "${EGL_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${EGL_INCLUDE_DIR}"
    )
    set(EGL_INCLUDE_DIRS ${EGL_INCLUDE_DIR})
    set(EGL_LIBRARIES    ${EGL_LIBRARY})
endif()

mark_as_advanced(EGL_INCLUDE_DIR EGL_LIBRARY)
