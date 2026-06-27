// ============================================================
//  pocket/runtime/main.cpp
//  PocketEngine runtime (player) - runs a packaged scene
// ============================================================
#include "core/log.h"
#include "core/ecs.h"
#include "core/scene.h"
#include "core/math.h"
#include "platform/window.h"
#include "renderer/renderer.h"
#include "assets/asset_manager.h"

int main(int argc, char** argv) {
    using namespace pk;

    log::setLevel(log::Level::Info);

    String scenePath = (argc > 1) ? argv[1] : "scene.json";

    platform::WindowDesc desc;
    desc.width = 1280;
    desc.height = 720;
    desc.title = "PocketEngine Player";
    desc.landscape = true;
    platform::Window window;
    if (!window.create(desc)) return 1;

    auto& r = rnd::renderer();
    auto fb = window.framebufferSize();
    r.init(fb.x, fb.y);

    scene::Scene scene;
    scene::loadScene(scene, scenePath);

    f32 camX = 0, camY = 0, camZoom = 5.0f;

    while (!window.shouldClose()) {
        window.pollEvents();
        r.setClearColor(scene.backgroundColor.x, scene.backgroundColor.y,
                        scene.backgroundColor.z, scene.backgroundColor.w);
        r.clear();

        f32 aspect = (f32)fb.x / (f32)fb.y;
        auto vp = math::ortho(-camZoom * aspect + camX, camZoom * aspect + camX,
                              -camZoom + camY, camZoom + camY, -100, 100);
        r.beginScene(vp);
        scene.world.each<ecs::Transform, ecs::Sprite>(
            [&](EntityID, ecs::Transform& t, ecs::Sprite& s) {
                r.drawSprite(s.textureID, t.x - s.width * 0.5f * t.sx,
                             t.y - s.height * 0.5f * t.sy,
                             s.width * t.sx, s.height * t.sy,
                             s.u0, s.v0, s.u1, s.v1,
                             math::Vec4(s.color[0], s.color[1], s.color[2], s.color[3]));
            });
        r.endScene();
        window.swapBuffers();
    }

    r.shutdown();
    return 0;
}
