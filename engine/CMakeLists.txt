# ============================================================
#  Engine core library
# ============================================================

# Collect sources
file(GLOB_RECURSE ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/physics/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/audio/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripting/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/platform/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/network/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/db/*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_library(pocketengine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(pocketengine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(pocketengine
    PUBLIC
        Threads::Threads
)

if(POCKET_USE_OPENGL_ES)
    target_link_libraries(pocketengine PUBLIC OpenGLES3::OpenGLES3 EGL::EGL)
    target_compile_definitions(pocketengine PUBLIC POCKET_GLES3=1)
else()
    target_link_libraries(pocketengine PUBLIC OpenGL::GL)
endif()

target_link_libraries(pocketengine PUBLIC glfw glm)

if(POCKET_USE_BOX2D)
    find_package(Box2D REQUIRED)
    target_link_libraries(pocketengine PUBLIC Box2D::Box2D)
    target_compile_definitions(pocketengine PUBLIC POCKET_HAS_BOX2D=1)
endif()

if(POCKET_USE_BULLET)
    find_package(Bullet REQUIRED)
    target_link_libraries(pocketengine PUBLIC Bullet::Bullet)
    target_compile_definitions(pocketengine PUBLIC POCKET_HAS_BULLET=1)
endif()

if(POCKET_USE_LUA)
    find_package(Lua REQUIRED)
    target_link_libraries(pocketengine PUBLIC Lua::Lua)
    target_compile_definitions(pocketengine PUBLIC POCKET_HAS_LUA=1)
endif()

if(POCKET_USE_OPENAL)
    find_package(OpenAL REQUIRED)
    target_link_libraries(pocketengine PUBLIC OpenAL::OpenAL)
    target_compile_definitions(pocketengine PUBLIC POCKET_HAS_OPENAL=1)
endif()

if(POCKET_USE_SQLITE)
    find_package(SQLite3 REQUIRED)
    target_link_libraries(pocketengine PUBLIC SQLite::SQLite3)
    target_compile_definitions(pocketengine PUBLIC POCKET_HAS_SQLITE=1)
endif()

# ImGui (vendored)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/imgui third_party/imgui-build EXCLUDE_FROM_ALL)
target_link_libraries(pocketengine PUBLIC imgui)
target_compile_definitions(pocketengine PUBLIC POCKET_HAS_IMGUI=1)
