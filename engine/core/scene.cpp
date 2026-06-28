// ============================================================
//  pocket/engine/core/scene.cpp
// ============================================================
#include "core/scene.h"
#include "core/log.h"

#include <fstream>
#include <sstream>

namespace pk::scene {

namespace {
// Minimal JSON writer (no dep on nlohmann)
String esc(const String& s) {
    String out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;
        }
    }
    return out;
}
}

bool saveScene(Scene& s, const String& path) noexcept {
    std::ofstream f(path);
    if (!f) {
        PK_LOG_ERROR("Scene", "Cannot open %s for writing", path.c_str());
        return false;
    }
    f << "{\n";
    f << "  \"name\": \"" << esc(s.name) << "\",\n";
    f << "  \"entities\": [\n";
    // Iterate all entities (we iterate Tag component as driver)
    // For simplicity, write Tag + Transform + Sprite
    bool first = true;
    s.world.each<ecs::Tag, ecs::Transform>([&](EntityID e, ecs::Tag& tag, ecs::Transform& t) {
        if (!first) f << ",\n";
        first = false;
        f << "    {\n";
        f << "      \"id\": " << e.value << ",\n";
        f << "      \"name\": \"" << esc(tag.name) << "\",\n";
        f << "      \"transform\": {\"x\":" << t.x << ",\"y\":" << t.y
          << ",\"z\":" << t.z << ",\"sx\":" << t.sx << ",\"sy\":" << t.sy << "}\n";
        if (auto* sp = s.world.get<ecs::Sprite>(e)) {
            f << "      ,\"sprite\": {\"tex\":" << sp->textureID
              << ",\"w\":" << sp->width << ",\"h\":" << sp->height << "}\n";
        }
        f << "    }";
    });
    f << "\n  ]\n}\n";
    PK_LOG_INFO("Scene", "Saved scene '%s' to %s", s.name.c_str(), path.c_str());
    return true;
}

bool loadScene(Scene& s, const String& path) noexcept {
    std::ifstream f(path);
    if (!f) {
        PK_LOG_ERROR("Scene", "Cannot open %s", path.c_str());
        return false;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    String content = ss.str();
    // Minimal JSON parser skipped for brevity - real impl uses nlohmann::json
    PK_LOG_WARN("Scene", "loadScene: JSON parsing stub - implement nlohmann/json");
    (void)content;
    return true;
}

} // namespace pk::scene
