cmake_minimum_required(VERSION 3.16)
project(PocketEngine VERSION 0.1.0 LANGUAGES C CXX)

# ============================================================
#  PocketEngine - C++ Game Engine for Termux:X11
#  Unity/Godot-inspired, OpenGL ES 3.0, ImGui, ECS, MT
# ============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Architecture flags (ARM64 phone optimization)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    add_compile_options(-march=native -mtune=native -ffast-math -funsafe-math-optimizations)
endif()

# Warning flags (treat warnings seriously)
add_compile_options(
    -Wall -Wextra -Wpedantic -Werror
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Wshadow -Wnon-virtual-dtor -Wold-style-cast
    -Wcast-align -Woverloaded-virtual -Wconversion
)

# Release: full optimization
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -flto -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g2 -DNDEBUG")

# Output dirs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Custom Find modules (FindOpenGLES3, FindEGL)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# ============================================================
#  Platform detection
# ============================================================
if(ENV{TERMUX_VERSION})
    set(POCKET_PLATFORM_TERMUX ON)
    message(STATUS "Building on Termux")
endif()

if(EXISTS "/data/data/com.termux")
    set(POCKET_PLATFORM_ANDROID ON)
    add_compile_definitions(POCKET_ANDROID)
endif()

# ============================================================
#  Options
# ============================================================
option(POCKET_BUILD_EDITOR     "Build the editor application"            ON)
option(POCKET_BUILD_RUNTIME    "Build the runtime (player)"              ON)
option(POCKET_BUILD_TESTS      "Build unit tests"                        OFF)
option(POCKET_BUILD_SERVER     "Build server-side tools (not used on phone)" OFF)
option(POCKET_USE_BOX2D        "Enable 2D physics via Box2D"             ON)
option(POCKET_USE_BULLET       "Enable 3D physics via Bullet3"           ON)
option(POCKET_USE_LUA          "Enable Lua scripting"                    ON)
option(POCKET_USE_OPENAL       "Enable OpenAL audio"                     ON)
option(POCKET_USE_SQLITE       "Enable SQLite local database"            ON)
option(POCKET_USE_OPENGL_ES    "Use OpenGL ES (mobile) instead of desktop GL" ON)

# ============================================================
#  Find packages
# ============================================================
find_package(Threads REQUIRED)

# OpenGL ES / EGL
if(POCKET_USE_OPENGL_ES)
    find_package(OpenGLES3 REQUIRED)
    find_package(EGL REQUIRED)
    add_compile_definitions(POCKET_GLES3)
else()
    find_package(OpenGL REQUIRED)
endif()

# GLFW for windowing (X11 via Termux:X11)
# Try find_package first, fall back to pkg-config, then manual find
find_path(GLFW3_INCLUDE_DIR GLFW/glfw3.h
    PATHS /usr/include /usr/local/include /opt/vc/include)
find_library(GLFW3_LIBRARY
    NAMES glfw3 glfw
    PATHS /usr/lib /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE} /usr/local/lib /opt/vc/lib)
if(NOT GLFW3_INCLUDE_DIR OR NOT GLFW3_LIBRARY)
    message(FATAL_ERROR "GLFW3 not found - run: apt-get install -y libglfw3-dev")
endif()
add_library(pk_glfw3 INTERFACE IMPORTED)
set_target_properties(pk_glfw3 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GLFW3_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES      "${GLFW3_LIBRARY}"
)

# GLM (math) - header-only
find_path(GLM_INCLUDE_DIR glm/glm.hpp
    PATHS /usr/include /usr/local/include)
if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "GLM not found - run: apt-get install -y libglm-dev")
endif()
add_library(pk_glm INTERFACE IMPORTED)
set_target_properties(pk_glm PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIR}"
)

# stb_image (header-only, vendored)
include_directories(third_party/stb)

#============================================================
# Subdirs
#============================================================
add_subdirectory(engine)
if(POCKET_BUILD_EDITOR)
    add_subdirectory(editor)
endif()
if(POCKET_BUILD_RUNTIME)
    add_subdirectory(runtime)
endif()
if(POCKET_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Print config summary
message(STATUS "============================================")
message(STATUS "  PocketEngine ${PROJECT_VERSION}")
message(STATUS "============================================")
message(STATUS "  Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Editor:        ${POCKET_BUILD_EDITOR}")
message(STATUS "  Runtime:       ${POCKET_BUILD_RUNTIME}")
message(STATUS "  OpenGL ES:     ${POCKET_USE_OPENGL_ES}")
message(STATUS "  Box2D (2D):    ${POCKET_USE_BOX2D}")
message(STATUS "  Bullet (3D):   ${POCKET_USE_BULLET}")
message(STATUS "  Lua:           ${POCKET_USE_LUA}")
message(STATUS "  OpenAL:        ${POCKET_USE_OPENAL}")
message(STATUS "  SQLite:        ${POCKET_USE_SQLITE}")
message(STATUS "============================================")

# Install rules
install(TARGETS pocketengine pocketeditor pocketruntime
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
