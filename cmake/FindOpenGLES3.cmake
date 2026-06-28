# ============================================================
#  FindOpenGLES3.cmake
#  Find OpenGL ES 3 library and headers on Linux/Debian
#
#  Output variables:
#    OpenGLES3_FOUND       - TRUE if found
#    OpenGLES3_INCLUDE_DIRS - include directory
#    OpenGLES3_LIBRARIES   - libraries to link
#    OpenGLES3::OpenGLES3  - imported target
# ============================================================

find_path(OpenGLES3_INCLUDE_DIR
    NAMES GLES3/gl3.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/vc/include
    DOC "OpenGL ES 3 include directory"
)

find_library(OpenGLES3_LIBRARY
    NAMES GLESv3 GLESv2
    PATHS
        /usr/lib
        /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
        /usr/local/lib
        /opt/vc/lib
    DOC "OpenGL ES 3 library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGLES3
    REQUIRED_VARS OpenGLES3_LIBRARY OpenGLES3_INCLUDE_DIR
)

if(OpenGLES3_FOUND AND NOT TARGET OpenGLES3::OpenGLES3)
    add_library(OpenGLES3::OpenGLES3 UNKNOWN IMPORTED)
    set_target_properties(OpenGLES3::OpenGLES3 PROPERTIES
        IMPORTED_LOCATION "${OpenGLES3_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OpenGLES3_INCLUDE_DIR}"
    )
    set(OpenGLES3_INCLUDE_DIRS ${OpenGLES3_INCLUDE_DIR})
    set(OpenGLES3_LIBRARIES    ${OpenGLES3_LIBRARY})
endif()

mark_as_advanced(OpenGLES3_INCLUDE_DIR OpenGLES3_LIBRARY)
