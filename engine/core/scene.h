// ============================================================
//  pocket/engine/core/scene.h
//  Scene = World + camera + serializer
// ============================================================
#pragma once

#include "core/types.h"
#include "core/ecs.h"
#include "core/math.h"

namespace pk::scene {

struct Scene {
    String         name{"Untitled"};
    ecs::World     world;
    math::Vec4     ambientColor{0.3f, 0.3f, 0.3f, 1.0f};
    math::Vec4     backgroundColor{0.12f, 0.12f, 0.15f, 1.0f};
    bool           paused{false};
    f32            timeScale{1.0f};
};

// Save/load scene to JSON file
// (non-const Scene& because we iterate components via ECS each<>() which
// requires non-const access for now)
bool saveScene(Scene& s, const String& path) noexcept;
bool loadScene(Scene& s, const String& path) noexcept;

} // namespace pk::scene
