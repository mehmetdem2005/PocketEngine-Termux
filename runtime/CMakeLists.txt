# Runtime / player - builds a packaged game

file(GLOB_RECURSE RUNTIME_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(pocketruntime ${RUNTIME_SOURCES})

target_link_libraries(pocketruntime
    PRIVATE
        pocketengine
)

target_include_directories(pocketruntime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
