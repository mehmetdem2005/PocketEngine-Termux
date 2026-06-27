# ============================================================
#  third_party/imgui/CMakeLists.txt
#  ImGui build (vendor from /opt/imgui at build time)
# ============================================================
cmake_minimum_required(VERSION 3.16)

# Path to imgui sources - either vendored or system-installed
set(IMGUI_DIR "" CACHE PATH "Path to ImGui source directory")
if(NOT IMGUI_DIR)
    set(IMGUI_DIR "/opt/imgui" CACHE PATH "" FORCE)
endif()

if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(WARNING "ImGui not found at ${IMGUI_DIR}. Run scripts/termux/setup_debian.sh to install.")
endif()

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

find_package(glfw3 REQUIRED)
target_link_libraries(imgui PUBLIC glfw)

if(POCKET_USE_OPENGL_ES)
    target_link_libraries(imgui PUBLIC OpenGLES3::OpenGLES3 EGL::EGL)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_ES3)
else()
    target_link_libraries(imgui PUBLIC OpenGL::GL)
endif()
