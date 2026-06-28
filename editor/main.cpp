// ============================================================
//  pocket/editor/main.cpp
//  PocketEngine editor entry point
// ============================================================
#include "editor.h"
#include "core/log.h"
#include "platform/window.h"

int main(int argc, char** argv) {
    using namespace pk;

    log::setLevel(log::Level::Info);

    platform::WindowDesc desc;
    desc.width     = 1280;
    desc.height    = 720;
    desc.title     = "PocketEngine Editor";
    desc.landscape = true;
    desc.resizable = true;

    platform::Window window;
    if (!window.create(desc)) {
        PK_LOG_FATAL("Main", "Window creation failed");
        return 1;
    }

    editor::Editor ed;
    if (!ed.init(&window)) {
        PK_LOG_FATAL("Main", "Editor init failed");
        return 1;
    }

    ed.run();
    ed.shutdown();
    return 0;
}
