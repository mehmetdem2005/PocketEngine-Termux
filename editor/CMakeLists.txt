# Editor application - Unity-style layout, landscape-first

file(GLOB_RECURSE EDITOR_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE EDITOR_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

add_executable(pocketeditor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

target_link_libraries(pocketeditor
    PRIVATE
        pocketengine
)

target_include_directories(pocketeditor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Force landscape orientation hint for X11
target_compile_definitions(pocketeditor PRIVATE POCKET_LANDSCAPE=1)
